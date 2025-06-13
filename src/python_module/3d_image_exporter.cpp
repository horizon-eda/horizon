#include "3d_image_exporter.hpp"
#include "image_3d_exporter_wrapper.hpp"
#include <nlohmann/json.hpp>
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

using Color = horizon::Color;

static PyObject *ortho_get(PyObject *pself, void *pa)
{
    auto self = reinterpret_cast<PyImage3DExporter *>(pself);
    return PyBool_FromLong(self->exporter->get_projection() == horizon::Canvas3DBase::Projection::ORTHO);
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
        self->exporter->set_projection(horizon::Canvas3DBase::Projection::ORTHO);
    }
    else {
        self->exporter->set_projection(horizon::Canvas3DBase::Projection::PERSP);
    }
    return 0;
}


template <typename T> struct FnGetSet {
    using Set = void (horizon::Image3DExporterWrapper::*)(const T &v);
    using Get = const T &(horizon::Image3DExporterWrapper::*)() const;
    Get get;
    Set set;
};

#define GET_SET(t_, x_)                                                                                                \
    static FnGetSet<t_> get_set_##x_{                                                                                  \
            &horizon::Image3DExporterWrapper::get_##x_,                                                                \
            &horizon::Image3DExporterWrapper::set_##x_,                                                                \
    };


// clang-format off
#define ATTRS                                                                                                          \
    X(bool, render_background)                                                                                         \
    X(bool, show_models)                                                                                               \
    X(bool, show_dnp_models)                                                                                           \
    X(bool, show_silkscreen)                                                                                           \
    X(bool, show_solder_mask)                                                                                          \
    X(bool, show_solder_paste)                                                                                         \
    X(bool, show_substrate)                                                                                            \
    X(bool, use_layer_colors)                                                                                          \
    X(bool, show_copper)                                                                                               \
    X(float, cam_azimuth)                                                                                              \
    X(float, cam_elevation)                                                                                            \
    X(float, cam_distance)                                                                                             \
    X(float, cam_fov)                                                                                                  \
    X(float, center_x)                                                                                                 \
    X(float, center_y)                                                                                                 \
    X(Color, background_top_color)                                                                                     \
    X(Color, background_bottom_color)                                                                                  \
    X(Color, solder_mask_color)                                                                                        \
    X(Color, silkscreen_color)                                                                                         \
    X(Color, substrate_color)
// clang-format on


#define X GET_SET
ATTRS
#undef X

static PyObject *bool_attr_get(PyObject *pself, void *pa)
{
    auto self = reinterpret_cast<PyImage3DExporter *>(pself);
    auto getset = static_cast<FnGetSet<bool> *>(pa);
    return PyBool_FromLong(std::invoke(getset->get, self->exporter));
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
    auto self = reinterpret_cast<PyImage3DExporter *>(pself);
    auto getset = static_cast<FnGetSet<bool> *>(pa);
    std::invoke(getset->set, self->exporter, pval == Py_True);
    return 0;
}

static PyObject *float_attr_get(PyObject *pself, void *pa)
{
    auto self = reinterpret_cast<PyImage3DExporter *>(pself);
    auto getset = static_cast<FnGetSet<float> *>(pa);
    return PyFloat_FromDouble(std::invoke(getset->get, self->exporter));
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
    auto self = reinterpret_cast<PyImage3DExporter *>(pself);
    auto getset = static_cast<FnGetSet<float> *>(pa);
    std::invoke(getset->set, self->exporter, PyFloat_AsDouble(pfloat));
    Py_DecRef(pfloat);
    return 0;
}


static PyObject *Color_attr_get(PyObject *pself, void *pa)
{
    auto self = reinterpret_cast<PyImage3DExporter *>(pself);
    auto getset = static_cast<FnGetSet<Color> *>(pa);
    auto c = std::invoke(getset->get, self->exporter);
    return Py_BuildValue("(fff)", c.r, c.g, c.b);
}

static int Color_attr_set(PyObject *pself, PyObject *pval, void *pa)
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
    Color c;
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
    auto self = reinterpret_cast<PyImage3DExporter *>(pself);
    auto getset = static_cast<FnGetSet<Color> *>(pa);
    std::invoke(getset->set, self->exporter, c);
    return 0;
}

static PyGetSetDef PyImage3DExporter_getset[] = {
#define X(t_, x_) {#x_, &t_##_attr_get, &t_##_attr_set, NULL, static_cast<void *>(&get_set_##x_)},
        ATTRS
#undef X
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
