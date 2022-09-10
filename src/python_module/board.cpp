#include "board.hpp"
#include "nlohmann/json.hpp"
#include "util.hpp"
#include "export_gerber/gerber_export.hpp"
#include "export_odb/odb_export.hpp"
#include "export_pdf/export_pdf_board.hpp"
#include "export_pnp/export_pnp.hpp"
#include "export_step/export_step.hpp"
#include "image_3d_exporter_wrapper.hpp"
#include "document/document_board.hpp"
#include "rules/cache.hpp"
#include <podofo/podofo.h>
#include "blocks/blocks.hpp"
#include "board/board.hpp"
#include "project/project.hpp"
#include "pool/project_pool.hpp"
#include "rules/rule_descr.hpp"
#include "3d_image_exporter.hpp"

class BoardWrapper : public horizon::DocumentBoard {
public:
    BoardWrapper(const horizon::Project &prj, PlaneMode mode);
    horizon::ProjectPool pool;
    horizon::Block block;
    horizon::Board board;
    horizon::Board *get_board() override
    {
        return &board;
    }
    horizon::Block *get_top_block() override
    {
        return &block;
    }
    horizon::IPool &get_pool() override
    {
        return pool;
    }
    horizon::IPool &get_pool_caching() override
    {
        return pool;
    }
    horizon::Rules *get_rules() override
    {
        return &board.rules;
    }
    horizon::LayerProvider &get_layer_provider() override
    {
        return board;
    }
    horizon::GerberOutputSettings &get_gerber_output_settings() override
    {
        return board.gerber_output_settings;
    }
    horizon::ODBOutputSettings &get_odb_output_settings() override
    {
        return board.odb_output_settings;
    }
    horizon::PDFExportSettings &get_pdf_export_settings() override
    {
        return board.pdf_export_settings;
    }
    horizon::STEPExportSettings &get_step_export_settings() override
    {
        return board.step_export_settings;
    }
    horizon::PnPExportSettings &get_pnp_export_settings() override
    {
        return board.pnp_export_settings;
    }
    horizon::BoardColors &get_colors() override
    {
        return board.colors;
    }
    horizon::GridSettings *get_grid_settings() override
    {
        return &board.grid_settings;
    }
    const horizon::FileVersion &get_version() const override
    {
        return board.version;
    }
};

class BoardWrapper *create_board_wrapper(const horizon::Project &prj, PlaneMode plane_mode)
{
    return new BoardWrapper(prj, plane_mode);
}


static horizon::Block get_flattend_block(const std::string &blocks_filename, horizon::IPool &pool)
{
    auto blocks = horizon::Blocks::new_from_file(blocks_filename, pool);
    return blocks.get_top_block_item().block.flatten();
}

BoardWrapper::BoardWrapper(const horizon::Project &prj, PlaneMode plane_mode)
    : pool(prj.pool_directory, false), block(get_flattend_block(prj.blocks_filename, pool)),
      board(horizon::Board::new_from_file(prj.board_filename, block, pool))
{
    board.expand();
    if (plane_mode == PlaneMode::LOAD_FROM_FILE)
        board.load_planes_from_file(prj.planes_filename);
    else
        board.update_planes();
}


static PyObject *PyBoard_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    PyBoard *self;
    self = (PyBoard *)type->tp_alloc(type, 0);
    if (self != NULL) {
        self->board = nullptr;
    }
    return (PyObject *)self;
}

static void PyBoard_dealloc(PyObject *pself)
{
    auto self = reinterpret_cast<PyBoard *>(pself);
    delete self->board;
    Py_TYPE(self)->tp_free((PyObject *)self);
}


static PyObject *PyBoard_get_gerber_export_settings(PyObject *pself, PyObject *args)
{
    auto self = reinterpret_cast<PyBoard *>(pself);
    auto settings = self->board->board.gerber_output_settings.serialize();
    return py_from_json(settings);
}

static PyObject *PyBoard_get_odb_export_settings(PyObject *pself, PyObject *args)
{
    auto self = reinterpret_cast<PyBoard *>(pself);
    auto settings = self->board->board.odb_output_settings.serialize();
    return py_from_json(settings);
}

