#pragma once
#include <Python.h>

extern PyTypeObject PoolManagerType;

typedef struct {
    PyObject_HEAD
    /* Type-specific fields go here. */
} PyPoolManager;
