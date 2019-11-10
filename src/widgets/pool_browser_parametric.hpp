#pragma once
#include "pool_browser_stockinfo.hpp"
#include "pool/pool_parametric.hpp"
#include "util/stock_info_provider.hpp"

namespace horizon {
class PoolBrowserParametric : public PoolBrowserStockinfo {
public:
    class FilterAppliedLabel;
    friend FilterAppliedLabel;
    PoolBrowserParametric(class Pool *p, class PoolParametric *pp, const std::string &table_name);
    void search() override;
    ObjectType get_type() const override
    {
        return ObjectType::PART;
    }
    void add_copy_name_context_menu_item() override;
    void set_similar_part_uuid(const UUID &uu);
    void filter_similar(const UUID &uu, float tol = .1);

protected:
    Glib::RefPtr<Gtk::ListStore> create_list_store() override;
    void create_columns() override;
    void add_sort_controller_columns() override;
    UUID uuid_from_row(const Gtk::TreeModel::Row &row) override;
    PoolItemSource pool_item_source_from_row(const Gtk::TreeModel::Row &row) override;
    Gtk::TreeModelColumn<std::shared_ptr<StockInfoRecord>> &get_stock_info_column() override;

private:
    class PoolParametric *pool_parametric;
    const PoolParametric::Table &table;
    std::map<std::string, std::reference_wrapper<const PoolParametric::Column>> columns;
    UUID similar_part_uuid;

    class ListColumns : public Gtk::TreeModelColumnRecord {
    public:
        ListColumns(const PoolParametric::Table &table)
        {
            Gtk::TreeModelColumnRecord::add(MPN);
            Gtk::TreeModelColumnRecord::add(manufacturer);
            Gtk::TreeModelColumnRecord::add(package);
            Gtk::TreeModelColumnRecord::add(uuid);
            Gtk::TreeModelColumnRecord::add(path);
            Gtk::TreeModelColumnRecord::add(source);
            for (const auto &col : table.columns) {
                Gtk::TreeModelColumnRecord::add(params[col.name]);
            }
            Gtk::TreeModelColumnRecord::add(stock_info);
        }
        Gtk::TreeModelColumn<Glib::ustring> MPN;
        Gtk::TreeModelColumn<Glib::ustring> manufacturer;
        Gtk::TreeModelColumn<Glib::ustring> package;
        Gtk::TreeModelColumn<Glib::ustring> path;
        Gtk::TreeModelColumn<UUID> uuid;
        Gtk::TreeModelColumn<PoolItemSource> source;
        std::map<std::string, Gtk::TreeModelColumn<std::string>> params;
        Gtk::TreeModelColumn<std::shared_ptr<StockInfoRecord>> stock_info;
    };
    ListColumns list_columns;

    Gtk::Box *search_box = nullptr;
    Gtk::Box *filters_applied_box = nullptr;
    std::map<std::string, std::set<std::string>> values_remaining;
    std::map<std::string, std::set<std::string>> filters_applied;
    std::map<std::string, class ParametricFilterBox *> filter_boxes;
    void apply_filters();
    void update_filters();
    void update_filters_applied();
    void remove_filter(const std::string &col);
};
} // namespace horizon
