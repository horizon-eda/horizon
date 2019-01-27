#pragma once
#include <gtkmm.h>
#include <map>
#include <string>
#include "pool/pool_parametric.hpp"
#include "util/changeable.hpp"

namespace horizon {


class ParametricEditor : public Gtk::Grid, public Changeable {
public:
    ParametricEditor(PoolParametric *p, const std::string &t);

    void update(const std::map<std::string, std::string> &params);

    std::map<std::string, std::string> get_values();

private:
    PoolParametric *pool;
    const PoolParametric::Table &table;
    std::map<std::string, class ParametricParamEditor *> editors;
};
} // namespace horizon
