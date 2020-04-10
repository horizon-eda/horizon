#pragma once
#include <Python.h>
#include "project/project.hpp"
extern PyTypeObject BoardType;

class BoardWrapper *create_board_wrapper(const horizon::Project &prj);

typedef struct {
    PyObject_HEAD
            /* Type-specific fields go here. */
            class BoardWrapper *board;
} PyBoard;
