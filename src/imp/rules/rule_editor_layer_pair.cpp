#include "rule_editor_layer_pair.hpp"
#include "board/rule_layer_pair.hpp"
#include "rule_match_editor.hpp"
#include "util/gtk_util.hpp"
#include "widgets/layer_combo_box.hpp"

namespace horizon {
void RuleEditorLayerPair::populate()
{
    rule2 = &dynamic_cast<RuleLayerPair &>(rule);


    auto grid = Gtk::manage(new Gtk::Grid());
    grid->set_row_spacing(10);
    grid->set_column_spacing(10);
    grid->set_margin_start(20);
    grid->set_margin_end(20);
    grid->set_margin_bottom(20);
    pack_start(*grid, true, true, 0);
    grid->set_halign(Gtk::ALIGN_START);

    int top = 0;
    auto match_editor = Gtk::manage(new RuleMatchEditor(rule2->match, core));
    match_editor->set_orientation(Gtk::ORIENTATION_HORIZONTAL);
    match_editor->signal_updated().connect([this] { s_signal_updated.emit(); });
    grid_attach_label_and_widget(grid, "Match", match_editor, top);

    {
        auto &lc = *create_layer_combo(rule2->layers.first, false);
        grid_attach_label_and_widget(grid, "Layer 1", &lc, top);
    }

    {
        auto &lc = *create_layer_combo(rule2->layers.second, false);
        grid_attach_label_and_widget(grid, "Layer 2", &lc, top);
    }


    grid->show();
}


} // namespace horizon
