#pragma once
#include "util/pool_goto_provider.hpp"

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
    bool get_needs_save()
    {
        return needs_save;
    }

protected:
    bool needs_save = false;
};
} // namespace horizon
