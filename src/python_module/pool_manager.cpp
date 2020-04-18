#include "pool/pool_manager.hpp"
#include "pool_manager.hpp"

static PyObject *PyPoolManager_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    PyPoolManager *self;
    self = (PyPoolManager *)type->tp_alloc(type, 0);
    return (PyObject *)self;
}

static void PyPoolManager_dealloc(PyObject *pself)
{
    auto self = reinterpret_cast<PyPoolManager *>(pself);
    Py_TYPE(self)->tp_free((PyObject *)self);
}

static int PyPoolManager_init(PyObject *pself, PyObject *args, PyObject *kwds)
{
    auto self = reinterpret_cast<PyPoolManager *>(pself);
    return 0;
}

static PyObject *PyPoolManager_get_pools(PyObject *pself, PyObject *args)
{
    auto py_pools = PyDict_New();
    if (!py_pools)
        return NULL;
    auto pools = horizon::PoolManager::get().get_pools();
    for (const auto &it : pools) {
        auto uu_str = (std::string)it.second.uuid;
        PyDict_SetItemString(py_pools, it.first.c_str(), PyUnicode_FromString(uu_str.c_str()));
    }
    return py_pools;
}


static PyObject *PyPoolManager_add_pool(PyObject *pself, PyObject *args)
{
    const char *path;
    if (!PyArg_ParseTuple(args, "s", &path))
        return NULL;
    try {
        horizon::PoolManager::get().add_pool(path);
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


static PyMethodDef PyPoolManager_methods[] = {
        {"get_pools", (PyCFunction)PyPoolManager_get_pools, METH_NOARGS | METH_STATIC, "Get Pools"},
        {"add_pool", (PyCFunction)PyPoolManager_add_pool, METH_VARARGS | METH_STATIC, "Add Pool"},
        {NULL} /* Sentinel */
};


PyTypeObject PoolManagerType = [] {
    PyTypeObject r = {PyVarObject_HEAD_INIT(NULL, 0)};
    r.tp_name = "horizon.PoolManager";
    r.tp_basicsize = sizeof(PyPoolManager);

    r.tp_itemsize = 0;
    r.tp_dealloc = PyPoolManager_dealloc;
    r.tp_flags = Py_TPFLAGS_DEFAULT;
    r.tp_doc = "PoolManager";

    r.tp_methods = PyPoolManager_methods;
    r.tp_init = PyPoolManager_init;
    r.tp_new = PyPoolManager_new;
    return r;
}();
