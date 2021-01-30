#pragma once
#include <Python.h>

namespace horizon {
class Image3DExporterWrapper;
}

extern PyTypeObject Image3DExporterType;

typedef struct {
    PyObject_HEAD
            /* Type-specific fields go here. */
            horizon::Image3DExporterWrapper *exporter;
} PyImage3DExporter;
