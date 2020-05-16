#pragma once
#include "util/pool_goto_provider.hpp"
#include "util/item_set.hpp"

namespace horizon {
class PoolEditorInterface : public PoolGotoProvider {
public:
    virtual void reload()
    {
    }
    virtual void save()
    {
        needs_save = false;
    }
    bool get_needs_save() const
    {
        return needs_save;
    }
    virtual void select(const ItemSet &items)
    {
    }

protected:
    bool needs_save = false;
};
} // namespace horizon
