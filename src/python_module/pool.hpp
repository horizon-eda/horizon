#pragma once
#include <Python.h>

namespace horizon {
class Pool;
}

extern PyTypeObject PoolType;

typedef struct {
    PyObject_HEAD
            /* Type-specific fields go here. */
            horizon::Pool *pool;
} PyPool;

void PoolType_init();
