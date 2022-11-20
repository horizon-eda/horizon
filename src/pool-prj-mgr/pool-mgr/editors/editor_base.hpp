#pragma once
#include "util/pool_goto_provider.hpp"
#include "util/item_set.hpp"
#include "rules/rules.hpp"
#include "util/history_manager.hpp"
#include <gtkmm.h>

namespace horizon {
class PoolEditorBase : public PoolGotoProvider {
public:
    PoolEditorBase(const std::string &fn, class IPool &apool) : filename(fn), pool(apool)
    {
    }

    virtual void reload()
    {
    }
    bool get_needs_save() const
    {
        return needs_save;
    }
    virtual void select(const ItemSet &items)
    {
    }

    virtual void save_as(const std::string &fn) = 0;
    virtual std::string get_name() const = 0;
    virtual const UUID &get_uuid() const = 0;
    virtual const class FileVersion &get_version() const = 0;
    virtual unsigned int get_required_version() const;
    virtual ObjectType get_type() const = 0;
    virtual RulesCheckResult run_checks() const = 0;

    std::string filename;

    void undo();
    void redo();

    bool can_undo() const;
    bool can_redo() const;

    void history_append(const std::string &comment);
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
    IPool &pool;

    HistoryManager history_manager;

    virtual std::unique_ptr<HistoryManager::HistoryItem> make_history_item(const std::string &comment) = 0;
    virtual void history_load(const HistoryManager::HistoryItem &it) = 0;

    void set_needs_save()
    {
        set_needs_save(false);
    }

    void unset_needs_save()
    {
        needs_save = false;
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

    void label_make_item_link(Gtk::Label &la, ObjectType type);

private:
    bool loading = false;
    bool needs_save = false;
    type_signal_needs_save s_signal_needs_save;

    void set_needs_save(bool from_undo)
    {
        if (loading)
            throw std::runtime_error("set_needs_save called while loading");
        needs_save = true;
        if (!from_undo)
            history_append("edit");
        s_signal_needs_save.emit();
    }
};
} // namespace horizon
