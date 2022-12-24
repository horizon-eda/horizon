#pragma once
#include <gtkmm.h>
#include <array>
#include <set>
#include "common/common.hpp"
#include "parameter/set.hpp"

namespace horizon {

class EditViaDialog : public Gtk::Dialog {
public:
    EditViaDialog(Gtk::Window *parent, std::set<class Via *> &vias, class IPool &pool, IPool &pool_caching,
                  const class LayerProvider &prv, const class RuleViaDefinitions &defs);
    bool valid = false;

private:
    class ParameterSetEditor *editor = nullptr;
    Gtk::ComboBoxText *source_combo = nullptr;
    Gtk::ComboBoxText *definition_combo = nullptr;
    class LayerRangeEditor *span_editor = nullptr;
    class PoolBrowserButton *button_vp = nullptr;

    Gtk::Button *padstack_apply_all_button = nullptr;
    Gtk::Button *rules_apply_all_button = nullptr;
    Gtk::Button *span_apply_all_button = nullptr;
    Gtk::Button *definition_apply_all_button = nullptr;

    void update_sensitive();
};
} // namespace horizon
