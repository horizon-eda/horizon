#pragma once
#include "multi_item_selector.hpp"

namespace horizon {
class MultiPadSelector : public MultiItemSelector {
public:
    MultiPadSelector();
    void set_package(const class Package &pkg);
    const std::map<UUID, std::string> &get_pads() const
    {
        return pads;
    }

    void update() override;

protected:
    std::string get_column_heading() const override;

private:
    std::map<UUID, std::string> pads;
};
} // namespace horizon
