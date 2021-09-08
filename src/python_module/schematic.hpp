#pragma once
#include <Python.h>
#include "blocks/blocks_schematic.hpp"
#include "project/project.hpp"
#include "pool/project_pool.hpp"

extern PyTypeObject SchematicType;

class SchematicWrapper {
public:
    SchematicWrapper(const horizon::Project &pr);
    horizon::ProjectPool pool;
    horizon::BlocksSchematic blocks;
};

typedef struct {
    PyObject_HEAD
            /* Type-specific fields go here. */
            SchematicWrapper *schematic;
} PySchematic;
