#include "../imp_package.hpp"
#include "import_canvas_3d.hpp"
#include "import_step/step_importer.hpp"
#include "util/geom_util.hpp"
#include <HLRBRep_Algo.hxx>
#include <HLRBRep_HLRToShape.hxx>
#include <gp_Ax2.hxx>
#include <gp_Ax3.hxx>
#include <gp_Trsf.hxx>
#include <TopoDS_Iterator.hxx>
#include <TopoDS_Edge.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Vertex.hxx>
#include <GCPnts_TangentialDeflection.hxx>
#include <gp_Circ.hxx>
#include <ShapeAnalysis_Edge.hxx>
#include "canvas/annotation.hpp"
#include <glm/gtx/rotate_vector.hpp>
#include <BRep_Tool.hxx>
#include "pool/pool_manager.hpp"
#include "model_editor.hpp"
#include "3d_view.hpp"

namespace horizon {

STEPImporter::Faces ImpPackage::ImportCanvas3D::import_step(const std::string &filename_rel,
                                                            const std::string &filename_abs)
{
    auto importer = std::make_unique<STEPImporter::STEPImporter>(filename_abs);
    auto result = importer->get_faces_and_points();
    {
        std::lock_guard<std::mutex> lock(imp.model_info_mutex);
        auto &inf = imp.model_info[filename_rel];
        inf.importer = std::move(importer);
        inf.points.reserve(result.points.size());
        for (const auto &it : result.points) {
            inf.points.emplace_back(it.x, it.y, it.z);
        }
    }
    return result.faces;
}

std::optional<std::pair<std::string, std::string>>
ImpPackage::ImportCanvas3D::get_model_filename(const BoardPackage &pkg, IPool &pool)
{
    auto model = pkg.package.get_model(pkg.model);
    if (model) {
        UUID this_pool_uuid = pool.get_pool_info().uuid;
        UUID pkg_pool_uuid = this_pool_uuid;
        try {
            pool.get_package(pkg.package.uuid, &pkg_pool_uuid);
        }
        catch (const std::runtime_error &e) {
            // it's okay
        }
        auto pkg_pool = PoolManager::get().get_by_uuid(pkg_pool_uuid);
        std::string bp;
        if (pkg_pool_uuid != this_pool_uuid) { // pkg is open in RO mode from included pool
            if (pkg_pool)
                bp = pkg_pool->base_path;
            else
                throw std::runtime_error("pool " + (std::string)pkg_pool_uuid + " not found");
        }
        else {
            bp = pool.get_base_path();
        }
        const auto fn = Glib::build_filename(bp, model->filename);
        return std::make_pair(model->filename, fn);
    }
    else {
        return {};
    }
}


static Coordi coord_from_pt(const gp_Pnt &p)
{
    return Coordi(p.X() * 1e6, p.Y() * 1e6);
}

static void process_shapes(const TopoDS_Shape &sh, CanvasAnnotation *annotation,
                           std::vector<Coordi> &projection_targets)
{
    if (sh.IsNull())
        return;
    assert(sh.ShapeType() == TopAbs_COMPOUND);
    TopoDS_Iterator it;
    for (it.Initialize(sh, false, false); it.More(); it.Next()) {
        const TopoDS_Shape &subShape = it.Value();
        TopAbs_ShapeEnum stype = subShape.ShapeType();
        assert(stype == TopAbs_EDGE);
        auto edge = TopoDS::Edge(subShape);
        auto curve = BRepAdaptor_Curve(edge);
        auto curvetype = curve.GetType();

        {
            ShapeAnalysis_Edge sh_an;
            for (const auto vert : {sh_an.FirstVertex(edge), sh_an.LastVertex(edge)}) {
                const gp_Pnt pnt = BRep_Tool::Pnt(vert);
                projection_targets.emplace_back(coord_from_pt(pnt));
            }
        }

        {
            GCPnts_TangentialDeflection discretizer(curve, M_PI / 16, 1e3);
            if (discretizer.NbPoints() > 0) {
                int nbPoints = discretizer.NbPoints();
                for (int i = 2; i <= nbPoints; i++) {
                    const gp_Pnt pnt = discretizer.Value(i);
                    const gp_Pnt pnt2 = discretizer.Value(i - 1);
                    annotation->draw_line(coord_from_pt(pnt), coord_from_pt(pnt2), ColorP::PROJECTION, 0);
                }
            }
        }
        if (curvetype == GeomAbs_Circle) {
            const gp_Circ c = curve.Circle();
            const auto loc = c.Location();
            const auto size = c.Radius() * 1e6 / 20;
            const auto p = coord_from_pt(loc);
            projection_targets.emplace_back(p);
            annotation->draw_line(p + Coordi(-size, -size), p + Coordi(size, size), ColorP::PROJECTION, 0);
            annotation->draw_line(p + Coordi(-size, size), p + Coordi(size, -size), ColorP::PROJECTION, 0);
        }
    }
}

static glm::dvec3 transform_v(glm::dvec3 v, const Package::Model &model)
{
    v = glm::rotateX(v, angle_to_rad(model.roll));
    v = glm::rotateY(v, angle_to_rad(model.pitch));
    v = glm::rotateZ(v, angle_to_rad(model.yaw));
    return v;
}

static glm::dvec3 get_model_pos(const Package::Model &model)
{
    glm::dmat4 mat = glm::dmat4(1);
    mat = glm::rotate(mat, angle_to_rad(model.yaw), glm::dvec3(0, 0, 1));
    mat = glm::rotate(mat, angle_to_rad(model.pitch), glm::dvec3(0, 1, 0));
    mat = glm::rotate(mat, angle_to_rad(model.roll), glm::dvec3(1, 0, 0));
    auto v4 = mat * glm::dvec4(-model.x / 1e6, -model.y / 1e6, -model.z / 1e6, 1);
    return {v4.x, v4.y, v4.z};
}

void ImpPackage::project_model(const Package::Model &model, ProjectionMode proj)
{
    std::lock_guard<std::mutex> lock(model_info_mutex);
    auto &importer = *model_info.at(model.filename).importer.get();
    auto imp_shapes = importer.get_shapes();
    projection_annotation->clear();
    projection_targets.clear();
    for (auto &shape : imp_shapes) {
        Handle(HLRBRep_Algo) brep_hlr = new HLRBRep_Algo;
        brep_hlr->Add(shape);

        const auto vn = transform_v(glm::dvec3(0, 0, 1), model);
        const auto vx = transform_v(glm::dvec3(1, 0, 0), model);
        const auto v0 = get_model_pos(model);

        gp_Ax2 axis(gp_Pnt(v0.x, v0.y, v0.z), gp_Dir(vn.x, vn.y, vn.z), gp_Dir(vx.x, vx.y, vx.z));
        gp_Trsf transform;

        transform.SetTransformation(axis);
        if (proj == ProjectionMode::BOTTOM_UP) {
            /* flips handedness of projection, not possible in gp_Ax2 AFAICS */
            gp_Trsf reverse;
            reverse.SetValues(
                    /* clang-format off */
                    1.0,  0.0,  0.0,  0.0,
                    0.0,  1.0,  0.0,  0.0,
                    0.0,  0.0, -1.0,  0.0
                    /* clang-format on */
            );
            transform.PreMultiply(reverse);
        }

        HLRAlgo_Projector projector(transform, Standard_False, 0);
        /* reverse above can result in a scale factor of -1, which is ignored
         * by default...  but the rest of the matrix is still applied...
         */
        projector.Scaled(Standard_True);

        brep_hlr->Projector(projector);
        brep_hlr->Update();
        brep_hlr->Hide();

        HLRBRep_HLRToShape shapes(brep_hlr);
        process_shapes(shapes.VCompound(), projection_annotation, projection_targets);
        process_shapes(shapes.Rg1LineVCompound(), projection_annotation, projection_targets);
        process_shapes(shapes.OutLineVCompound(), projection_annotation, projection_targets);
    }
    show_projection_action->set_enabled(true);
    show_projection_action->change_state(Glib::Variant<bool>::create(true));
    canvas_update_from_pp();
    main_window->present();
}

std::pair<int64_t, int64_t> ImpPackage::get_height_from_model(const Package::Model &model)
{
    std::lock_guard<std::mutex> lock(model_info_mutex);
    auto mat = mat_from_model(model, 1e-6);
    double ht = 0;
    double hb = 0;

    auto &importer = *model_info.at(model.filename).importer.get();

    const auto result = importer.get_faces_and_points();
    for (const auto &face : result.faces) {
        for (const auto &v : face.vertices) {
            const auto vt = mat * glm::dvec4(v.x, v.y, v.z, 1);
            ht = std::max(ht, vt.z);
            hb = std::min(hb, vt.z);
        }
    }

    return {ht * 1_mm, -hb * 1_mm};
}


ImpPackage::ModelInfo::~ModelInfo()
{
}

} // namespace horizon
