#include "multi_component_button.hpp"
#include "block/block.hpp"
#include "multi_component_selector.hpp"

namespace horizon {
MultiComponentButton::MultiComponentButton(const Block &b) : block(b)
{
    ns = Gtk::manage(new MultiComponentSelector(block));
    construct();
}

MultiItemSelector &MultiComponentButton::get_selector()
{
    return *ns;
}

const MultiItemSelector &MultiComponentButton::get_selector() const
{
    return *ns;
}

std::string MultiComponentButton::get_item_name(const UUID &uu) const
{
    return block.components.at(uu).refdes;
}

} // namespace horizon
