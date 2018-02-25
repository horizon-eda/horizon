#pragma once
#include <gtkmm.h>
#include <set>
#include "util/changeable.hpp"

namespace horizon {
class PlaneEditor : public Gtk::Grid, public Changeable {
public:
    PlaneEditor(class PlaneSettings *settings, int *priority = nullptr);
    void set_from_rules(bool v);

private:
    class PlaneSettings *settings;
    void update_thermal();
    std::set<Gtk::Widget *> widgets_from_rules_disable;
    std::set<Gtk::Widget *> widgets_thermal_only;
};
} // namespace horizon
