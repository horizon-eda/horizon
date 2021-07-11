#include "schematic.hpp"
#include "project/project.hpp"
#include "block/block.hpp"
#include "schematic/schematic.hpp"
#include "nlohmann/json.hpp"
#include "export_pdf/export_pdf.hpp"
#include "export_bom/export_bom.hpp"
#include "util.hpp"
#include <podofo/podofo.h>

SchematicWrapper::SchematicWrapper(const horizon::Project &prj)
    : pool(prj.pool_directory, false), blocks(horizon::BlocksSchematic::new_from_file(prj.blocks_filename, pool))
{
    auto &top = blocks.get_top_block();
    top.block.create_instance_mappings();
    top.schematic.update_sheet_mapping();
    for (auto &[uu, block] : blocks.blocks) {
        if (uu == top.uuid)
            continue;

        top.block.update_non_top(block.block);
    }
    for (auto &[uu, block] : blocks.blocks) {
        block.symbol.expand();
    }
    for (auto &[uu, block] : blocks.blocks) {
        block.schematic.expand();
    }
}


static PyObject *PySchematic_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    PySchematic *self;
    self = (PySchematic *)type->tp_alloc(type, 0);
    if (self != NULL) {
        self->schematic = nullptr;
    }
    return (PyObject *)self;
}

static void PySchematic_dealloc(PyObject *pself)
{
    auto self = reinterpret_cast<PySchematic *>(pself);
    delete self->schematic;
    Py_TYPE(self)->tp_free((PyObject *)self);
}


static PyObject *PySchematic_get_pdf_export_settings(PyObject *pself)
{
    auto self = reinterpret_cast<PySchematic *>(pself);
    auto settings = self->schematic->blocks.get_top_block().schematic.pdf_export_settings.serialize_schematic();
    return py_from_json(settings);
}

static PyObject *PySchematic_export_pdf(PyObject *pself, PyObject *args)
{
    auto self = reinterpret_cast<PySchematic *>(pself);
    PyObject *py_export_settings = nullptr;
    if (!PyArg_ParseTuple(args, "O!", &PyDict_Type, &py_export_settings))
        return NULL;
    try {
        auto settings_json = json_from_py(py_export_settings);
        horizon::PDFExportSettings settings(settings_json);
        horizon::export_pdf(self->schematic->blocks.get_top_block().schematic, settings, nullptr);
    }
    catch (const std::exception &e) {
        PyErr_SetString(PyExc_IOError, e.what());
        return NULL;
    }
    catch (const PoDoFo::PdfError &e) {
        PyErr_SetString(PyExc_IOError, e.what());
        return NULL;
    }
    catch (...) {
        PyErr_SetString(PyExc_IOError, "unknown exception");
        return NULL;
    }
    Py_RETURN_NONE;
}


static PyObject *PySchematic_get_bom_export_settings(PyObject *pself)
{
    auto self = reinterpret_cast<PySchematic *>(pself);
    auto settings = self->schematic->blocks.get_top_block().block.bom_export_settings.serialize();
    return py_from_json(settings);
}

static PyObject *PySchematic_export_bom(PyObject *pself, PyObject *args)
{
    auto self = reinterpret_cast<PySchematic *>(pself);
    PyObject *py_export_settings = nullptr;
    if (!PyArg_ParseTuple(args, "O!", &PyDict_Type, &py_export_settings))
        return NULL;
    try {
        auto settings_json = json_from_py(py_export_settings);
        horizon::BOMExportSettings settings(settings_json, self->schematic->pool);
        horizon::export_BOM(settings.output_filename, self->schematic->blocks.get_top_block().block, settings);
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


static PyMethodDef PySchematic_methods[] = {
        {"get_pdf_export_settings", (PyCFunction)PySchematic_get_pdf_export_settings, METH_NOARGS,
         "Return pdf export settings"},
        {"get_bom_export_settings", (PyCFunction)PySchematic_get_bom_export_settings, METH_NOARGS,
         "Return bom export settings"},
        {"export_pdf", (PyCFunction)PySchematic_export_pdf, METH_VARARGS, "Export as pdf"},
        {"export_bom", (PyCFunction)PySchematic_export_bom, METH_VARARGS, "Export BOM"},
        {NULL} /* Sentinel */
};

PyTypeObject SchematicType = [] {
    PyTypeObject r = {PyVarObject_HEAD_INIT(NULL, 0)};
    r.tp_name = "horizon.Schematic";
    r.tp_basicsize = sizeof(PySchematic);

    r.tp_itemsize = 0;
    r.tp_dealloc = PySchematic_dealloc;
    r.tp_flags = Py_TPFLAGS_DEFAULT;
    r.tp_doc = "Schematic";

    r.tp_methods = PySchematic_methods;
    r.tp_new = PySchematic_new;
    return r;
}();
