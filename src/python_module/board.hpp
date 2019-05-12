#pragma once
#include <Python.h>
#include "block/block.hpp"
#include "board/board.hpp"
#include "board/via_padstack_provider.hpp"
#include "project/project.hpp"
#include "pool/pool_cached.hpp"

extern PyTypeObject BoardType;

class BoardWrapper {
public:
    BoardWrapper(const horizon::Project &prj);
    horizon::PoolCached pool;
    horizon::Block block;
    horizon::ViaPadstackProvider vpp;
    horizon::Board board;
};

typedef struct {
    PyObject_HEAD
            /* Type-specific fields go here. */
            BoardWrapper *board;
} PyBoard;
