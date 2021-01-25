#pragma once
#include <gtkmm.h>
#include "rules/rules.hpp"

namespace horizon {
void add_check_column(Gtk::TreeView &treeview, Gtk::TreeModelColumn<RulesCheckResult> &result_col);
}
