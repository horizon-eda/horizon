#include "rule_editor_track_width.hpp"
#include "board/rule_track_width.hpp"
#include "document/idocument.hpp"
#include "common/layer_provider.hpp"
#include "rule_match_editor.hpp"
#include "widgets/spin_button_dim.hpp"
#include "widgets/color_box.hpp"
#include "preferences/preferences_provider.hpp"
#include "preferences/preferences.hpp"

namespace horizon {
void RuleEditorTrackWidth::populate()
{
    rule2 = &dynamic_cast<RuleTrackWidth &>(rule);

    builder = Gtk::Builder::create_from_resource(
            "/org/horizon-eda/horizon/imp/rules/"
            "rule_editor_track_width.ui");
    Gtk::Box *editor;
    builder->get_widget("editor", editor);
    pack_start(*editor, true, true, 0);

    auto match_editor = create_rule_match_editor("match_box", rule2->match);
    match_editor->set_orientation(Gtk::ORIENTATION_HORIZONTAL);
    match_editor->signal_updated().connect([this] { s_signal_updated.emit(); });

    Gtk::Grid *layer_grid;
    builder->get_widget("layer_grid", layer_grid);
    auto layers = core.get_layer_provider().get_layers();
    int top = 1;
    for (auto it = layers.rbegin(); it != layers.rend(); it++) {
        auto &la = it->second;
        auto i = it->first;
        if (la.copper) {
            std::cout << "have cu layer " << la.name << std::endl;
            if (rule2->widths.count(i) == 0) {
                rule2->widths[i];
            }

            {
                auto label = Gtk::manage(new Gtk::Label(la.name));
                auto box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 5));
                box->set_halign(Gtk::ALIGN_END);
                box->pack_start(*label, false, false, 0);
                auto cb = Gtk::manage(new ColorBox);
                cb->set_size_request(16, 16);
                cb->set_valign(Gtk::ALIGN_CENTER);
                auto &colors = PreferencesProvider::get_prefs().canvas_layer.appearance.layer_colors;
                if (colors.count(la.index))
                    cb->set_color(colors.at(la.index));
                box->pack_start(*cb, false, false, 0);
                box->show_all();
                layer_grid->attach(*box, 0, top, 1, 1);
            }
            auto sp_min = Gtk::manage(new SpinButtonDim());
            sp_min->set_range(0, 100_mm);
            sp_min->set_value(rule2->widths.at(i).min);
            sp_min->signal_value_changed().connect(
                    [this, i, sp_min] { rule2->widths.at(i).min = sp_min->get_value_as_int(); });
            layer_grid->attach(*sp_min, 1, top, 1, 1);

            auto sp_default = Gtk::manage(new SpinButtonDim());
            sp_default->set_range(0, 100_mm);
            sp_default->set_value(rule2->widths.at(i).def);
            sp_default->signal_value_changed().connect(
                    [this, i, sp_default] { rule2->widths.at(i).def = sp_default->get_value_as_int(); });
            layer_grid->attach(*sp_default, 2, top, 1, 1);

            auto sp_max = Gtk::manage(new SpinButtonDim());
            sp_max->set_range(0, 100_mm);
            sp_max->set_value(rule2->widths.at(i).max);
            sp_max->signal_value_changed().connect(
                    [this, i, sp_max] { rule2->widths.at(i).max = sp_max->get_value_as_int(); });
            layer_grid->attach(*sp_max, 3, top, 1, 1);

            top++;
        }
    }
    layer_grid->show_all();

    editor->show();
}
} // namespace horizon
