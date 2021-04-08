#pragma once
#include "generic_combo_box.hpp"
#include "util/uuid.hpp"

namespace horizon {
class PoolSelector : public GenericComboBox<UUID> {
public:
    PoolSelector(class IPool &pool);
    UUID get_selected_pool() const;
    void reload();

private:
    void populate();
    IPool &pool;
};

} // namespace horizon
