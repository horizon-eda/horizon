#pragma once
#include <Python.h>

namespace horizon {
class Image3DExporter;
}

extern PyTypeObject Image3DExporterType;

typedef struct {
    PyObject_HEAD
            /* Type-specific fields go here. */
            horizon::Image3DExporter *exporter;
} PyImage3DExporter;
