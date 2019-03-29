#include "step_importer.hpp"
#include <Quantity_Color.hxx>
#include <TDocStd_Document.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Shape.hxx>
#include <XCAFApp_Application.hxx>

#include <AIS_Shape.hxx>

#include <IGESCAFControl_Reader.hxx>
#include <IGESControl_Reader.hxx>
#include <Interface_Static.hxx>

#include <STEPCAFControl_Reader.hxx>
#include <STEPControl_Reader.hxx>

#include <XCAFDoc_ColorTool.hxx>
#include <XCAFDoc_DocumentTool.hxx>
#include <XCAFDoc_ShapeTool.hxx>

#include <BRepMesh_IncrementalMesh.hxx>
#include <BRep_Tool.hxx>

#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Compound.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Shape.hxx>

#include <Poly_PolygonOnTriangulation.hxx>
#include <Poly_Triangulation.hxx>
#include <Precision.hxx>
#include <Quantity_Color.hxx>

#include <TDF_ChildIterator.hxx>
#include <TDF_LabelSequence.hxx>

#define USER_PREC (0.14)
#define USER_ANGLE (0.52359878)

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/transform.hpp>

namespace STEPImporter {

// adapted from
// https://github.com/KiCad/kicad-source-mirror/blob/master/plugins/3d/oce/loadmodel.cpp
struct DATA;

static bool processNode(const TopoDS_Shape &shape, DATA &data);

static bool processComp(const TopoDS_Shape &shape, DATA &data, const glm::mat4 &mat_in = glm::mat4(1));

struct DATA {
    Handle(TDocStd_Document) m_doc;
    Handle(XCAFDoc_ColorTool) m_color;
    Handle(XCAFDoc_ShapeTool) m_assy;
    bool hasSolid;

