#include "util.hpp"
#include "nlohmann/json.hpp"

static PyObject *json_module = nullptr;
static PyObject *json_loads = nullptr;
static PyObject *json_dumps = nullptr;

bool json_init()
{
    json_module = PyImport_ImportModule("json");
    if (!json_module)
        return false;

    json_loads = PyObject_GetAttrString(json_module, "loads");
    if (!json_loads)
        return false;

    json_dumps = PyObject_GetAttrString(json_module, "dumps");
    if (!json_dumps)
        return false;
    return true;
}

PyObject *py_from_json(const json &j)
{
    std::string s = j.dump();
    PyObject *r = nullptr;
    auto arglist = Py_BuildValue("(s)", s.c_str());
    r = PyObject_CallObject(json_loads, arglist);
    Py_DECREF(arglist);
    return r;
}

json json_from_py(PyObject *o)
{
    auto arglist = Py_BuildValue("(O)", o);
    PyObject *os = PyObject_CallObject(json_dumps, arglist);
    Py_DECREF(arglist);
    if (!os)
        throw std::runtime_error("json_dumps failed");
    auto s = PyUnicode_AsUTF8(os);
    if (!s) {
        Py_DECREF(os);
        throw std::runtime_error("PyUnicode_AsUTF8 failed");
    }
    json j;
    try {
        j = json::parse(s);
    }
    catch (...) {
        Py_DECREF(os);
        throw;
    }
    Py_DECREF(os);
    return j;
}
