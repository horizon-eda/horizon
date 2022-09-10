#pragma once
#include <Python.h>
#include "project/project.hpp"
extern PyTypeObject BoardType;

enum class PlaneMode { LOAD_FROM_FILE, UPDATE };
class BoardWrapper *create_board_wrapper(const horizon::Project &prj, PlaneMode plane_mode);

typedef struct {
    PyObject_HEAD
            /* Type-specific fields go here. */
            class BoardWrapper *board;
} PyBoard;
