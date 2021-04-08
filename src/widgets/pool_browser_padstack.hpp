#pragma once
#include "pool_browser.hpp"
#include "pool/padstack.hpp"

namespace horizon {
class PoolBrowserPadstack : public PoolBrowser {
public:
    PoolBrowserPadstack(class IPool &p);
    void search() override;
    void set_package_uuid(const UUID &uu);
    const std::set<Padstack::Type> &get_padstacks_included() const;
    void set_padstacks_included(const std::set<Padstack::Type> &types);
    ObjectType get_type() const override
    {
        return ObjectType::PADSTACK;
    }

protected:
    Glib::RefPtr<Gtk::ListStore> create_list_store() override;
    void create_columns() override;
    void add_sort_controller_columns() override;
    UUID uuid_from_row(const Gtk::TreeModel::Row &row) override;
    PoolItemSource pool_item_source_from_row(const Gtk::TreeModel::Row &row) override;

private:
    class ListColumns : public Gtk::TreeModelColumnRecord {
    public:
        ListColumns()
        {
            Gtk::TreeModelColumnRecord::add(padstack_name);
            Gtk::TreeModelColumnRecord::add(padstack_type);
            Gtk::TreeModelColumnRecord::add(package_name);
            Gtk::TreeModelColumnRecord::add(path);
            Gtk::TreeModelColumnRecord::add(uuid);
            Gtk::TreeModelColumnRecord::add(source);
        }
        Gtk::TreeModelColumn<Glib::ustring> padstack_name;
        Gtk::TreeModelColumn<Glib::ustring> padstack_type;
        Gtk::TreeModelColumn<Glib::ustring> package_name;
        Gtk::TreeModelColumn<Glib::ustring> path;
        Gtk::TreeModelColumn<UUID> uuid;
        Gtk::TreeModelColumn<PoolItemSource> source;
    };
    ListColumns list_columns;

    Gtk::Entry *name_entry = nullptr;
    UUID package_uuid;
    std::set<Padstack::Type> padstacks_included = {Padstack::Type::TOP, Padstack::Type::BOTTOM,
                                                   Padstack::Type::THROUGH};
};
} // namespace horizon
