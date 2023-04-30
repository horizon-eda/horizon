#include "layer_range_editor.hpp"
#include "layer_combo_box.hpp"
#include "util/scoped_block.hpp"
#include "util/layer_range.hpp"

namespace horizon {
LayerRangeEditor::LayerRangeEditor() : Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 0)
{
    get_style_context()->add_class("linked");
    combo_start = Gtk::manage(new LayerComboBox);
    combo_end = Gtk::manage(new LayerComboBox);

    combo_end->show();
    pack_start(*combo_end, false, false, 0);
    combo_start->show();
    pack_start(*combo_start, false, false, 0);

    connections.push_back(combo_start->signal_changed().connect(sigc::mem_fun(*this, &LayerRangeEditor::changed)));
    connections.push_back(combo_end->signal_changed().connect(sigc::mem_fun(*this, &LayerRangeEditor::changed)));
}

void LayerRangeEditor::add_layer(const Layer &layer)
{
    combo_start->prepend(layer);
    combo_end->prepend(layer);
}

void LayerRangeEditor::changed()
{
    {
        ScopedBlock block(connections);
        update();
    }
    s_signal_changed.emit();
}

void LayerRangeEditor::update()
{
    combo_start->set_layer_insensitive(combo_end->get_active_layer());
    combo_end->set_layer_insensitive(combo_start->get_active_layer());
    const LayerRange r{combo_start->get_active_layer(), combo_end->get_active_layer()};
    combo_start->set_active_layer(r.start());
    combo_end->set_active_layer(r.end());
}

void LayerRangeEditor::set_layer_range(LayerRange rng)
{
    {
        ScopedBlock block(connections);
        combo_start->set_active_layer(rng.start());
        combo_end->set_active_layer(rng.end());
        update();
    }
    s_signal_changed.emit();
}

LayerRange LayerRangeEditor::get_layer_range() const
{
    return {combo_start->get_active_layer(), combo_end->get_active_layer()};
}

} // namespace horizon