    std::deque<Face> *faces;
};

static bool readSTEP(Handle(TDocStd_Document) & m_doc, const char *fname)
{
    STEPCAFControl_Reader reader;
    IFSelect_ReturnStatus stat = reader.ReadFile(fname);

    if (stat != IFSelect_RetDone)
        return false;

    // Enable user-defined shape precision
    if (!Interface_Static::SetIVal("read.precision.mode", 1))
        return false;

    // Set the shape conversion precision to USER_PREC (default 0.0001 has too
    // many triangles)
    if (!Interface_Static::SetRVal("read.precision.val", USER_PREC))
        return false;

    // set other translation options
    reader.SetColorMode(true);  // use model colors
    reader.SetNameMode(false);  // don't use label names
    reader.SetLayerMode(false); // ignore LAYER data

    if (!reader.Transfer(m_doc)) {
        m_doc->Close();
        return false;
    }

    // are there any shapes to translate?
    if (reader.NbRootsForTransfer() < 1)
        return false;

    return true;
}

static bool getColor(DATA &data, TDF_Label label, Quantity_Color &color)
{
    while (true) {
        if (data.m_color->GetColor(label, XCAFDoc_ColorGen, color))
            return true;
        else if (data.m_color->GetColor(label, XCAFDoc_ColorSurf, color))
            return true;
        else if (data.m_color->GetColor(label, XCAFDoc_ColorCurv, color))
            return true;

        label = label.Father();

        if (label.IsNull())
            break;
    };

    return false;
}

static bool processFace(const TopoDS_Face &face, DATA &data, Quantity_Color *color, const glm::mat4 &mat = glm::mat4(1))
{
    if (Standard_True == face.IsNull())
        return false;

    // bool reverse = ( face.Orientation() == TopAbs_REVERSED );

    //	bool useBothSides = false;

    TopLoc_Location loc;
    Standard_Boolean isTessellate(Standard_False);
    Handle(Poly_Triangulation) triangulation = BRep_Tool::Triangulation(face, loc);

    if (triangulation.IsNull() || triangulation->Deflection() > USER_PREC + Precision::Confusion())
        isTessellate = Standard_True;

    if (isTessellate) {
        BRepMesh_IncrementalMesh IM(face, USER_PREC, Standard_False, USER_ANGLE);
        triangulation = BRep_Tool::Triangulation(face, loc);
    }

    if (triangulation.IsNull() == Standard_True)
        return false;

    Quantity_Color lcolor;

    // check for a face color; this has precedence over SOLID colors
    do {
        TDF_Label L;

        if (data.m_color->ShapeTool()->Search(face, L)) {
            if (data.m_color->GetColor(L, XCAFDoc_ColorGen, lcolor)
                || data.m_color->GetColor(L, XCAFDoc_ColorCurv, lcolor)
                || data.m_color->GetColor(L, XCAFDoc_ColorSurf, lcolor))
                color = &lcolor;
        }
    } while (0);

    const TColgp_Array1OfPnt &arrPolyNodes = triangulation->Nodes();
    const Poly_Array1OfTriangle &arrTriangles = triangulation->Triangles();

    data.faces->emplace_back();
    auto &face_out = data.faces->back();
    if (color) {
        face_out.color.r = color->Red();
        face_out.color.g = color->Green();
        face_out.color.b = color->Blue();
    }
    else {
        face_out.color = {0.5, 0.5, 0.5};
    }
    face_out.vertices.reserve(triangulation->NbNodes());

    for (int i = 1; i <= triangulation->NbNodes(); i++) {
        gp_XYZ v(arrPolyNodes(i).Coord());
        glm::vec4 vg(v.X(), v.Y(), v.Z(), 1);
        auto vt = mat * vg;
        face_out.vertices.emplace_back(vt.x, vt.y, vt.z);
    }

    face_out.triangle_indices.reserve(triangulation->NbTriangles());
    for (int i = 1; i <= triangulation->NbTriangles(); i++) {
        int a, b, c;
        arrTriangles(i).Get(a, b, c);
        face_out.triangle_indices.emplace_back(a - 1, b - 1, c - 1);
    }

    return true;
}

static bool processShell(const TopoDS_Shape &shape, DATA &data, Quantity_Color *color,
                         const glm::mat4 &mat = glm::mat4(1))
{
    TopoDS_Iterator it;
    bool ret = false;

    for (it.Initialize(shape, false, false); it.More(); it.Next()) {
        const TopoDS_Face &face = TopoDS::Face(it.Value());

        if (processFace(face, data, color, mat))
            ret = true;
    }

    return ret;
}

static bool processSolid(const TopoDS_Shape &shape, DATA &data, const glm::mat4 &mat_in = glm::mat4(1))
{
    TDF_Label label = data.m_assy->FindShape(shape, Standard_False);
    bool ret = false;

    data.hasSolid = true;

    Quantity_Color col;
    Quantity_Color *lcolor = NULL;

    if (label.IsNull()) {
    }
    else {

        if (getColor(data, label, col))
            lcolor = &col;
    }

    TopLoc_Location loc = shape.Location();
    gp_Trsf T = loc.Transformation();
    gp_XYZ coord = T.TranslationPart();

    auto mat = mat_in * glm::translate(glm::vec3(coord.X(), coord.Y(), coord.Z()));

    gp_XYZ axis;
    Standard_Real angle;

    if (T.GetRotation(axis, angle)) {
        glm::vec3 gaxis(axis.X(), axis.Y(), axis.Z());
        float angle_f = angle;
        mat = glm::rotate(mat, angle_f, gaxis);
    }

    TopoDS_Iterator it;
    for (it.Initialize(shape, false, false); it.More(); it.Next()) {
        const TopoDS_Shape &subShape = it.Value();

        if (processShell(subShape, data, lcolor, mat))
            ret = true;
    }

    return ret;
}

static bool processComp(const TopoDS_Shape &shape, DATA &data, const glm::mat4 &mat_in)
{
    TopoDS_Iterator it;
    TopLoc_Location loc = shape.Location();
    gp_Trsf T = loc.Transformation();
    gp_XYZ coord = T.TranslationPart();

    auto mat = mat_in * glm::translate(glm::vec3(coord.X(), coord.Y(), coord.Z()));

    gp_XYZ axis;
    Standard_Real angle;

    if (T.GetRotation(axis, angle)) {
        glm::vec3 gaxis(axis.X(), axis.Y(), axis.Z());
        float angle_f = angle;
        mat = glm::rotate(mat, angle_f, gaxis);
    }

    bool ret = false;

    for (it.Initialize(shape, false, false); it.More(); it.Next()) {
        const TopoDS_Shape &subShape = it.Value();
        TopAbs_ShapeEnum stype = subShape.ShapeType();

        data.hasSolid = false;

        switch (stype) {
        case TopAbs_COMPOUND:
        case TopAbs_COMPSOLID:
            if (processComp(subShape, data, mat))
                ret = true;
            break;

        case TopAbs_SOLID:
            if (processSolid(subShape, data, mat))
                ret = true;
            break;

        case TopAbs_SHELL:
            if (processShell(subShape, data, NULL))
                ret = true;
            break;

        case TopAbs_FACE:
            if (processFace(TopoDS::Face(subShape), data, NULL))
                ret = true;
            break;

        default:
            break;
        }
    }

    return ret;
}

static bool processNode(const TopoDS_Shape &shape, DATA &data)
{
    TopAbs_ShapeEnum stype = shape.ShapeType();
    bool ret = true;
    switch (stype) {
    case TopAbs_COMPOUND:
    case TopAbs_COMPSOLID:
        if (processComp(shape, data))
            ret = true;
        break;

    case TopAbs_SOLID:
        if (processSolid(shape, data))
            ret = true;
        break;

    case TopAbs_SHELL:
        if (processShell(shape, data, NULL))
            ret = true;
        break;

    case TopAbs_FACE:
        if (processFace(TopoDS::Face(shape), data, NULL))
            ret = true;
        break;

    default:
        break;
    }

    return ret;
}

std::deque<Face> import(const std::string &filename)
{
    std::deque<Face> faces;
    DATA data;
    data.faces = &faces;
    Handle(XCAFApp_Application) m_app = XCAFApp_Application::GetApplication();

    m_app->NewDocument("MDTV-XCAF", data.m_doc);
    if (!readSTEP(data.m_doc, filename.c_str())) {
        std::cout << "error loading " << filename << std::endl;
        return faces;
    }
    std::cout << "loaded" << std::endl;

    data.m_assy = XCAFDoc_DocumentTool::ShapeTool(data.m_doc->Main());
    data.m_color = XCAFDoc_DocumentTool::ColorTool(data.m_doc->Main());
    TDF_LabelSequence frshapes;
    data.m_assy->GetFreeShapes(frshapes);

    int nshapes = frshapes.Length();
    int id = 1;
    std::cout << "shapes " << nshapes << std::endl;
    bool ret = false;
    while (id <= nshapes) {
        TopoDS_Shape shape = data.m_assy->GetShape(frshapes.Value(id));
        if (!shape.IsNull() && processNode(shape, data)) {
            ret = true;
        }
        ++id;
    }

    return faces;
}
} // namespace STEPImporter
