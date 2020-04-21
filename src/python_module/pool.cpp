#include "pool.hpp"
#include "pool-update/pool-update.hpp"
#include "util.hpp"
#include "pool/pool.hpp"
#include <structmember.h>

static PyObject *PyPool_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    PyPool *self;
    self = (PyPool *)type->tp_alloc(type, 0);
    if (self != NULL) {
        self->pool = nullptr;
    }
    return (PyObject *)self;
}

static void PyPool_dealloc(PyObject *pself)
{
    auto self = reinterpret_cast<PyPool *>(pself);
    delete self->pool;
    Py_TYPE(self)->tp_free((PyObject *)self);
}

static int PyPool_init(PyObject *pself, PyObject *args, PyObject *kwds)
{
    auto self = reinterpret_cast<PyPool *>(pself);
    const char *path;
    if (!PyArg_ParseTuple(args, "s", &path))
        return -1;
    horizon::Pool *new_pool = nullptr;
    try {
        new_pool = new horizon::Pool(path);
    }
    catch (const std::exception &e) {
        PyErr_SetString(PyExc_IOError, e.what());
        return -1;
    }
    catch (...) {
        PyErr_SetString(PyExc_IOError, "unknown exception");
        return -1;
    }
    delete self->pool;
    self->pool = new_pool;
    return 0;
}

static void callback_wrapper(PyObject *cb, horizon::PoolUpdateStatus status, std::string filename, std::string msg)
{
    if (cb) {
        PyObject *arglist = Py_BuildValue("(iss)", static_cast<int>(status), filename.c_str(), msg.c_str());
        PyObject *result = PyObject_CallObject(cb, arglist);
        Py_DECREF(arglist);
        if (result == NULL) {
            throw py_exception();
        }
        Py_DECREF(result);
    }
}

static PyObject *PyPool_update(PyObject *pself, PyObject *args)
{
    const char *path;
    PyObject *py_callback = nullptr;
    if (!PyArg_ParseTuple(args, "s|O", &path, &py_callback))
        return NULL;
    if (py_callback && !PyCallable_Check(py_callback)) {
        PyErr_SetString(PyExc_TypeError, "callback must be callable");
        return NULL;
    }

    try {
        horizon::pool_update(
                path,
                [py_callback](horizon::PoolUpdateStatus status, std::string filename, std::string msg) {
                    callback_wrapper(py_callback, status, filename, msg);
                },
                true);
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

static PyMethodDef PyPool_methods[] = {
        {"update", (PyCFunction)PyPool_update, METH_VARARGS | METH_STATIC, "Update pool"},
        {NULL} /* Sentinel */
};

PyTypeObject PoolType = {PyVarObject_HEAD_INIT(NULL, 0)};


void PoolType_init()
{
    auto &r = PoolType;
    r.tp_name = "horizon.Pool";
    r.tp_basicsize = sizeof(PyPool);

    r.tp_itemsize = 0;
    r.tp_dealloc = PyPool_dealloc;
    r.tp_flags = Py_TPFLAGS_DEFAULT;
    r.tp_doc = "Pool";

    r.tp_methods = PyPool_methods;
    r.tp_init = PyPool_init;
    r.tp_new = PyPool_new;

    r.tp_dict = PyDict_New();
#define ADD_STATUS(x)                                                                                                  \
    PyDict_SetItemString(r.tp_dict, "UPDATE_STATUS_" #x,                                                               \
                         PyLong_FromLong(static_cast<int>(horizon::PoolUpdateStatus::x)))
    ADD_STATUS(INFO);
    ADD_STATUS(FILE);
    ADD_STATUS(FILE_ERROR);
    ADD_STATUS(ERROR);
    ADD_STATUS(DONE);
#undef ADD_STATUS
}
