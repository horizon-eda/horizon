#include "multi_net_selector.hpp"
#include "block/block.hpp"

namespace horizon {
MultiNetSelector::MultiNetSelector(const Block &b) : block(b)
{
    construct();
}

void MultiNetSelector::update()
{
    store->clear();
    for (const auto &[uu, net] : block.nets) {
        if (net.is_named()) {
            Gtk::TreeModel::Row row = *(store->append());
            row[list_columns.name] = net.name;
            row[list_columns.uuid] = net.uuid;
        }
    }
}

std::string MultiNetSelector::get_column_heading() const
{
    return "Net";
}

} // namespace horizon
