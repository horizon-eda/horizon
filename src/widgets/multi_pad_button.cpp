#include "multi_pad_button.hpp"
#include "pool/package.hpp"
#include "multi_pad_selector.hpp"

namespace horizon {
MultiPadButton::MultiPadButton()
{
    ns = Gtk::manage(new MultiPadSelector());
    construct();
}

void MultiPadButton::set_package(const Package &p)
{
    pkg_name = p.name;
    ns->set_package(p);
    update_label();
}

MultiItemSelector &MultiPadButton::get_selector()
{
    return *ns;
}

const MultiItemSelector &MultiPadButton::get_selector() const
{
    return *ns;
}

std::string MultiPadButton::get_item_name(const UUID &uu) const
{
    auto &pads = ns->get_pads();
    if (pads.count(uu))
        return pads.at(uu);
    else
        return "?";
}

std::string MultiPadButton::get_label_text() const
{
    return MultiItemButton::get_label_text() + " (" + pkg_name + ")";
}

} // namespace horizon
