#pragma once
#include "multi_item_button.hpp"

namespace horizon {

class MultiNetButton : public MultiItemButton {
public:
    MultiNetButton(const class Block &b);

protected:
    virtual class MultiItemSelector &get_selector() override;
    virtual const MultiItemSelector &get_selector() const override;
    virtual std::string get_item_name(const UUID &uu) const override;

private:
    const Block &block;
    class MultiNetSelector *ns;
};
} // namespace horizon
