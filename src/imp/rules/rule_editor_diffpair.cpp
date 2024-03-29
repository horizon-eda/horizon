#include "rule_editor_diffpair.hpp"
#include "board/rule_diffpair.hpp"
#include "block/block.hpp"
#include "document/idocument_board.hpp"
#include "rule_match_editor.hpp"
#include "util/gtk_util.hpp"
#include "widgets/net_class_button.hpp"
#include "widgets/spin_button_dim.hpp"
#include "widgets/layer_combo_box.hpp"

namespace horizon {
void RuleEditorDiffpair::populate()
{
    rule2 = &dynamic_cast<RuleDiffpair &>(rule);

    auto editor = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL, 20));
    editor->set_margin_start(20);
    editor->set_margin_end(20);
    editor->set_margin_bottom(20);
    editor->set_halign(Gtk::ALIGN_START);

    auto grid = Gtk::manage(new Gtk::Grid());
    grid->set_row_spacing(10);
    grid->set_column_spacing(20);

    {
        auto la = Gtk::manage(new Gtk::Label("Net class"));
        la->get_style_context()->add_class("dim-label");
        la->set_halign(Gtk::ALIGN_START);
        grid->attach(*la, 0, 0, 1, 1);
    }
    {
        auto la = Gtk::manage(new Gtk::Label("Layer"));
        la->get_style_context()->add_class("dim-label");
        la->set_halign(Gtk::ALIGN_START);
        grid->attach(*la, 1, 0, 1, 1);
    }

    auto net_class_button = Gtk::manage(new NetClassButton(*core.get_top_block()));
    net_class_button->set_net_class(rule2->net_class);
    net_class_button->signal_net_class_changed().connect([this](const UUID &net_class) {
        rule2->net_class = net_class;
        s_signal_updated.emit();
    });
    if (!rule2->net_class)
        net_class_button->set_net_class(core.get_top_block()->net_class_default->uuid);
    grid->attach(*net_class_button, 0, 1, 1, 1);

    auto layer_combo = create_layer_combo(rule2->layer, true);
    grid->attach(*layer_combo, 1, 1, 1, 1);

    grid->show_all();
    editor->pack_start(*grid, false, false, 0);

    auto grid2 = Gtk::manage(new Gtk::Grid());
    grid2->set_row_spacing(10);
    grid2->set_column_spacing(10);

    int top = 0;
    {
        auto sp = Gtk::manage(new SpinButtonDim);
        sp->set_range(0, 10_mm);
        bind_widget(sp, rule2->track_width);
        sp->signal_value_changed().connect([this] { s_signal_updated.emit(); });
        sp->set_hexpand(true);
        grid_attach_label_and_widget(grid2, "Track width", sp, top);
    }
    {
        auto sp = Gtk::manage(new SpinButtonDim);
        sp->set_range(0, 10_mm);
        bind_widget(sp, rule2->track_gap);
        sp->signal_value_changed().connect([this] { s_signal_updated.emit(); });
        grid_attach_label_and_widget(grid2, "Track gap", sp, top);
    }
    {
        auto sp = Gtk::manage(new SpinButtonDim);
        sp->set_range(0, 10_mm);
        bind_widget(sp, rule2->via_gap);
        sp->signal_value_changed().connect([this] { s_signal_updated.emit(); });
        grid_attach_label_and_widget(grid2, "Via gap", sp, top);
    }
    grid2->show_all();
    editor->pack_start(*grid2, false, false, 0);

    pack_start(*editor, true, true, 0);
    editor->show();
}
} // namespace horizon
