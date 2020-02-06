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
    RuleEditor(Rule *r, class IDocument *c);
    virtual void populate();
    typedef sigc::signal<void> type_signal_updated;
    type_signal_updated signal_updated()
    {
        return s_signal_updated;
    }

private:
    Gtk::CheckButton *enable_cb = nullptr;

protected:
    Rule *rule;
    class IDocument *core = nullptr;
    Glib::RefPtr<Gtk::Builder> builder;
    class SpinButtonDim *create_spinbutton(const char *box);
    class RuleMatchEditor *create_rule_match_editor(const char *box, class RuleMatch *match);
    type_signal_updated s_signal_updated;
};
} // namespace horizon
