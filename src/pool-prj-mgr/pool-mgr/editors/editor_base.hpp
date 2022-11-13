#pragma once
#include "util/pool_goto_provider.hpp"
#include "util/item_set.hpp"
#include <stdexcept>

namespace horizon {
class PoolEditorBase : public PoolGotoProvider {
public:
    virtual void reload()
    {
    }
    void save()
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

    typedef sigc::signal<void, std::string> type_signal_extra_file_saved;
    type_signal_extra_file_saved signal_extra_file_saved()
    {
        return s_signal_extra_file_saved;
    }

    type_signal_goto signal_open_item()
    {
        return s_signal_open_item;
    }

protected:
    void set_needs_save()
    {
        if (loading)
            throw std::runtime_error("set_needs_save called while loading");
        needs_save = true;
        s_signal_needs_save.emit();
    }

    class LoadingSetter {
        friend PoolEditorBase;
        LoadingSetter(PoolEditorBase &i) : iface(i)
        {
            iface.loading = true;
        }
        PoolEditorBase &iface;

    public:
        ~LoadingSetter()
        {
            iface.loading = false;
        }
    };
    friend LoadingSetter;

    [[nodiscard]] LoadingSetter set_loading()
    {
        return LoadingSetter(*this);
    }


    bool is_loading() const
    {
        return loading;
    }

    type_signal_extra_file_saved s_signal_extra_file_saved;
    type_signal_goto s_signal_open_item;

private:
    bool loading = false;
    bool needs_save = false;
    type_signal_needs_save s_signal_needs_save;
};
} // namespace horizon
