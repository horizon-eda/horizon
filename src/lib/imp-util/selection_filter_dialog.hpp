#pragma once
#include "canvas/selection_filter.hpp"
#include "common/common.hpp"
#include "core/core.hpp"
#include "util/uuid.hpp"
#include <array>
#include <gtkmm.h>
#include <set>
namespace horizon {

class SelectionFilterDialog : public Gtk::Window {
public:
    SelectionFilterDialog(Gtk::Window *parent, SelectionFilter *sf, Core *c);

private:
    SelectionFilter *selection_filter;
    Core *core;
    Gtk::ListBox *listbox = nullptr;
    std::vector<Gtk::CheckButton *> checkbuttons;
};
} // namespace horizon
