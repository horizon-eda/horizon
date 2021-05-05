#pragma once

#include "pool_browser.hpp"

namespace horizon {
class PoolBrowserSymbol : public PoolBrowser {
public:
    PoolBrowserSymbol(class IPool &p, const UUID &unit_uuid = UUID(), const std::string &instance = "");
    void search() override;
    void set_unit_uuid(const UUID &uu);
    ObjectType get_type() const override
    {
        return ObjectType::SYMBOL;
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
            Gtk::TreeModelColumnRecord::add(name);
            Gtk::TreeModelColumnRecord::add(unit_name);
            Gtk::TreeModelColumnRecord::add(unit_manufacturer);
            Gtk::TreeModelColumnRecord::add(uuid);
            Gtk::TreeModelColumnRecord::add(path);
            Gtk::TreeModelColumnRecord::add(source);
        }
        Gtk::TreeModelColumn<Glib::ustring> name;
        Gtk::TreeModelColumn<Glib::ustring> unit_name;
        Gtk::TreeModelColumn<Glib::ustring> unit_manufacturer;
        Gtk::TreeModelColumn<Glib::ustring> path;
        Gtk::TreeModelColumn<UUID> uuid;
        Gtk::TreeModelColumn<PoolItemSource> source;
    };
    ListColumns list_columns;
    Gtk::Entry *name_entry = nullptr;
    UUID unit_uuid;
};
} // namespace horizon
