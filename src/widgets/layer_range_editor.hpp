#pragma once
#include <gtkmm.h>
#include "util/changeable.hpp"
#include "util/layer_range.hpp"

namespace horizon {
class LayerRangeEditor : public Gtk::Box, public Changeable {
public:
    LayerRangeEditor();
    void add_layer(const class Layer &layer);
    void set_layer_range(LayerRange rng);
    LayerRange get_layer_range() const;

private:
    class LayerComboBox *combo_start = nullptr;
    class LayerComboBox *combo_end = nullptr;
    std::vector<sigc::connection> connections;

    void changed();
    void update();
};

} // namespace horizon
