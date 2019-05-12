#include <Python.h>

#include "pool/pool_manager.hpp"

#include <giomm/init.h>

#include "util.hpp"
#include "schematic.hpp"
#include "board.hpp"
#include "project.hpp"

PyDoc_STRVAR(module_doc, "Parts of horizon as a module");

static struct PyModuleDef horizonmodule = {
        PyModuleDef_HEAD_INIT, "horizon", module_doc, -1, NULL, NULL, NULL, NULL, NULL};

extern "C" {
PyMODINIT_FUNC PyInit_horizon(void);
}

PyMODINIT_FUNC PyInit_horizon(void)
{
    Gio::init();
    horizon::PoolManager::init();

    if (PyType_Ready(&ProjectType) < 0)
        return NULL;

    if (PyType_Ready(&SchematicType) < 0)
        return NULL;

    if (PyType_Ready(&BoardType) < 0)
        return NULL;

    if (!json_init())
        return NULL;

    PyObject *m;

    m = PyModule_Create(&horizonmodule);
    if (m == NULL)
        return NULL;

    Py_INCREF(&ProjectType);
    PyModule_AddObject(m, "Project", (PyObject *)&ProjectType);
    return m;
}
