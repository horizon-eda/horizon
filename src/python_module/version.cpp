#include "version.hpp"
#include "pool/part.hpp"
#include "pool/symbol.hpp"
#include "common/common.hpp"
#include "frame/frame.hpp"
#include "pool/decal.hpp"
#include "project/project.hpp"
#include "schematic/schematic.hpp"
#include "board/board.hpp"

using namespace horizon;

static unsigned int get_app_version(ObjectType type)
{
    switch (type) {
    case ObjectType::UNIT:
        return Unit::get_app_version();

    case ObjectType::SYMBOL:
        return Symbol::get_app_version();

    case ObjectType::ENTITY:
        return Entity::get_app_version();

    case ObjectType::PART:
        return Part::get_app_version();

    case ObjectType::PADSTACK:
        return Padstack::get_app_version();

    case ObjectType::PACKAGE:
        return Package::get_app_version();

    case ObjectType::FRAME:
        return Frame::get_app_version();

    case ObjectType::DECAL:
        return Decal::get_app_version();

    case ObjectType::PROJECT:
        return Project::get_app_version();

    case ObjectType::BOARD:
        return Board::get_app_version();

    case ObjectType::SCHEMATIC:
        return Schematic::get_app_version();

    default:
        throw std::runtime_error("no version for type " + object_type_lut.lookup_reverse(type));
    }
}

PyObject *py_get_app_version(PyObject *self, PyObject *args)
{
    const char *type;
    if (!PyArg_ParseTuple(args, "s", &type))
        return NULL;
    unsigned int version = 0;
    try {
        auto t = object_type_lut.lookup(type);
        version = get_app_version(t);
    }
    catch (const std::exception &e) {
        PyErr_SetString(PyExc_IOError, e.what());
        return NULL;
    }
    catch (...) {
        PyErr_SetString(PyExc_IOError, "unknown exception");
        return NULL;
    }
    return PyLong_FromLong(version);
}
