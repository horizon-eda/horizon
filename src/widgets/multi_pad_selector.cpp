#include "multi_pad_selector.hpp"
#include "pool/package.hpp"
#include "util/util.hpp"

namespace horizon {
MultiPadSelector::MultiPadSelector()
{
    construct();
}

void MultiPadSelector::set_package(const Package &p)
{
    auto sel = get_selected_items();
    set_erase_if(sel, [&p](const auto &x) { return p.pads.count(x) == 0; });
    select_items(sel);
    pads.clear();
    for (const auto &[uu, pad] : p.pads) {
        pads.emplace(uu, pad.name);
    }
    update();
}

void MultiPadSelector::update()
{
    store->clear();
    for (const auto &[uu, name] : pads) {
        Gtk::TreeModel::Row row = *(store->append());
        row[list_columns.name] = name;
        row[list_columns.uuid] = uu;
    }
}

std::string MultiPadSelector::get_column_heading() const
{
    return "Pad";
}

} // namespace horizon
