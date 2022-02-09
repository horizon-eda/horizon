#include "multi_component_selector.hpp"
#include "block/block.hpp"

namespace horizon {
MultiComponentSelector::MultiComponentSelector(const Block &b) : block(b)
{
    construct();
}

void MultiComponentSelector::update()
{
    store->clear();
    for (const auto &[uu, comp] : block.components) {
        Gtk::TreeModel::Row row = *(store->append());
        row[list_columns.name] = comp.refdes;
        row[list_columns.uuid] = comp.uuid;
    }
}

std::string MultiComponentSelector::get_column_heading() const
{
    return "Ref. Desig.";
}

} // namespace horizon
