#include "export_step.hpp"
#include "board/board.hpp"
#include "board/board_layers.hpp"
#include "canvas/canvas_patch.hpp"
#include "util/util.hpp"

#include <BRepBuilderAPI_MakeWire.hxx>
#include <TDocStd_Document.hxx>
#include <XCAFApp_Application.hxx>
#include <XCAFDoc_ShapeTool.hxx>
#include <TopoDS_Shape.hxx>
#include <TopoDS_Edge.hxx>
#include <Standard_Version.hxx>


#include <IGESCAFControl_Reader.hxx>
#include <IGESCAFControl_Writer.hxx>
#include <IGESControl_Controller.hxx>
#include <IGESData_GlobalSection.hxx>
#include <IGESData_IGESModel.hxx>
#include <Interface_Static.hxx>
#include <Quantity_Color.hxx>
#include <STEPCAFControl_Reader.hxx>
#include <STEPCAFControl_Writer.hxx>
#include <APIHeaderSection_MakeHeader.hxx>
#include <TCollection_ExtendedString.hxx>
#include <TDataStd_Name.hxx>
#include <TDF_LabelSequence.hxx>
#include <TDF_ChildIterator.hxx>
#include <TopExp_Explorer.hxx>
#include <XCAFDoc_DocumentTool.hxx>
#include <XCAFDoc_ColorTool.hxx>

#include <BRep_Tool.hxx>
#include <BRepMesh_IncrementalMesh.hxx>
#include <BRepBuilderAPI.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_Transform.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepPrimAPI_MakePrism.hxx>
#include <BRepPrimAPI_MakeCylinder.hxx>
#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepAlgoAPI_Cut.hxx>

#include <TopoDS.hxx>
#include <TopoDS_Wire.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Compound.hxx>
#include <TopoDS_Builder.hxx>

#include <gp_Ax2.hxx>
#include <gp_Circ.hxx>
#include <gp_Dir.hxx>
#include <gp_Pnt.hxx>

#include <glibmm/miscutils.h>

namespace horizon {

// adapted from https://github.com/KiCad/kicad-source-mirror/blob/master/utils/kicad2step/pcb/oce_utils.cpp

static TopoDS_Shape prism_from_countour(const ClipperLib::Path &contour, uint64_t thickness)
{
    BRepBuilderAPI_MakeWire wire;
    auto contour_sz = contour.size();
    for (size_t i = 0; i < contour_sz; i++) {
        auto pt1 = contour[i];
        auto pt2 = contour[(i + 1) % contour_sz];
        TopoDS_Edge edge;
        edge = BRepBuilderAPI_MakeEdge(gp_Pnt(pt1.X / 1e6, pt1.Y / 1e6, 0.0), gp_Pnt(pt2.X / 1e6, pt2.Y / 1e6, 0.0));
        wire.Add(edge);
    }
    TopoDS_Face face = BRepBuilderAPI_MakeFace(wire);
    return BRepPrimAPI_MakePrism(face, gp_Vec(0, 0, thickness / 1e6));
}

class CanvasHole : public Canvas {
public:
    CanvasHole(std::deque<TopoDS_Shape> &cs, int64_t th) : cutouts(cs), thickness(th)
    {
        img_mode = true;
    }

private:
    std::deque<TopoDS_Shape> &cutouts;
    int64_t thickness;
    void img_hole(const class Hole &hole) override
    {
        Placement tr = transform;
        tr.accumulate(hole.placement);
        if (hole.shape == Hole::Shape::ROUND) {
            TopoDS_Shape s = BRepPrimAPI_MakeCylinder(hole.diameter / 2e6, thickness * 2e-6).Shape();
            gp_Trsf shift;
            shift.SetTranslation(gp_Vec(tr.shift.x / 1e6, tr.shift.y / 1e6, -thickness / 2e6));
            BRepBuilderAPI_Transform t_hole(s, shift);
            cutouts.push_back(t_hole.Shape());
        }
        else if (hole.shape == Hole::Shape::SLOT) {
            int64_t box_width = hole.length - hole.diameter;
            if (box_width > 0) {
                TopoDS_Shape s = BRepPrimAPI_MakeBox(gp_Pnt(-box_width / 2e6, hole.diameter / -2e6, 0), box_width / 1e6,
                                                     hole.diameter / 1e6, thickness * 2e-6)
                                         .Shape();
                gp_Trsf trsf;
                trsf.SetTranslation(gp_Vec(tr.shift.x / 1e6, tr.shift.y / 1e6, -thickness / 2e6));
                gp_Trsf rot;
                if (tr.mirror)
                    rot.SetRotation(gp_Ax1(), -tr.get_angle_rad());
                else
                    rot.SetRotation(gp_Ax1(), tr.get_angle_rad());
                trsf.Multiply(rot);
                BRepBuilderAPI_Transform s_shift(s, trsf);
                cutouts.push_back(s_shift.Shape());
            }

            for (int mul : {1, -1}) {
                TopoDS_Shape s = BRepPrimAPI_MakeCylinder(hole.diameter / 2e6, thickness * 2e-6).Shape();
                gp_Trsf shift;
                auto hole_pos = tr.transform(Coordi(mul * box_width / 2, 0));
                shift.SetTranslation(gp_Vec(hole_pos.x / 1e6, hole_pos.y / 1e6, -thickness / 2e6));
                BRepBuilderAPI_Transform t_hole(s, shift);
                cutouts.push_back(t_hole.Shape());
            }
        }
    }

