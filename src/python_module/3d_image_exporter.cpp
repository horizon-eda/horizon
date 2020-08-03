#include "3d_image_exporter.hpp"
#include "export_3d_image/export_3d_image.hpp"
#include "nlohmann/json.hpp"
#include "util.hpp"
#define PYCAIRO_NO_IMPORT
#include <py3cairo.h>

static PyObject *PyImage3DExporter_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    PyImage3DExporter *self;
    self = (PyImage3DExporter *)type->tp_alloc(type, 0);
    if (self != NULL) {
        self->exporter = nullptr;
    }
    return (PyObject *)self;
}

static void PyImage3DExporter_dealloc(PyObject *pself)
{
    auto self = reinterpret_cast<PyImage3DExporter *>(pself);
    delete self->exporter;
    Py_TYPE(self)->tp_free((PyObject *)self);
}

static PyObject *PyImage3DExporter_render_to_png(PyObject *pself, PyObject *args)
{
    auto self = reinterpret_cast<PyImage3DExporter *>(pself);
    const char *filename = nullptr;
    if (!PyArg_ParseTuple(args, "s", &filename))
        return NULL;
    try {
        auto surf = self->exporter->render_to_surface();
        surf->write_to_png(filename);
    }
    catch (const std::exception &e) {
        PyErr_SetString(PyExc_IOError, e.what());
        return NULL;
    }
    catch (...) {
        PyErr_SetString(PyExc_IOError, "unknown exception");
        return NULL;
    }
    Py_RETURN_NONE;
}

static PyObject *PyImage3DExporter_render_to_surface(PyObject *pself, PyObject *args)
{
    auto self = reinterpret_cast<PyImage3DExporter *>(pself);
    try {
        cairo_surface_t *csurf = nullptr;
        {
            auto surf = self->exporter->render_to_surface();
            csurf = surf->cobj();
            cairo_surface_reference(csurf);
        }
        return PycairoSurface_FromSurface(csurf, NULL);
    }
    catch (const std::exception &e) {
        PyErr_SetString(PyExc_IOError, e.what());
        return NULL;
    }
    catch (...) {
        PyErr_SetString(PyExc_IOError, "unknown exception");
        return NULL;
    }
    Py_RETURN_NONE;
}

static PyObject *PyImage3DExporter_view_all(PyObject *pself, PyObject *args)
{
    auto self = reinterpret_cast<PyImage3DExporter *>(pself);
    self->exporter->view_all();
    Py_RETURN_NONE;
}

static PyObject *PyImage3DExporter_load_3d_models(PyObject *pself, PyObject *args)
{
    auto self = reinterpret_cast<PyImage3DExporter *>(pself);
    self->exporter->load_3d_models();
    Py_RETURN_NONE;
}

static PyMethodDef PyImage3DExporter_methods[] = {
        {"render_to_png", (PyCFunction)PyImage3DExporter_render_to_png, METH_VARARGS, "Render to PNG"},
        {"render_to_surface", (PyCFunction)PyImage3DExporter_render_to_surface, METH_NOARGS, "Render to cairo surface"},
        {"view_all", (PyCFunction)PyImage3DExporter_view_all, METH_NOARGS, "View all"},
        {"load_3d_models", (PyCFunction)PyImage3DExporter_load_3d_models, METH_NOARGS, "Load 3D models"},
        {NULL} /* Sentinel */
};

enum class BoolAttr : intptr_t {
    RENDER_BACKGROUND,
    SHOW_SILKSCREEN,
    SHOW_MODELS,
    SHOW_SOLDER_MASK,
    SHOW_SOLDER_PASTE,
    SHOW_SUBSTRATE,
    USE_LAYER_COLORS
};

static bool &get_bool_attr(PyObject *pself, void *pa)
{
    auto self = reinterpret_cast<PyImage3DExporter *>(pself);
    auto ex = self->exporter;
    auto a = static_cast<BoolAttr>(reinterpret_cast<intptr_t>(pa));
    switch (a) {
    case BoolAttr::RENDER_BACKGROUND:
        return ex->render_background;

    case BoolAttr::SHOW_SILKSCREEN:
        return ex->show_silkscreen;

    case BoolAttr::SHOW_MODELS:
        return ex->show_models;

    case BoolAttr::SHOW_SOLDER_MASK:
        return ex->show_solder_mask;

    case BoolAttr::SHOW_SOLDER_PASTE:
        return ex->show_solder_paste;

    case BoolAttr::SHOW_SUBSTRATE:
        return ex->show_substrate;

    case BoolAttr::USE_LAYER_COLORS:
        return ex->use_layer_colors;

    default:
        throw std::runtime_error("invalid attr");
    }
}

