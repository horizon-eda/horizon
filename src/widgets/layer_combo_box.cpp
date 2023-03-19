#include "layer_combo_box.hpp"
#include "cell_renderer_color_box.hpp"
#include "common/layer.hpp"
#include "preferences/preferences.hpp"
#include "preferences/preferences_provider.hpp"

namespace horizon {

LayerComboBox::LayerComboBox() : Gtk::ComboBox()
{
    store = Gtk::ListStore::create(list_columns);
    set_model(store);
    auto cr_color = Gtk::manage(new CellRendererColorBox);
    pack_start(*cr_color, false);

    auto cr_text = Gtk::manage(new Gtk::CellRendererText);
    pack_start(*cr_text, true);
    add_attribute(cr_text->property_text(), list_columns.name);
    add_attribute(cr_color->property_color(), list_columns.color);
    add_attribute(cr_text->property_sensitive(), list_columns.sensitive);
    add_attribute(cr_color->property_sensitive(), list_columns.sensitive);
}

void LayerComboBox::set_active_layer(int layer)
{
    for (const auto &it : store->children()) {
        Gtk::TreeModel::Row row = *it;
        if (row[list_columns.layer] == layer) {
            set_active(it);
            return;
        }
    }
}

int LayerComboBox::get_active_layer() const
{
    if (get_active_row_number() == -1)
        return 10000;
    const auto it = get_active();
    Gtk::TreeModel::Row row = *it;
    return row.get_value(list_columns.layer);
}

void LayerComboBox::remove_all()
{
    store->clear();
}

static Gdk::RGBA rgba_from_color(const Color &c)
{
    Gdk::RGBA r;
    r.set_rgba(c.r, c.g, c.b);
    return r;
}

void LayerComboBox::prepend(const Layer &layer)
{
    Gtk::TreeModel::Row row = *store->prepend();
    row[list_columns.layer] = layer.index;
    row[list_columns.name] = layer.name;
    row[list_columns.sensitive] = true;
    auto &prefs = PreferencesProvider::get_prefs();
    const auto &colors = prefs.canvas_layer.appearance.layer_colors;
    Gdk::RGBA rgba;
    if (colors.count(layer.index) && layer.index != 10000) {
        rgba = rgba_from_color(colors.at(layer.index));
    }
    else {
        rgba.set_alpha(0);
    }
    row[list_columns.color] = rgba;
}

void LayerComboBox::set_layer_insensitive(int layer)
{
    for (auto it : store->children()) {
        Gtk::TreeModel::Row row = *it;
        row[list_columns.sensitive] = row[list_columns.layer] != layer;
    }
}

} // namespace horizon
