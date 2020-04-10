#include "project.hpp"
#include "schematic.hpp"
#include "board.hpp"
#include "pool/pool_manager.hpp"

ProjectWrapper::ProjectWrapper(const std::string &path) : project(horizon::Project::new_from_file(path))
{
}

static PyObject *PyProject_open_top_schematic(PyObject *pself)
{
    auto self = reinterpret_cast<PyProject *>(pself);
    if (horizon::PoolManager::get().get_by_uuid(self->project->project.pool_uuid) == nullptr) {
        PyErr_SetString(PyExc_FileNotFoundError, "pool not found");
        return NULL;
    }
    SchematicWrapper *schematic = nullptr;
    try {
        auto top_block = self->project->project.get_top_block();
        schematic = new SchematicWrapper(self->project->project, top_block.uuid);
    }
    catch (const std::exception &e) {
        PyErr_SetString(PyExc_IOError, e.what());
        return NULL;
    }
    catch (...) {
        PyErr_SetString(PyExc_IOError, "unknown exception");
        return NULL;
    }

    PySchematic *sch = PyObject_New(PySchematic, &SchematicType);
    sch->schematic = schematic;
    return reinterpret_cast<PyObject *>(sch);
}

static PyObject *PyProject_open_board(PyObject *pself)
{
    auto self = reinterpret_cast<PyProject *>(pself);
    if (horizon::PoolManager::get().get_by_uuid(self->project->project.pool_uuid) == nullptr) {
        PyErr_SetString(PyExc_FileNotFoundError, "pool not found");
        return NULL;
    }
    class BoardWrapper *board = nullptr;
    try {
        board = create_board_wrapper(self->project->project);
    }
    catch (const std::exception &e) {
        PyErr_SetString(PyExc_IOError, e.what());
        return NULL;
    }
    catch (...) {
        PyErr_SetString(PyExc_IOError, "unknown exception");
        return NULL;
    }

    PyBoard *brd = PyObject_New(PyBoard, &BoardType);
    brd->board = board;
    return reinterpret_cast<PyObject *>(brd);
}


static PyObject *PyProject_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    PyProject *self;
    self = (PyProject *)type->tp_alloc(type, 0);
    if (self != NULL) {
        self->project = nullptr;
    }
    return (PyObject *)self;
}

static void PyProject_dealloc(PyObject *pself)
{
    auto self = reinterpret_cast<PyProject *>(pself);
    delete self->project;
    Py_TYPE(self)->tp_free((PyObject *)self);
}

static int PyProject_init(PyObject *pself, PyObject *args, PyObject *kwds)
{
    auto self = reinterpret_cast<PyProject *>(pself);
    const char *path;
    if (!PyArg_ParseTuple(args, "s", &path))
        return -1;
    ProjectWrapper *new_project = nullptr;
    try {
        new_project = new ProjectWrapper(path);
    }
    catch (const std::exception &e) {
        PyErr_SetString(PyExc_IOError, e.what());
        return -1;
    }
    catch (...) {
        PyErr_SetString(PyExc_IOError, "unknown exception");
        return -1;
    }
    delete self->project;
    self->project = new_project;
    return 0;
}

static PyMethodDef PyProject_methods[] = {
        {"open_top_schematic", (PyCFunction)PyProject_open_top_schematic, METH_NOARGS, "Open top block"},
        {"open_board", (PyCFunction)PyProject_open_board, METH_NOARGS, "Open board"},
        {NULL} /* Sentinel */
};


PyTypeObject ProjectType = [] {
    PyTypeObject r = {PyVarObject_HEAD_INIT(NULL, 0)};
    r.tp_name = "horizon.Project";
    r.tp_basicsize = sizeof(PyProject);

    r.tp_itemsize = 0;
    r.tp_dealloc = PyProject_dealloc;
    r.tp_flags = Py_TPFLAGS_DEFAULT;
    r.tp_doc = "Project";

    r.tp_methods = PyProject_methods;
    r.tp_init = PyProject_init;
    r.tp_new = PyProject_new;
    return r;
}();