static PyObject *bool_attr_get(PyObject *pself, void *pa)
{
    return PyBool_FromLong(get_bool_attr(pself, pa));
}

static int bool_attr_set(PyObject *pself, PyObject *pval, void *pa)
{
    if (!pval) {
        PyErr_SetString(PyExc_AttributeError, "can't delete attr");
        return -1;
    }
    if (!PyBool_Check(pval)) {
        PyErr_SetString(PyExc_TypeError, "must be bool");
        return -1;
    }
    get_bool_attr(pself, pa) = (pval == Py_True);
    return 0;
}

enum class FloatAttr : intptr_t {
    CAM_AZIMUTH,
    CAM_ELEVATION,
    CAM_DISTANCE,
    CAM_FOV,
    CENTER_X,
    CENTER_Y,
};

static float &get_float_attr(PyObject *pself, void *pa)
{
    auto self = reinterpret_cast<PyImage3DExporter *>(pself);
    auto ex = self->exporter;
    auto a = static_cast<FloatAttr>(reinterpret_cast<intptr_t>(pa));
    switch (a) {
    case FloatAttr::CAM_AZIMUTH:
        return ex->cam_azimuth;

    case FloatAttr::CAM_ELEVATION:
        return ex->cam_elevation;

    case FloatAttr::CAM_DISTANCE:
        return ex->cam_distance;

    case FloatAttr::CAM_FOV:
        return ex->cam_fov;

    case FloatAttr::CENTER_X:
        return ex->center.x;

    case FloatAttr::CENTER_Y:
        return ex->center.y;

    default:
        throw std::runtime_error("invalid attr");
    }
}

static PyObject *float_attr_get(PyObject *pself, void *pa)
{
    return PyFloat_FromDouble(get_float_attr(pself, pa));
}

static int float_attr_set(PyObject *pself, PyObject *pval, void *pa)
{
    if (!pval) {
        PyErr_SetString(PyExc_AttributeError, "can't delete attr");
        return -1;
    }
    if (!PyNumber_Check(pval)) {
        PyErr_SetString(PyExc_TypeError, "must be number");
        return -1;
    }
    auto pfloat = PyNumber_Float(pval);
    if (!pfloat)
        return -1;
    get_float_attr(pself, pa) = PyFloat_AsDouble(pfloat);
    Py_DecRef(pfloat);
    return 0;
}

enum class ColorAttr : intptr_t { BACKGROUND_TOP, BACKGROUND_BOTTOM, SOLDER_MASK, SUBSTRATE };

static horizon::Color &get_color_attr(PyObject *pself, void *pa)
{
    auto self = reinterpret_cast<PyImage3DExporter *>(pself);
    auto ex = self->exporter;
    auto a = static_cast<ColorAttr>(reinterpret_cast<intptr_t>(pa));
    switch (a) {
    case ColorAttr::BACKGROUND_TOP:
        return ex->background_top_color;

    case ColorAttr::BACKGROUND_BOTTOM:
        return ex->background_bottom_color;

    case ColorAttr::SOLDER_MASK:
        return ex->solder_mask_color;

    case ColorAttr::SUBSTRATE:
        return ex->substrate_color;

    default:
        throw std::runtime_error("invalid attr");
    }
}

static PyObject *color_attr_get(PyObject *pself, void *pa)
{
    const auto &c = get_color_attr(pself, pa);
    return Py_BuildValue("(fff)", c.r, c.g, c.b);
}

static int color_attr_set(PyObject *pself, PyObject *pval, void *pa)
{
    if (!pval) {
        PyErr_SetString(PyExc_AttributeError, "can't delete attr");
        return -1;
    }
    if (!PySequence_Check(pval)) {
        PyErr_SetString(PyExc_TypeError, "must be sequence");
        return -1;
    }
    if (PySequence_Length(pval) != 3) {
        PyErr_SetString(PyExc_TypeError, "must be sequence of length 3");
        return -1;
    }
    auto &c = get_color_attr(pself, pa);
    for (size_t idx = 0; idx < 3; idx++) {
        auto elem = PySequence_GetItem(pval, idx);
        if (!elem)
            return -1;
        if (!PyNumber_Check(elem)) {
            Py_DecRef(elem);
            PyErr_SetString(PyExc_TypeError, "elem must be number");
            return -1;
        }
        auto pfloat = PyNumber_Float(elem);
        if (!pfloat) {
            Py_DecRef(elem);
            return -1;
        }
        float f = PyFloat_AsDouble(pfloat);
        switch (idx) {
        case 0:
            c.r = f;
            break;

        case 1:
            c.g = f;
            break;

        case 2:
            c.b = f;
            break;

        default:;
        }
        Py_DecRef(pfloat);
        Py_DecRef(elem);
    }
    return 0;
}

