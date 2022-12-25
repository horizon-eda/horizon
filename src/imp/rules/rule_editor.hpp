#pragma once
#include "common/common.hpp"
#include "rules/rule.hpp"
#include "util/uuid.hpp"
#include <array>
#include <gtkmm.h>
#include <set>
namespace horizon {
class RuleEditor : public Gtk::Box {
public:
    enum class HasEnable { YES, NO };
    RuleEditor(Rule &r, class IDocument &c, HasEnable has_enable = HasEnable::YES);
    Rule &get_rule()
    {
        return rule;
    }
    virtual void populate();
    typedef sigc::signal<void> type_signal_updated;
    type_signal_updated signal_updated()
    {
        return s_signal_updated;
    }

private:
    Gtk::CheckButton *enable_cb = nullptr;

protected:
    Rule &rule;
    class IDocument &core;
    Glib::RefPtr<Gtk::Builder> builder;
    class SpinButtonDim *create_spinbutton(const char *box);
    class RuleMatchEditor *create_rule_match_editor(const char *box, class RuleMatch &match);
    class LayerComboBox *create_layer_combo(int &layer, bool show_any);
    type_signal_updated s_signal_updated;
};
} // namespace horizon
