#pragma once
#include <Python.h>
#include "nlohmann/json_fwd.hpp"

using json = nlohmann::json;

bool json_init();
PyObject *py_from_json(const json &j);
json json_from_py(PyObject *o);

class py_exception : public std::exception {
};
