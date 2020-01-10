#pragma once
#include <gtkmm.h>
#include <memory>
#include <set>
#include "util/uuid.hpp"
#include "util/sort_controller.hpp"
#include "util/selection_provider.hpp"
#include "common/common.hpp"

namespace horizon {
class PoolBrowser : public Gtk::Box, public SelectionProvider {
public:
    PoolBrowser(class Pool *pool);
    UUID get_selected() override;
    bool get_any_selected();
    void set_show_none(bool v);
    void set_show_path(bool v);
    void add_context_menu_item(const std::string &label, sigc::slot1<void, UUID> cb);
    virtual void add_copy_name_context_menu_item(){};
    virtual void search() = 0;
    void search_once();
    void clear_search_once();
    virtual ObjectType get_type() const
    {
        return ObjectType::INVALID;
    };
    void go_to(const UUID &uu);
    void clear_search();

    enum class PoolItemSource { LOCAL, INCLUDED, OVERRIDING };

protected:
    void construct(Gtk::Widget *search_box = nullptr);
    class Pool *pool = nullptr;
    UUID pool_uuid;
    bool pools_included = false;
    bool show_none = false;
    bool show_path = false;
    Gtk::TreeViewColumn *path_column = nullptr;


    Gtk::TreeView *treeview = nullptr;
    Gtk::ScrolledWindow *scrolled_window = nullptr;

    Gtk::TreeViewColumn *append_column(const std::string &name, const Gtk::TreeModelColumnBase &column,
                                       Pango::EllipsizeMode ellipsize = Pango::ELLIPSIZE_NONE);
    Gtk::TreeViewColumn *append_column_with_item_source_cr(const std::string &name,
                                                           const Gtk::TreeModelColumnBase &column,
                                                           Pango::EllipsizeMode ellipsize = Pango::ELLIPSIZE_NONE);
    class CellRendererColorBox *create_pool_item_source_cr(Gtk::TreeViewColumn *tvc);
    void install_column_tooltip(Gtk::TreeViewColumn &tvc, const Gtk::TreeModelColumnBase &col);

    Gtk::Entry *create_search_entry(const std::string &label);
    class TagEntry *create_tag_entry(const std::string &label);
    void add_search_widget(const std::string &label, Gtk::Widget &w);


    virtual Glib::RefPtr<Gtk::ListStore> create_list_store() = 0;
    virtual void create_columns() = 0;
    virtual void add_sort_controller_columns() = 0;
    virtual UUID uuid_from_row(const Gtk::TreeModel::Row &row) = 0;

    Glib::RefPtr<Gtk::ListStore> store;
    std::unique_ptr<SortController> sort_controller;

    void row_activated(const Gtk::TreeModel::Path &path, Gtk::TreeViewColumn *column);
    void selection_changed();

    void select_uuid(const UUID &uu);
    void scroll_to_selection();

    Gtk::Menu context_menu;
    std::set<Gtk::Entry *> search_entries;
    std::set<TagEntry *> tag_entries;

    PoolItemSource pool_item_source_from_db(const UUID &uu, bool overridden);

    void install_pool_item_source_tooltip();
    virtual PoolItemSource pool_item_source_from_row(const Gtk::TreeModel::Row &row);
    bool searched_once = false;

    void set_busy(bool busy);
    void prepare_search();
    void finish_search();

    Gtk::Box *status_box = nullptr;

private:
    Gtk::Grid *grid = nullptr;
    int grid_top = 0;
    class CellRendererColorBox *cell_renderer_item_source = nullptr;
    Gtk::Box *busy_box = nullptr;
    UUID selected_uuid_before_search;

    Gtk::Label *status_label = nullptr;
};
} // namespace horizon
