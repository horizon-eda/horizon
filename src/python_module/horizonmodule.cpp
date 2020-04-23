#include <Python.h>
#include <py3cairo.h>

#include "pool/pool_manager.hpp"

#include <giomm/init.h>

#include "util.hpp"
#include "util/util.hpp"
#include "schematic.hpp"
#include "board.hpp"
#include "project.hpp"
#include "pool_manager.hpp"
#include "pool.hpp"
#include "3d_image_exporter.hpp"

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
    horizon::setup_locale();
    horizon::create_config_dir();

    if (PyType_Ready(&ProjectType) < 0)
        return NULL;

    if (PyType_Ready(&SchematicType) < 0)
        return NULL;

    if (PyType_Ready(&BoardType) < 0)
        return NULL;

    if (PyType_Ready(&PoolManagerType) < 0)
        return NULL;

    PoolType_init();
    if (PyType_Ready(&PoolType) < 0)
        return NULL;

    if (PyType_Ready(&Image3DExporterType) < 0)
        return NULL;

    if (!json_init())
        return NULL;

    if (import_cairo() < 0)
        return NULL;

    PyObject *m;

    m = PyModule_Create(&horizonmodule);
    if (m == NULL)
        return NULL;

    Py_INCREF(&ProjectType);
    PyModule_AddObject(m, "Project", (PyObject *)&ProjectType);
    PyModule_AddObject(m, "PoolManager", (PyObject *)&PoolManagerType);
    PyModule_AddObject(m, "Pool", (PyObject *)&PoolType);
    return m;
}
