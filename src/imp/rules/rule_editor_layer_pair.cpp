#include "rule_editor_layer_pair.hpp"
#include "board/rule_layer_pair.hpp"
#include "document/idocument.hpp"
#include "common/layer_provider.hpp"
#include "rule_match_editor.hpp"
#include "util/gtk_util.hpp"

namespace horizon {
void RuleEditorLayerPair::populate()
{
    rule2 = dynamic_cast<RuleLayerPair *>(rule);


    auto grid = Gtk::manage(new Gtk::Grid());
    grid->set_row_spacing(10);
    grid->set_column_spacing(10);
    grid->set_margin_start(20);
    grid->set_margin_end(20);
    grid->set_margin_bottom(20);
    pack_start(*grid, true, true, 0);
    grid->set_halign(Gtk::ALIGN_START);

    int top = 0;
    auto match_editor = Gtk::manage(new RuleMatchEditor(&rule2->match, core));
    match_editor->set_orientation(Gtk::ORIENTATION_HORIZONTAL);
    match_editor->signal_updated().connect([this] { s_signal_updated.emit(); });
    grid_attach_label_and_widget(grid, "Match", match_editor, top);

    {
        auto &lc = make_layer_combo(rule2->layers.first);
        grid_attach_label_and_widget(grid, "Layer 1", &lc, top);
    }

    {
        auto &lc = make_layer_combo(rule2->layers.second);
        grid_attach_label_and_widget(grid, "Layer 2", &lc, top);
    }


    grid->show();
}


Gtk::ComboBoxText &RuleEditorLayerPair::make_layer_combo(int &v)
{
    auto layer_combo = Gtk::manage(new Gtk::ComboBoxText());
    for (const auto &it : core->get_layer_provider().get_layers()) {
        if (it.second.copper)
            layer_combo->insert(0, std::to_string(it.first), it.second.name + ": " + std::to_string(it.first));
    }
    layer_combo->set_active_id(std::to_string(v));
    layer_combo->signal_changed().connect([this, layer_combo, &v] {
        v = std::stoi(layer_combo->get_active_id());
        s_signal_updated.emit();
    });
    return *layer_combo;
}


} // namespace horizon