    void push() override
    {
    }
    void request_push() override
    {
    }
};

struct DOUBLET {
    double x;
    double y;

    DOUBLET() : x(0.0), y(0.0)
    {
        return;
    }
    DOUBLET(double aX, double aY) : x(aX), y(aY)
    {
        return;
    }
};

struct TRIPLET {
    double x;
    double y;

    union {
        double z;
        double angle;
    };

    TRIPLET() : x(0.0), y(0.0), z(0.0)
    {
        return;
    }
    TRIPLET(double aX, double aY, double aZ) : x(aX), y(aY), z(aZ)
    {
        return;
    }
};

#define BOARD_OFFSET (0.05)

static bool getModelLocation(bool aBottom, DOUBLET aPosition, double aRotation, TRIPLET aOffset, TRIPLET aOrientation,
                             TopLoc_Location &aLocation, double board_thickness)
{
    // Order of operations:
    // a. aOrientation is applied -Z*-Y*-X
    // b. aOffset is applied
    //      Top ? add thickness to the Z offset
    // c. Bottom ? Rotate on X axis (in contrast to most ECAD which mirror on Y),
    //             then rotate on +Z
    //    Top ? rotate on -Z
    // d. aPosition is applied
    //
    // Note: Y axis is inverted in KiCad

    gp_Trsf lPos;
    lPos.SetTranslation(gp_Vec(aPosition.x, -aPosition.y, 0.0));

    // Offset board thickness
    aOffset.z += BOARD_OFFSET;

    gp_Trsf lRot;

    if (aBottom) {
        lRot.SetRotation(gp_Ax1(gp_Pnt(0.0, 0.0, 0.0), gp_Dir(0.0, 0.0, 1.0)), aRotation + M_PI);
        lPos.Multiply(lRot);
        lRot.SetRotation(gp_Ax1(gp_Pnt(0.0, 0.0, 0.0), gp_Dir(1.0, 0.0, 0.0)), M_PI);
        lPos.Multiply(lRot);
    }
    else {
        aOffset.z += board_thickness;
        lRot.SetRotation(gp_Ax1(gp_Pnt(0.0, 0.0, 0.0), gp_Dir(0.0, 0.0, 1.0)), aRotation);
        lPos.Multiply(lRot);
    }

    gp_Trsf lOff;
    lOff.SetTranslation(gp_Vec(aOffset.x, aOffset.y, aOffset.z));
    lPos.Multiply(lOff);

    gp_Trsf lOrient;
    lOrient.SetRotation(gp_Ax1(gp_Pnt(0.0, 0.0, 0.0), gp_Dir(1.0, 0.0, 0.0)), -aOrientation.x);
    lPos.Multiply(lOrient);
    lOrient.SetRotation(gp_Ax1(gp_Pnt(0.0, 0.0, 0.0), gp_Dir(0.0, 1.0, 0.0)), -aOrientation.y);
    lPos.Multiply(lOrient);
    lOrient.SetRotation(gp_Ax1(gp_Pnt(0.0, 0.0, 0.0), gp_Dir(0.0, 0.0, 1.0)), -aOrientation.z);
    lPos.Multiply(lOrient);

    aLocation = TopLoc_Location(lPos);
    return true;
}

#define USER_PREC (1e-4)

static bool readSTEP(Handle(TDocStd_Document) & doc, const char *fname)
{
    STEPCAFControl_Reader reader;
    IFSelect_ReturnStatus stat = reader.ReadFile(fname);

    if (stat != IFSelect_RetDone)
        return false;

    // Enable user-defined shape precision
    if (!Interface_Static::SetIVal("read.precision.mode", 1))
        return false;

    // Set the shape conversion precision to USER_PREC (default 0.0001 has too many triangles)
    if (!Interface_Static::SetRVal("read.precision.val", USER_PREC))
        return false;

    // set other translation options
    reader.SetColorMode(true);  // use model colors
    reader.SetNameMode(false);  // don't use label names
    reader.SetLayerMode(false); // ignore LAYER data

    if (!reader.Transfer(doc)) {
        doc->Close();
        return false;
    }

    // are there any shapes to translate?
    if (reader.NbRootsForTransfer() < 1) {
        doc->Close();
        return false;
    }

    return true;
}

static TDF_Label transferModel(Handle(TDocStd_Document) & source, Handle(TDocStd_Document) & dest,
                               const std::string &name)
{
    // transfer data from Source into a top level component of Dest

    // s_assy = shape tool for the source
    Handle(XCAFDoc_ShapeTool) s_assy = XCAFDoc_DocumentTool::ShapeTool(source->Main());

    // retrieve all free shapes within the assembly
    TDF_LabelSequence frshapes;
    s_assy->GetFreeShapes(frshapes);

    // d_assy = shape tool for the destination
    Handle(XCAFDoc_ShapeTool) d_assy = XCAFDoc_DocumentTool::ShapeTool(dest->Main());

    // create a new shape within the destination and set the assembly tool to point to it
    TDF_Label component = d_assy->NewShape();

    int nshapes = frshapes.Length();
    int id = 1;
    Handle(XCAFDoc_ColorTool) scolor = XCAFDoc_DocumentTool::ColorTool(source->Main());
    Handle(XCAFDoc_ColorTool) dcolor = XCAFDoc_DocumentTool::ColorTool(dest->Main());
    TopExp_Explorer dtop;
    TopExp_Explorer stop;

    while (id <= nshapes) {
        TopoDS_Shape shape = s_assy->GetShape(frshapes.Value(id));

        if (!shape.IsNull()) {
            TDF_Label la = d_assy->AddShape(shape, false);
            TDF_Label niulab = d_assy->AddComponent(component, la, shape.Location());
            TDataStd_Name::Set(la, (name + "-" + std::to_string(id)).c_str());

            // check for per-surface colors
            stop.Init(shape, TopAbs_FACE);
            dtop.Init(d_assy->GetShape(niulab), TopAbs_FACE);

            while (stop.More() && dtop.More()) {
                Quantity_Color face_color;

                TDF_Label tl;

                // give priority to the base shape's color
                if (s_assy->FindShape(stop.Current(), tl)) {
                    if (scolor->GetColor(tl, XCAFDoc_ColorSurf, face_color)
                        || scolor->GetColor(tl, XCAFDoc_ColorGen, face_color)
                        || scolor->GetColor(tl, XCAFDoc_ColorCurv, face_color)) {
                        dcolor->SetColor(dtop.Current(), face_color, XCAFDoc_ColorSurf);
                    }
                }
                else if (scolor->GetColor(stop.Current(), XCAFDoc_ColorSurf, face_color)
                         || scolor->GetColor(stop.Current(), XCAFDoc_ColorGen, face_color)
                         || scolor->GetColor(stop.Current(), XCAFDoc_ColorCurv, face_color)) {

                    dcolor->SetColor(dtop.Current(), face_color, XCAFDoc_ColorSurf);
                }

                stop.Next();
                dtop.Next();
            }

            // check for per-solid colors
            stop.Init(shape, TopAbs_SOLID);
            dtop.Init(d_assy->GetShape(niulab), TopAbs_SOLID, TopAbs_FACE);

            while (stop.More() && dtop.More()) {
                Quantity_Color face_color;

                TDF_Label tl;

                // give priority to the base shape's color
                if (s_assy->FindShape(stop.Current(), tl)) {
                    if (scolor->GetColor(tl, XCAFDoc_ColorSurf, face_color)
                        || scolor->GetColor(tl, XCAFDoc_ColorGen, face_color)
                        || scolor->GetColor(tl, XCAFDoc_ColorCurv, face_color)) {
                        dcolor->SetColor(dtop.Current(), face_color, XCAFDoc_ColorGen);
                    }
                }
                else if (scolor->GetColor(stop.Current(), XCAFDoc_ColorSurf, face_color)
                         || scolor->GetColor(stop.Current(), XCAFDoc_ColorGen, face_color)
                         || scolor->GetColor(stop.Current(), XCAFDoc_ColorCurv, face_color)) {
                    dcolor->SetColor(dtop.Current(), face_color, XCAFDoc_ColorSurf);
                }

                stop.Next();
                dtop.Next();
            }
        }

        ++id;
    };

    return component;
}

static bool getModelLabel(const std::string &aFileName, TDF_Label &aLabel, Handle(XCAFApp_Application) app,
                          Handle(TDocStd_Document) doc, const std::string &name)
{
    Handle(TDocStd_Document) my_doc;
    app->NewDocument("MDTV-XCAF", my_doc);
    if (!readSTEP(my_doc, aFileName.c_str())) {
        throw std::runtime_error("error loading step");
    }

    aLabel = transferModel(my_doc, doc, name);

    TCollection_ExtendedString partname(name.c_str());
    TDataStd_Name::Set(aLabel, partname);

    return !aLabel.IsNull();
}

static double angle_to_rad(int angle)
{
    return (angle / 32768.) * M_PI;
}

void export_step(const std::string &filename, const Board &brd, class Pool &pool, bool include_models,
                 std::function<void(const std::string &)> progress_cb, const BoardColors *colors,
                 const std::string &prefix)
{
    auto app = XCAFApp_Application::GetApplication();
    Handle(TDocStd_Document) doc;
    app->NewDocument("MDTV-XCAF", doc);
    XCAFDoc_ShapeTool::SetAutoNaming(false);
    BRepBuilderAPI::Precision(1.0e-6);
    auto assy = XCAFDoc_DocumentTool::ShapeTool(doc->Main());
    auto assy_label = assy->NewShape();
    TDataStd_Name::Set(assy_label, (prefix + "PCA").c_str());

    progress_cb("Board outline...");
    ClipperLib::Clipper cl;
    {
        CanvasPatch canvas;
        canvas.update(brd);
        for (const auto &it : canvas.get_patches()) {
            if (it.first.layer == BoardLayers::L_OUTLINE) {
                cl.AddPaths(it.second, ClipperLib::ptSubject, true);
            }
        }
    }
    ClipperLib::PolyTree result;
    cl.Execute(ClipperLib::ctUnion, result, ClipperLib::pftEvenOdd);

    if (result.ChildCount() != 1) {
        throw std::runtime_error("invalid board outline");
    }

    auto outline = result.Childs.front()->Contour;
    if (outline.size() < 3) {
        throw std::runtime_error("outline has less than 3 vertices");
    }

    int64_t total_thickness = 0;
    for (const auto &it : brd.stackup) {
        if (it.second.layer != BoardLayers::BOTTOM_COPPER) {
            total_thickness += it.second.substrate_thickness;
        }
    }

    progress_cb("Board cutouts...");
    std::deque<TopoDS_Shape> cutouts;
    for (const auto &hole_node : result.Childs.front()->Childs) {
        auto hole_outline = hole_node->Contour;
        cutouts.push_back(prism_from_countour(hole_outline, total_thickness));
    }

    progress_cb("Holes...");
    {
        CanvasHole canvas_hole(cutouts, total_thickness);
        canvas_hole.update(brd);
    }

    progress_cb("Creating board...");
    TopoDS_Shape board = prism_from_countour(outline, total_thickness);
    for (const auto &it : cutouts) {
        board = BRepAlgoAPI_Cut(board, it);
    }

    TDF_Label board_label = assy->AddShape(board, false);
    assy->AddComponent(assy_label, board_label, board.Location());
    TDataStd_Name::Set(board_label, (prefix + "PCB").c_str());

    if (!board_label.IsNull()) {
        Handle(XCAFDoc_ColorTool) color = XCAFDoc_DocumentTool::ColorTool(doc->Main());
        Color c;
        if (colors) {
            c = colors->solder_mask;
        }
        else {
            c = brd.colors.solder_mask;
        }
        Quantity_Color pcb_color(c.r, c.g, c.b, Quantity_TOC_RGB);
        color->SetColor(board_label, pcb_color, XCAFDoc_ColorSurf);
        TopExp_Explorer topex;
        topex.Init(assy->GetShape(board_label), TopAbs_SOLID);

        while (topex.More()) {
            color->SetColor(topex.Current(), pcb_color, XCAFDoc_ColorSurf);
            topex.Next();
        }
    }

    if (include_models) {
        progress_cb("Packages...");
        auto n_pkg = brd.packages.size();
        size_t i = 1;
        std::vector<const BoardPackage *> pkgs;
        pkgs.reserve(brd.packages.size());
        for (const auto &it : brd.packages) {
            pkgs.push_back(&it.second);
        }
        std::sort(pkgs.begin(), pkgs.end(),
                  [](auto a, auto b) { return strcmp_natural(a->component->refdes, b->component->refdes) < 0; });

        for (const auto it : pkgs) {
            try {
                auto model = it->package.get_model(it->model);
                if (model) {
                    progress_cb("Package " + it->component->refdes + " (" + std::to_string(i) + "/"
                                + std::to_string(n_pkg) + ")");
                    TDF_Label lmodel;

                    if (!getModelLabel(pool.get_model_filename(it->package.uuid, model->uuid), lmodel, app, doc,
                                       prefix + it->component->refdes)) {
                        throw std::runtime_error("get model label");
                    }

                    TopLoc_Location toploc;
                    DOUBLET pos(it->placement.shift.x / 1e6, it->placement.shift.y / -1e6);
                    double rot = angle_to_rad(it->placement.get_angle());
                    TRIPLET offset(model->x / 1e6, model->y / 1e6, model->z / 1e6);
                    TRIPLET orientation(angle_to_rad(model->roll), angle_to_rad(model->pitch),
                                        angle_to_rad(model->yaw));
                    getModelLocation(it->flip, pos, rot, offset, orientation, toploc, total_thickness / 1e6);

                    assy->AddComponent(assy_label, lmodel, toploc);

                    TCollection_ExtendedString refdes((prefix + it->component->refdes).c_str());
                    TDataStd_Name::Set(lmodel, refdes);
                }
            }
            catch (const std::exception &e) {
                progress_cb("Error processing package " + it->component->refdes + ": " + e.what());
            }
            i++;
        }
    }
    progress_cb("Writing output file");
#if OCC_VERSION_MAJOR >= 7 && OCC_VERSION_MINOR >= 2
    assy->UpdateAssemblies();
#endif
    STEPCAFControl_Writer writer;
    writer.SetColorMode(Standard_True);
    writer.SetNameMode(Standard_True);
    if (Standard_False == writer.Transfer(doc, STEPControl_AsIs)) {
        throw std::runtime_error("transfer error");
    }

    APIHeaderSection_MakeHeader hdr(writer.ChangeWriter().Model());
    hdr.SetName(new TCollection_HAsciiString("Board"));
    hdr.SetAuthorValue(1, new TCollection_HAsciiString("An Author"));
    hdr.SetOrganizationValue(1, new TCollection_HAsciiString("A Company"));
    hdr.SetOriginatingSystem(new TCollection_HAsciiString("horizon EDA"));
    hdr.SetDescriptionValue(1, new TCollection_HAsciiString("Electronic assembly"));

    if (Standard_False == writer.Write(filename.c_str()))
        throw std::runtime_error("write error");

    progress_cb("Done");
}
} // namespace horizon
