#include "rule_editor.hpp"
#include "rule_match_editor.hpp"
#include "rules/rule_match.hpp"
#include "widgets/spin_button_dim.hpp"
#include "widgets/layer_combo_box.hpp"
#include "document/idocument.hpp"
#include "common/layer_provider.hpp"

namespace horizon {
RuleEditor::RuleEditor(Rule &r, class IDocument &c, HasEnable has_enable)
    : Gtk::Box(Gtk::ORIENTATION_VERTICAL, 20), rule(r), core(c)
{
    if (has_enable != HasEnable::YES) {
        set_margin_top(20);
        return;
    }
    enable_cb = Gtk::manage(new Gtk::CheckButton("Enable this rule"));
    enable_cb->set_margin_start(20);
    enable_cb->set_margin_top(20);
    pack_start(*enable_cb, false, false, 0);
    enable_cb->show();
    enable_cb->set_active(rule.enabled);
    enable_cb->signal_toggled().connect([this] {
        rule.enabled = enable_cb->get_active();
        s_signal_updated.emit();
    });
}

void RuleEditor::populate()
{
    auto la = Gtk::manage(new Gtk::Label(static_cast<std::string>(rule.uuid)));
    pack_start(*la, true, true, 0);
    la->show();
}

SpinButtonDim *RuleEditor::create_spinbutton(const char *into)
{
    Gtk::Box *box;
    builder->get_widget(into, box);
    SpinButtonDim *r = Gtk::manage(new SpinButtonDim());
    r->show();
    box->pack_start(*r, true, true, 0);
    return r;
}

RuleMatchEditor *RuleEditor::create_rule_match_editor(const char *into, RuleMatch &match)
{
    Gtk::Box *box;
    builder->get_widget(into, box);
    RuleMatchEditor *r = Gtk::manage(new RuleMatchEditor(match, core));
    r->show();
    box->pack_start(*r, true, true, 0);
    return r;
}

LayerComboBox *RuleEditor::create_layer_combo(int &layer, bool show_any)
{
    auto layer_combo = Gtk::manage(new LayerComboBox);
    for (const auto &it : core.get_layer_provider().get_layers()) {
        if (it.second.copper)
            layer_combo->prepend(it.second);
    }
    if (show_any) {
        Layer layer_any(10000, "Any layer");
        layer_combo->prepend(layer_any);
    }
    layer_combo->set_active_layer(layer);
    layer_combo->signal_changed().connect([this, layer_combo, &layer] {
        layer = layer_combo->get_active_layer();
        s_signal_updated.emit();
    });
    return layer_combo;
}
} // namespace horizon
