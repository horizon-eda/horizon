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
        s_signal_needs_save.emit();
    }
    bool get_needs_save() const
    {
        return needs_save;
    }
    virtual void select(const ItemSet &items)
    {
    }

    typedef sigc::signal<void> type_signal_needs_save;
    type_signal_needs_save signal_needs_save()
    {
        return s_signal_needs_save;
    }

protected:
    void set_needs_save()
    {
        needs_save = true;
        s_signal_needs_save.emit();
    }

private:
    bool needs_save = false;
    type_signal_needs_save s_signal_needs_save;
};
} // namespace horizon
