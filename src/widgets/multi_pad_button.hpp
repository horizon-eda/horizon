#pragma once
#include "multi_item_button.hpp"

namespace horizon {

class MultiPadButton : public MultiItemButton {
public:
    MultiPadButton();
    void set_package(const class Package &pkg);

protected:
    class MultiItemSelector &get_selector() override;
    const MultiItemSelector &get_selector() const override;
    std::string get_item_name(const UUID &uu) const override;

    std::string get_label_text() const override;

private:
    std::string pkg_name;
    class MultiPadSelector *ns;
};
} // namespace horizon
