#pragma once
#include "multi_item_selector.hpp"

namespace horizon {
class MultiNetSelector : public MultiItemSelector {
public:
    MultiNetSelector(const class Block &block);

    void update() override;

protected:
    std::string get_column_heading() const override;

private:
    const Block &block;
};
} // namespace horizon
