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
    SelectionFilterDialog(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &builder, SelectionFilter *sf,
                          Core *c);
    static SelectionFilterDialog *create(Gtk::Window *parent, SelectionFilter *sf, Core *c);

private:
    SelectionFilter *selection_filter;
    Core *core;
    Gtk::ListBox *listbox = nullptr;
    Gtk::Button *select_all = nullptr;
    std::vector<Gtk::CheckButton *> checkbuttons;
};
} // namespace horizon