static PyObject *ortho_get(PyObject *pself, void *pa)
{
    auto self = reinterpret_cast<PyImage3DExporter *>(pself);
    return PyBool_FromLong(self->exporter->projection == horizon::Canvas3DBase::Projection::ORTHO);
}

static int ortho_set(PyObject *pself, PyObject *pval, void *pa)
{
    if (!pval) {
        PyErr_SetString(PyExc_AttributeError, "can't delete attr");
        return -1;
    }
    if (!PyBool_Check(pval)) {
        PyErr_SetString(PyExc_TypeError, "must be bool");
        return -1;
    }
    auto self = reinterpret_cast<PyImage3DExporter *>(pself);
    if (pval == Py_True) {
        self->exporter->projection = horizon::Canvas3DBase::Projection::ORTHO;
    }
    else {
        self->exporter->projection = horizon::Canvas3DBase::Projection::PERSP;
    }
    return 0;
}

static PyGetSetDef PyImage3DExporter_getset[] = {
        {"render_background", &bool_attr_get, &bool_attr_set, NULL,
         reinterpret_cast<void *>(BoolAttr::RENDER_BACKGROUND)},

        {"show_models", &bool_attr_get, &bool_attr_set, NULL, reinterpret_cast<void *>(BoolAttr::SHOW_MODELS)},

        {"show_silkscreen", &bool_attr_get, &bool_attr_set, NULL, reinterpret_cast<void *>(BoolAttr::SHOW_SILKSCREEN)},

        {"show_solder_mask", &bool_attr_get, &bool_attr_set, NULL,
         reinterpret_cast<void *>(BoolAttr::SHOW_SOLDER_MASK)},

        {"show_solder_paste", &bool_attr_get, &bool_attr_set, NULL,
         reinterpret_cast<void *>(BoolAttr::SHOW_SOLDER_PASTE)},

        {"show_substrate", &bool_attr_get, &bool_attr_set, NULL, reinterpret_cast<void *>(BoolAttr::SHOW_SUBSTRATE)},

        {"use_layer_colors", &bool_attr_get, &bool_attr_set, NULL,
         reinterpret_cast<void *>(BoolAttr::USE_LAYER_COLORS)},

        {"cam_azimuth", &float_attr_get, &float_attr_set, NULL, reinterpret_cast<void *>(FloatAttr::CAM_AZIMUTH)},

        {"cam_elevation", &float_attr_get, &float_attr_set, NULL, reinterpret_cast<void *>(FloatAttr::CAM_ELEVATION)},

        {"cam_distance", &float_attr_get, &float_attr_set, NULL, reinterpret_cast<void *>(FloatAttr::CAM_DISTANCE)},

        {"cam_fov", &float_attr_get, &float_attr_set, NULL, reinterpret_cast<void *>(FloatAttr::CAM_FOV)},

        {"center_x", &float_attr_get, &float_attr_set, NULL, reinterpret_cast<void *>(FloatAttr::CENTER_X)},
        {"center_y", &float_attr_get, &float_attr_set, NULL, reinterpret_cast<void *>(FloatAttr::CENTER_Y)},

        {"background_top_color", &color_attr_get, &color_attr_set, NULL,
         reinterpret_cast<void *>(ColorAttr::BACKGROUND_TOP)},

        {"background_bottom_color", &color_attr_get, &color_attr_set, NULL,
         reinterpret_cast<void *>(ColorAttr::BACKGROUND_BOTTOM)},

        {"solder_mask_color", &color_attr_get, &color_attr_set, NULL, reinterpret_cast<void *>(ColorAttr::SOLDER_MASK)},

        {"substrate_color", &color_attr_get, &color_attr_set, NULL, reinterpret_cast<void *>(ColorAttr::SUBSTRATE)},

        {"ortho", &ortho_get, &ortho_set, NULL, NULL},

        {NULL},
        /* Sentinel */};

PyTypeObject Image3DExporterType = [] {
    PyTypeObject r = {PyVarObject_HEAD_INIT(NULL, 0)};
    r.tp_name = "horizon.Image3DExporter";
    r.tp_basicsize = sizeof(PyImage3DExporter);

    r.tp_itemsize = 0;
    r.tp_dealloc = PyImage3DExporter_dealloc;
    r.tp_flags = Py_TPFLAGS_DEFAULT;
    r.tp_doc = "Image3DExporer";

    r.tp_methods = PyImage3DExporter_methods;
    r.tp_getset = PyImage3DExporter_getset;
    r.tp_new = PyImage3DExporter_new;
    return r;
}();
