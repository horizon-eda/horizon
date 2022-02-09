#pragma once
#include "multi_item_button.hpp"

namespace horizon {

class MultiComponentButton : public MultiItemButton {
public:
    MultiComponentButton(const class Block &b);

protected:
    virtual class MultiItemSelector &get_selector() override;
    virtual const MultiItemSelector &get_selector() const override;
    virtual std::string get_item_name(const UUID &uu) const override;

private:
    const Block &block;
    class MultiComponentSelector *ns;
};
} // namespace horizon
