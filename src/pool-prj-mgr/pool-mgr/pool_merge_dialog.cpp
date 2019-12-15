#include <widgets/cell_renderer_color_box.hpp>
#include "pool_merge_dialog.hpp"
#include "common/object_descr.hpp"
#include "pool/pool.hpp"
#include "pool/part.hpp"
#include "common/lut.hpp"
#include "util/sqlite.hpp"
#include "util/util.hpp"
#include <iostream>
#include <iomanip>
#include "nlohmann/json.hpp"

namespace horizon {

class PoolMergeBox : public Gtk::Box {
public:
    PoolMergeBox(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x);
    static PoolMergeBox *create();

    Gtk::TreeView *pool_item_view = nullptr;
    Gtk::Stack *stack = nullptr;
    Gtk::TextView *delta_text_view = nullptr;
    Gtk::CheckButton *cb_update_layer_help = nullptr;
    Gtk::CheckButton *cb_update_tables = nullptr;

private:
};

PoolMergeBox::PoolMergeBox(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x) : Gtk::Box(cobject)
{
    x->get_widget("pool_item_view", pool_item_view);
    x->get_widget("stack", stack);
    x->get_widget("delta_text_view", delta_text_view);
    x->get_widget("cb_update_layer_help", cb_update_layer_help);
    x->get_widget("cb_update_tables", cb_update_tables);
}

PoolMergeBox *PoolMergeBox::create()
{
    PoolMergeBox *w;
    Glib::RefPtr<Gtk::Builder> x = Gtk::Builder::create();
    x->add_from_resource("/org/horizon-eda/horizon/pool-prj-mgr/pool-mgr/pool_merge.ui");
    x->get_widget_derived("box", w);
    w->reference();
    return w;
}

void PoolMergeDialog::selection_changed()
{
    auto rows = box->pool_item_view->get_selection()->get_selected_rows();

    if (rows.size() == 1) {
        Gtk::TreeModel::Row row = *item_store->get_iter(rows.at(0));
        ItemState state = row[list_columns.state];
        ObjectType type = row[list_columns.type];
        if (state == ItemState::CURRENT) {
            box->stack->set_visible_child("up_to_date");
        }
        else if (state == ItemState::LOCAL_ONLY) {
            box->stack->set_visible_child("local_only");
        }
        else if (state == ItemState::REMOTE_ONLY) {
            box->stack->set_visible_child("remote_only");
        }
        else if (state == ItemState::MOVED) {
            box->stack->set_visible_child("moved");
        }
        else if (state == ItemState::CHANGED && type == ObjectType::MODEL_3D) {
            box->stack->set_visible_child("3d_changed");
        }
        else if (state == ItemState::CHANGED || state == ItemState::MOVED_CHANGED) {
            box->stack->set_visible_child("delta");
            const json &j = row.get_value(list_columns.delta);
            std::stringstream ss;
            ss << std::setw(4) << j;
            box->delta_text_view->get_buffer()->set_text(ss.str());
        }
    }
}

void PoolMergeDialog::action_toggled(const Glib::ustring &path)
{
    auto it = item_store->get_iter(path);
    if (it) {
        Gtk::TreeModel::Row row = *it;
        row[list_columns.merge] = !row[list_columns.merge];
    }
}


PoolMergeDialog::PoolMergeDialog(Gtk::Window *parent, const std::string &lp, const std::string &rp)
    : Gtk::Dialog("Merge Pool", *parent, Gtk::DialogFlags::DIALOG_MODAL | Gtk::DialogFlags::DIALOG_USE_HEADER_BAR),
      local_path(lp), remote_path(rp)
{
    add_button("Cancel", Gtk::ResponseType::RESPONSE_CANCEL);
    auto ok_button = add_button("Merge", Gtk::ResponseType::RESPONSE_OK);
    ok_button->signal_clicked().connect(sigc::mem_fun(*this, &PoolMergeDialog::do_merge));
    set_default_response(Gtk::ResponseType::RESPONSE_OK);
    set_default_size(1024, 600);

    box = PoolMergeBox::create();
    get_content_area()->pack_start(*box, true, true, 0);
    box->unreference();
    get_content_area()->set_border_width(0);


    item_store = Gtk::ListStore::create(list_columns);
    item_store->set_sort_func(list_columns.state,
                              [this](const Gtk::TreeModel::iterator &a, const Gtk::TreeModel::iterator &b) {
                                  Gtk::TreeModel::Row ra = *a;
                                  Gtk::TreeModel::Row rb = *b;
                                  ItemState sa = ra[list_columns.state];
                                  ItemState sb = rb[list_columns.state];
                                  return static_cast<int>(sb) - static_cast<int>(sa);
                              });
    box->pool_item_view->set_model(item_store);
    box->pool_item_view->get_selection()->signal_changed().connect(
            sigc::mem_fun(*this, &PoolMergeDialog::selection_changed));

    {
        auto cr_text = Gtk::manage(new Gtk::CellRendererText());
        auto cr_toggle = Gtk::manage(new Gtk::CellRendererToggle());
        auto tvc = Gtk::manage(new Gtk::TreeViewColumn("Action"));
        tvc->pack_start(*cr_toggle, false);
        tvc->pack_start(*cr_text, true);
        tvc->set_cell_data_func(*cr_text, [this](Gtk::CellRenderer *tcr, const Gtk::TreeModel::iterator &it) {
            Gtk::TreeModel::Row row = *it;
            auto mcr = dynamic_cast<Gtk::CellRendererText *>(tcr);
            switch (row[list_columns.state]) {
            case ItemState::CURRENT:
                mcr->property_text() = "Keep";
                break;

            case ItemState::CHANGED:
                mcr->property_text() = "Overwrite local";
                break;

            case ItemState::MOVED:
                mcr->property_text() = "Move";
                break;

            case ItemState::MOVED_CHANGED:
                mcr->property_text() = "Overwrite and move local";
                break;

            case ItemState::LOCAL_ONLY:
                mcr->property_text() = "Keep";
                break;

            case ItemState::REMOTE_ONLY:
                mcr->property_text() = "Add to local";
                break;

            default:
                mcr->property_text() = "fixme";
            }
        });
        tvc->set_cell_data_func(*cr_toggle, [this](Gtk::CellRenderer *tcr, const Gtk::TreeModel::iterator &it) {
            Gtk::TreeModel::Row row = *it;
            auto mcr = dynamic_cast<Gtk::CellRendererToggle *>(tcr);
            if (row[list_columns.state] == ItemState::CURRENT) {
                mcr->property_sensitive() = false;
                mcr->property_active() = true;
            }
            else if (row[list_columns.state] == ItemState::LOCAL_ONLY) {
                mcr->property_sensitive() = false;
                mcr->property_active() = true;
            }
            else {
                mcr->property_active() = row[list_columns.merge];
                mcr->property_sensitive() = true;
            }
        });
        cr_toggle->signal_toggled().connect(sigc::mem_fun(*this, &PoolMergeDialog::action_toggled));
        box->pool_item_view->append_column(*tvc);
    }

    {
        auto cr = Gtk::manage(new CellRendererColorBox());
        auto tvc = Gtk::manage(new Gtk::TreeViewColumn("State", *cr));
        auto cr2 = Gtk::manage(new Gtk::CellRendererText());
        cr2->property_text() = "hallo";

        tvc->set_cell_data_func(*cr2, [this](Gtk::CellRenderer *tcr, const Gtk::TreeModel::iterator &it) {
            Gtk::TreeModel::Row row = *it;
            auto mcr = dynamic_cast<Gtk::CellRendererText *>(tcr);
            switch (row[list_columns.state]) {
            case ItemState::CURRENT:
                mcr->property_text() = "Current";
                break;

            case ItemState::CHANGED:
                mcr->property_text() = "Changed";
                break;

            case ItemState::MOVED:
                mcr->property_text() = "Just moved";
                break;

            case ItemState::MOVED_CHANGED:
                mcr->property_text() = "Moved and changed";
                break;

            case ItemState::LOCAL_ONLY:
                mcr->property_text() = "Local only";
                break;

            case ItemState::REMOTE_ONLY:
                mcr->property_text() = "Remote only";
                break;

            default:
                mcr->property_text() = "fixme";
            }
        });
        tvc->set_cell_data_func(*cr, [this](Gtk::CellRenderer *tcr, const Gtk::TreeModel::iterator &it) {
            Gtk::TreeModel::Row row = *it;
            auto mcr = dynamic_cast<CellRendererColorBox *>(tcr);
            Color co(1, 0, 1);
            switch (row[list_columns.state]) {
            case ItemState::CURRENT:
                co = Color::new_from_int(138, 226, 52);
                break;

            case ItemState::MOVED:
            case ItemState::LOCAL_ONLY:
            case ItemState::REMOTE_ONLY:
                co = Color::new_from_int(252, 175, 62);
                break;

            default:
                co = Color::new_from_int(239, 41, 41);
            }
            Gdk::RGBA va;
            va.set_red(co.r);
            va.set_green(co.g);
            va.set_blue(co.b);
            mcr->property_color() = va;
        });
        tvc->pack_start(*cr2, false);
        tvc->set_sort_column(list_columns.state);
        box->pool_item_view->append_column(*tvc);
    }

    {
        auto cr = Gtk::manage(new Gtk::CellRendererText());
        auto tvc = Gtk::manage(new Gtk::TreeViewColumn("Type", *cr));
        tvc->set_cell_data_func(*cr, [this](Gtk::CellRenderer *tcr, const Gtk::TreeModel::iterator &it) {
            Gtk::TreeModel::Row row = *it;
            auto mcr = dynamic_cast<Gtk::CellRendererText *>(tcr);
            mcr->property_text() = object_descriptions.at(row[list_columns.type]).name;
        });
        box->pool_item_view->append_column(*tvc);
    }

    box->pool_item_view->append_column("Name", list_columns.name);

    {
        auto cr = Gtk::manage(new Gtk::CellRendererText());
        auto tvc = Gtk::manage(new Gtk::TreeViewColumn("Local filename", *cr));
        tvc->add_attribute(cr->property_text(), list_columns.filename_local);
        cr->property_ellipsize() = Pango::ELLIPSIZE_START;
        tvc->set_resizable(true);
        box->pool_item_view->append_column(*tvc);
    }
    {
        auto cr = Gtk::manage(new Gtk::CellRendererText());
        auto tvc = Gtk::manage(new Gtk::TreeViewColumn("Remote filename", *cr));
        tvc->add_attribute(cr->property_text(), list_columns.filename_remote);
        cr->property_ellipsize() = Pango::ELLIPSIZE_START;
        tvc->set_resizable(true);
        box->pool_item_view->append_column(*tvc);
    }

    item_store->set_sort_column(list_columns.state, Gtk::SORT_ASCENDING);

    {
        tables_remote = Glib::build_filename(remote_path, "tables.json");
        tables_local = Glib::build_filename(local_path, "tables.json");
        auto tables_remote_exist = Glib::file_test(tables_remote, Glib::FILE_TEST_IS_REGULAR);
        auto tables_local_exist = Glib::file_test(tables_local, Glib::FILE_TEST_IS_REGULAR);

        if (tables_remote_exist && !tables_local_exist) {
            box->cb_update_tables->set_active(true);
            box->cb_update_tables->set_sensitive(false);
        }
        else if (!tables_remote_exist) {
            box->cb_update_tables->set_active(false);
            box->cb_update_tables->set_sensitive(false);
        }
        else if (tables_remote_exist && tables_local_exist) {
            auto j_tables_remote = load_json_from_file(tables_remote);
            auto j_tables_local = load_json_from_file(tables_local);
            auto diff = json::diff(j_tables_local, j_tables_remote);
            if (diff.size() > 0) { // different
                box->cb_update_tables->set_active(true);
                box->cb_update_tables->set_sensitive(true);
            }
            else {
                box->cb_update_tables->set_active(false);
                box->cb_update_tables->set_sensitive(false);
            }
        }
    }

    {
        layer_help_remote = Glib::build_filename(remote_path, "layer_help");
        layer_help_local = Glib::build_filename(local_path, "layer_help");
        bool layer_help_remote_exist = Glib::file_test(layer_help_remote, Glib::FILE_TEST_IS_DIR);
        bool layer_help_local_exist = Glib::file_test(layer_help_local, Glib::FILE_TEST_IS_DIR);
        if (layer_help_remote_exist && !layer_help_local_exist) {
            box->cb_update_layer_help->set_active(true);
            box->cb_update_layer_help->set_sensitive(false);
        }
        else if (!layer_help_remote_exist) {
            box->cb_update_layer_help->set_active(false);
            box->cb_update_layer_help->set_sensitive(false);
        }
        else if (layer_help_remote_exist && layer_help_local_exist) {
            bool can_update = false;
            Glib::Dir dir_remote(layer_help_remote);
            for (const auto &it : dir_remote) {
                auto it_remote = Glib::build_filename(layer_help_remote, it);
                auto it_local = Glib::build_filename(layer_help_local, it);
                if (Glib::file_test(it_local, Glib::FILE_TEST_IS_REGULAR)) {
                    if (!compare_files(it_remote, it_local)) {
                        can_update = true;
                        break;
                    }
                }
                else {
                    can_update = true;
                    break;
                }
            }
            box->cb_update_layer_help->set_active(can_update);
            box->cb_update_layer_help->set_sensitive(can_update);
        }
    }


    populate_store();
}

bool PoolMergeDialog::get_merged() const
{
    return merged;
}

void PoolMergeDialog::do_merge()
{
    std::cout << "do merge" << std::endl;
    for (const auto &it : item_store->children()) {
        Gtk::TreeModel::Row row = *it;
        if (row[list_columns.merge]) {
            if (row[list_columns.state] == ItemState::REMOTE_ONLY) { // remote only, copy to local
                std::string filename = Glib::build_filename(local_path, row[list_columns.filename_remote]);
                std::string dirname = Glib::path_get_dirname(filename);
                if (!Glib::file_test(dirname, Glib::FILE_TEST_IS_DIR)) {
                    Gio::File::create_for_path(dirname)->make_directory_with_parents();
                }
                Gio::File::create_for_path(Glib::build_filename(remote_path, row[list_columns.filename_remote]))
                        ->copy(Gio::File::create_for_path(filename));
                merged = true;
            }
            else if (row[list_columns.state] == ItemState::MOVED) { // moved, move local item to new
                                                                    // filename
                std::string filename_src = Glib::build_filename(local_path, row[list_columns.filename_local]);
                std::string filename_dest = Glib::build_filename(local_path, row[list_columns.filename_remote]);
                std::string dirname_dest = Glib::path_get_dirname(filename_dest);
                if (!Glib::file_test(dirname_dest, Glib::FILE_TEST_IS_DIR)) {
                    Gio::File::create_for_path(dirname_dest)->make_directory_with_parents();
                }
                Gio::File::create_for_path(filename_src)->move(Gio::File::create_for_path(filename_dest));
                merged = true;
            }
            else if (row[list_columns.state] == ItemState::CHANGED
                     || row[list_columns.state] == ItemState::MOVED_CHANGED) {
                std::string filename_local = Glib::build_filename(local_path, row[list_columns.filename_local]);
                std::string filename_local_new = Glib::build_filename(local_path, row[list_columns.filename_remote]);
                std::string dirname_local_new = Glib::path_get_dirname(filename_local_new);
                std::string filename_remote = Glib::build_filename(remote_path, row[list_columns.filename_remote]);
                Gio::File::create_for_path(filename_local)->remove(); // remove local file (needed if
                                                                      // moved_changed)
                if (!Glib::file_test(dirname_local_new, Glib::FILE_TEST_IS_DIR)) {
                    Gio::File::create_for_path(dirname_local_new)->make_directory_with_parents();
                }
                Gio::File::create_for_path(filename_remote)->copy(Gio::File::create_for_path(filename_local_new));
                merged = true;
            }
        }
    }
    if (box->cb_update_tables->get_active()) {
        Gio::File::create_for_path(tables_remote)
                ->copy(Gio::File::create_for_path(tables_local), Gio::FILE_COPY_OVERWRITE);
        merged = true;
    }
    if (box->cb_update_layer_help->get_active()) {
        if (!Glib::file_test(layer_help_local, Glib::FILE_TEST_IS_DIR)) {
            Gio::File::create_for_path(layer_help_local)->make_directory_with_parents();
        }
        Glib::Dir dir_remote(layer_help_remote);
        for (const auto &it : dir_remote) {
            auto it_remote = Glib::build_filename(layer_help_remote, it);
            auto it_local = Glib::build_filename(layer_help_local, it);
            Gio::File::create_for_path(it_remote)->copy(Gio::File::create_for_path(it_local), Gio::FILE_COPY_OVERWRITE);
        }
        merged = true;
    }
}

void PoolMergeDialog::populate_store()
{
    Pool pool_local(local_path);
    {
        SQLite::Query q(pool_local.db, "ATTACH ? AS remote");
        q.bind(1, Glib::build_filename(remote_path, "pool.db"));
        q.step();
    }
    {
        SQLite::Query q(pool_local.db,
                        // items present in local+remote
                        "SELECT main.all_items_view.type, main.all_items_view.uuid, "
                        "main.all_items_view.name, main.all_items_view.filename, "
                        "remote.all_items_view.filename FROM main.all_items_view "
                        "INNER JOIN remote.all_items_view "
                        "ON (main.all_items_view.uuid = remote.all_items_view.uuid AND "
                        "main.all_items_view.type = remote.all_items_view.type) "
                        "UNION "

                        // items present remote only
                        "SELECT remote.all_items_view.type, "
                        "remote.all_items_view.uuid, remote.all_items_view.name, '', "
                        "remote.all_items_view.filename FROM remote.all_items_view "
                        "LEFT JOIN main.all_items_view "
                        "ON (main.all_items_view.uuid = remote.all_items_view.uuid AND "
                        "main.all_items_view.type = remote.all_items_view.type) "
                        "WHERE main.all_items_view.uuid IS NULL "
                        "UNION "

                        // items present local only
                        "SELECT main.all_items_view.type, main.all_items_view.uuid, "
                        "main.all_items_view.name, main.all_items_view.filename, '' "
                        "FROM main.all_items_view "
                        "LEFT JOIN remote.all_items_view "
                        "ON (main.all_items_view.uuid = remote.all_items_view.uuid AND "
                        "main.all_items_view.type = remote.all_items_view.type) "
                        "WHERE remote.all_items_view.uuid IS NULL");
        while (q.step()) {
            std::string type_str = q.get<std::string>(0);
            UUID uuid = q.get<std::string>(1);
            std::string name = q.get<std::string>(2);
            std::string filename_local = q.get<std::string>(3);
            std::string filename_remote = q.get<std::string>(4);

            Gtk::TreeModel::Row row;
            row = *item_store->append();
            row[list_columns.type] = object_type_lut.lookup(type_str);
            row[list_columns.uuid] = uuid;
            row[list_columns.name] = name;
            row[list_columns.merge] = true;
            row[list_columns.filename_local] = filename_local;
            row[list_columns.filename_remote] = filename_remote;

            if (filename_remote.size() == 0) {
                row[list_columns.state] = ItemState::LOCAL_ONLY;
            }
            else if (filename_local.size() == 0) {
                row[list_columns.state] = ItemState::REMOTE_ONLY;
            }
            else if (filename_local.size() && filename_remote.size()) {
                json j_local;
                {
                    j_local = load_json_from_file(Glib::build_filename(local_path, filename_local));
                    if (j_local.count("_imp"))
                        j_local.erase("_imp");
                }
                json j_remote;
                {
                    j_remote = load_json_from_file(Glib::build_filename(remote_path, filename_remote));
                    if (j_remote.count("_imp"))
                        j_remote.erase("_imp");
                }
                auto delta = json::diff(j_local, j_remote);
                row[list_columns.delta] = delta;

                bool moved = filename_local != filename_remote;
                bool changed = delta.size();
                if (moved && changed) {
                    row[list_columns.state] = ItemState::MOVED_CHANGED;
                }
                else if (moved && !changed) {
                    row[list_columns.state] = ItemState::MOVED;
                }
                else if (!moved && changed) {
                    row[list_columns.state] = ItemState::CHANGED;
                }
                else if (!moved && !changed) {
                    row[list_columns.state] = ItemState::CURRENT;
                }
            }
        }
    }
    {
        SQLite::Query q(pool_local.db, "SELECT DISTINCT model_filename FROM remote.models");
        while (q.step()) {
            std::string filename = q.get<std::string>(0);
            std::string remote_filename = Glib::build_filename(remote_path, filename);
            std::string local_filename = Glib::build_filename(local_path, filename);

            if (Glib::file_test(remote_filename, Glib::FILE_TEST_IS_REGULAR)) { // remote is there
                Gtk::TreeModel::Row row;
                row = *item_store->append();
                row[list_columns.type] = ObjectType::MODEL_3D;
                row[list_columns.name] = Glib::path_get_basename(filename);
                row[list_columns.filename_local] = filename;
                row[list_columns.filename_remote] = filename;
                row[list_columns.merge] = true;
                if (Glib::file_test(local_filename, Glib::FILE_TEST_IS_REGULAR)) {
                    if (compare_files(remote_filename, local_filename)) {
                        row[list_columns.state] = ItemState::CURRENT;
                    }
                    else {
                        row[list_columns.state] = ItemState::CHANGED;
                    }
                }
                else {
                    row[list_columns.state] = ItemState::REMOTE_ONLY;
                }
            }
        }
    }
}
} // namespace horizon
