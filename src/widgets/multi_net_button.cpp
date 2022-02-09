#include "multi_net_button.hpp"
#include "block/block.hpp"
#include "multi_net_selector.hpp"

namespace horizon {
MultiNetButton::MultiNetButton(const Block &b) : block(b)
{
    ns = Gtk::manage(new MultiNetSelector(block));
    construct();
}

MultiItemSelector &MultiNetButton::get_selector()
{
    return *ns;
}

const MultiItemSelector &MultiNetButton::get_selector() const
{
    return *ns;
}

std::string MultiNetButton::get_item_name(const UUID &uu) const
{
    return block.get_net_name(uu);
}

} // namespace horizon