static PyObject *PyBoard_export_gerber(PyObject *pself, PyObject *args)
{
    auto self = reinterpret_cast<PyBoard *>(pself);
    PyObject *py_export_settings = nullptr;
    if (!PyArg_ParseTuple(args, "O!", &PyDict_Type, &py_export_settings))
        return NULL;
    try {
        auto settings_json = json_from_py(py_export_settings);
        horizon::GerberOutputSettings settings(settings_json);
        horizon::GerberExporter ex(self->board->board, settings);
        ex.generate();
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

static PyObject *PyBoard_export_odb(PyObject *pself, PyObject *args)
{
    auto self = reinterpret_cast<PyBoard *>(pself);
    PyObject *py_export_settings = nullptr;
    if (!PyArg_ParseTuple(args, "O!", &PyDict_Type, &py_export_settings))
        return NULL;
    try {
        auto settings_json = json_from_py(py_export_settings);
        horizon::ODBOutputSettings settings(settings_json);
        horizon::export_odb(self->board->board, settings);
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

static PyObject *PyBoard_get_pdf_export_settings(PyObject *pself, PyObject *args)
{
    auto self = reinterpret_cast<PyBoard *>(pself);
    auto settings = self->board->board.pdf_export_settings.serialize_board();
    return py_from_json(settings);
}

static PyObject *PyBoard_export_pdf(PyObject *pself, PyObject *args)
{
    auto self = reinterpret_cast<PyBoard *>(pself);
    PyObject *py_export_settings = nullptr;
    if (!PyArg_ParseTuple(args, "O!", &PyDict_Type, &py_export_settings))
        return NULL;
    try {
        auto settings_json = json_from_py(py_export_settings);
        horizon::PDFExportSettings settings(settings_json);
        horizon::export_pdf(self->board->board, settings, nullptr);
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

static PyObject *PyBoard_get_pnp_export_settings(PyObject *pself, PyObject *args)
{
    auto self = reinterpret_cast<PyBoard *>(pself);
    auto settings = self->board->board.pnp_export_settings.serialize();
    return py_from_json(settings);
}

static PyObject *PyBoard_export_pnp(PyObject *pself, PyObject *args)
{
    auto self = reinterpret_cast<PyBoard *>(pself);
    PyObject *py_export_settings = nullptr;
    if (!PyArg_ParseTuple(args, "O!", &PyDict_Type, &py_export_settings))
        return NULL;
    try {
        auto settings_json = json_from_py(py_export_settings);
        horizon::PnPExportSettings settings(settings_json);
        horizon::export_PnP(self->board->board, settings);
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

static PyObject *PyBoard_get_step_export_settings(PyObject *pself, PyObject *args)
{
    auto self = reinterpret_cast<PyBoard *>(pself);
    auto settings = self->board->board.step_export_settings.serialize();
    return py_from_json(settings);
}

static void callback_wrapper(PyObject *cb, const std::string &s)
{
    if (cb) {
        PyObject *arglist = Py_BuildValue("(s)", s.c_str());
        PyObject *result = PyObject_CallObject(cb, arglist);
        Py_DECREF(arglist);
        if (result == NULL) {
            throw py_exception();
        }
        Py_DECREF(result);
    }
}

static PyObject *PyBoard_export_step(PyObject *pself, PyObject *args)
{
    auto self = reinterpret_cast<PyBoard *>(pself);
    PyObject *py_export_settings = nullptr;
    PyObject *py_callback = nullptr;
    if (!PyArg_ParseTuple(args, "O!|O", &PyDict_Type, &py_export_settings, &py_callback))
        return NULL;
    if (py_callback && !PyCallable_Check(py_callback)) {
        PyErr_SetString(PyExc_TypeError, "callback must be callable");
        return NULL;
    }
    try {
        auto settings_json = json_from_py(py_export_settings);
        horizon::STEPExportSettings settings(settings_json);
        horizon::export_step(
                settings.filename, self->board->board, self->board->pool, settings.include_3d_models,
                [py_callback](const std::string &s) { callback_wrapper(py_callback, s); }, nullptr, settings.prefix);
    }
    catch (const py_exception &e) {
        return NULL;
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

static void dummy_callback(const std::string &s)
{
}

static PyObject *PyBoard_run_checks(PyObject *pself, PyObject *args)
{
    auto self = reinterpret_cast<PyBoard *>(pself);
    PyObject *py_rules = nullptr;
    PyObject *py_rule_ids = nullptr;
    if (!PyArg_ParseTuple(args, "O!O", &PyDict_Type, &py_rules, &py_rule_ids))
        return NULL;
    PyObject *iter = nullptr;
    if (!(iter = PyObject_GetIter(py_rule_ids))) {
        PyErr_SetString(PyExc_TypeError, "rule ids must be iterable");
        return NULL;
    }

    std::set<horizon::RuleID> ids;

    while (PyObject *item = PyIter_Next(iter)) {
        auto rule_name = PyUnicode_AsUTF8(item);
        if (!rule_name) {
            Py_DECREF(item);
            Py_DECREF(iter);
            return NULL;
        }
        Py_DECREF(item);
        horizon::RuleID id;
        try {
            id = horizon::rule_id_lut.lookup(std::string{rule_name});
            ids.emplace(id);
        }
        catch (...) {
            PyErr_SetString(PyExc_IOError, "rule not found");
            Py_DECREF(item);
            Py_DECREF(iter);
            return NULL;
        }
    }

    Py_DECREF(iter);

    if (PyErr_Occurred()) {
        return NULL;
    }
    try {
        auto rules_json = json_from_py(py_rules);
        horizon::BoardRules rules;
        rules.load_from_json(rules_json);

        horizon::RulesCheckCache cache(*self->board);
        json j;
        for (auto id : ids) {
            auto r = self->board->board.rules.check(id, self->board->board, cache, &dummy_callback);
            j[horizon::rule_id_lut.lookup_reverse(id)] = r.serialize();
        }
        return py_from_json(j);
    }
    catch (const py_exception &e) {
        return NULL;
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

static PyObject *PyBoard_get_rules(PyObject *pself, PyObject *args)
{
    auto self = reinterpret_cast<PyBoard *>(pself);
    auto rules = self->board->board.rules.serialize();
    return py_from_json(rules);
}

static PyObject *PyBoard_get_rule_ids(PyObject *pself, PyObject *args)
{
    auto self = reinterpret_cast<PyBoard *>(pself);
    auto rules = self->board->board.rules.get_rule_ids();
    auto r = PySet_New(NULL);
    if (!r)
        return NULL;
    for (auto id : rules) {
        if (horizon::rule_descriptions.at(id).can_check) {
            auto u = PyUnicode_FromString(horizon::rule_id_lut.lookup_reverse(id).c_str());
            if (PySet_Add(r, u) == -1) {
                return NULL;
            }
        }
    }
    return r;
}

static PyObject *PyBoard_export_3d(PyObject *pself, PyObject *args)
{
    auto self = reinterpret_cast<PyBoard *>(pself);
    unsigned int w, h;
    if (!PyArg_ParseTuple(args, "II", &w, &h))
        return NULL;
    class horizon::Image3DExporterWrapper *exporter = nullptr;
    try {
        exporter = new horizon::Image3DExporterWrapper(self->board->board, self->board->pool, w, h);
    }
    catch (const std::exception &e) {
        PyErr_SetString(PyExc_IOError, e.what());
        return NULL;
    }
    catch (...) {
        PyErr_SetString(PyExc_IOError, "unknown exception");
        return NULL;
    }

    PyImage3DExporter *ex = PyObject_New(PyImage3DExporter, &Image3DExporterType);
    ex->exporter = exporter;
    return reinterpret_cast<PyObject *>(ex);
}


static PyMethodDef PyBoard_methods[] = {
        {"get_gerber_export_settings", PyBoard_get_gerber_export_settings, METH_NOARGS,
         "Return gerber export settings"},
        {"get_odb_export_settings", PyBoard_get_odb_export_settings, METH_NOARGS, "Return ODB++ export settings"},
        {"export_gerber", PyBoard_export_gerber, METH_VARARGS, "Export gerber"},
        {"export_odb", PyBoard_export_odb, METH_VARARGS, "Export ODB++"},
        {"get_pdf_export_settings", PyBoard_get_pdf_export_settings, METH_NOARGS, "Return PDF export settings"},
        {"get_pnp_export_settings", PyBoard_get_pnp_export_settings, METH_NOARGS, "Return PnP export settings"},
        {"get_step_export_settings", PyBoard_get_step_export_settings, METH_NOARGS, "Return STEP export settings"},
        {"get_rules", PyBoard_get_rules, METH_NOARGS, "Return rules"},
        {"get_rule_ids", PyBoard_get_rule_ids, METH_NOARGS, "Return rule IDs"},
        {"export_pdf", PyBoard_export_pdf, METH_VARARGS, "Export PDF"},
        {"export_pnp", PyBoard_export_pnp, METH_VARARGS, "Export pick and place"},
        {"export_step", PyBoard_export_step, METH_VARARGS, "Export STEP"},
        {"run_checks", PyBoard_run_checks, METH_VARARGS, "Run checks"},
        {"export_3d", PyBoard_export_3d, METH_VARARGS, "Export 3D image"},
        {NULL} /* Sentinel */
};

PyTypeObject BoardType = [] {
    PyTypeObject r = {PyVarObject_HEAD_INIT(NULL, 0)};
    r.tp_name = "horizon.Board";
    r.tp_basicsize = sizeof(PyBoard);

    r.tp_itemsize = 0;
    r.tp_dealloc = PyBoard_dealloc;
    r.tp_flags = Py_TPFLAGS_DEFAULT;
    r.tp_doc = "Board";

    r.tp_methods = PyBoard_methods;
    r.tp_new = PyBoard_new;
    return r;
}();
