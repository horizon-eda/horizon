#include "close_utils.hpp"
#include "pool-prj-mgr-app_win.hpp"

namespace horizon {

ConfirmCloseDialog::ConfirmCloseDialog(Gtk::Window *parent)
    : Gtk::MessageDialog(*parent, "Save changes before closing?", false, Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_NONE)
{
    add_button("Close without Saving", RESPONSE_NO_SAVE);
    add_button("Cancel", Gtk::RESPONSE_CANCEL);
    add_button("Save selected", RESPONSE_SAVE);

    set_secondary_text("If you don't save, all your changes will be permanently lost.");

    store = Gtk::TreeStore::create(tree_columns);

    tv = Gtk::manage(new Gtk::TreeView(store));
    tv->set_headers_visible(false);
    {
        auto cr_text = Gtk::manage(new Gtk::CellRendererText());
        auto cr_toggle = Gtk::manage(new Gtk::CellRendererToggle());
        auto tvc = Gtk::manage(new Gtk::TreeViewColumn("File"));
        tvc->pack_start(*cr_toggle, false);
        tvc->pack_start(*cr_text, true);
        tvc->add_attribute(cr_text->property_markup(), tree_columns.display_name);
        tvc->add_attribute(cr_toggle->property_active(), tree_columns.save);
        tvc->add_attribute(cr_toggle->property_inconsistent(), tree_columns.inconsistent);
        tvc->add_attribute(cr_toggle->property_sensitive(), tree_columns.sensitive);

        tv->append_column(*tvc);
        cr_toggle->signal_toggled().connect([this](const Glib::ustring &path) {
            auto it = store->get_iter(path);
            if (it) {
                Gtk::TreeModel::Row row = *it;
                if (row[tree_columns.inconsistent]) {
                    row[tree_columns.save] = true;
                    row[tree_columns.inconsistent] = false;
                }
                else {
                    row[tree_columns.save] = !row[tree_columns.save];
                }
                if (auto it_parent = it->parent()) { // leaf node (filename)
                    auto children = it_parent->children();
                    int v = -1;
                    bool inconsistent = false;
                    for (auto &ch : children) {
                        Gtk::TreeModel::Row row_ch = *ch;
                        bool save = row_ch[tree_columns.save];
                        if (v != -1) {
                            if (v != save) {
                                inconsistent = true;
                                break;
                            }
                        }
                        else { // first one
                            v = save;
                        }
                    }
                    Gtk::TreeModel::Row row_parent = *it_parent;
                    row_parent[tree_columns.inconsistent] = inconsistent;
                    row_parent[tree_columns.save] = v;
                }
                else {
                    auto children = it->children();
                    for (auto &ch : children) {
                        Gtk::TreeModel::Row row_ch = *ch;
                        if (row_ch[tree_columns.sensitive])
                            row_ch[tree_columns.save] = static_cast<bool>(row[tree_columns.save]);
                    }
                }
            }
        });
        // box->pool_item_view->append_column(*tvc);
    }


    auto sc = Gtk::manage(new Gtk::ScrolledWindow);
    sc->set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC);
    sc->set_shadow_type(Gtk::SHADOW_IN);
    sc->property_margin() = 10;
    sc->set_propagate_natural_height(true);
    sc->add(*tv);


    get_content_area()->pack_start(*sc, true, true, 0);
    get_content_area()->show_all();
    get_content_area()->set_spacing(0);
}

void ConfirmCloseDialog::set_files(std::map<std::string, std::map<UUID, std::string>> &files)
{
    store->clear();
    Gtk::TreeModel::Row row;
    for (const auto &it : files) {
        auto itt = store->append();
        row = *itt;
        row[tree_columns.name] = it.first;
        row[tree_columns.display_name] = Glib::Markup::escape_text(it.first);
        auto dir_parent = Gio::File::create_for_path(it.first)->get_parent();
        row[tree_columns.save] = true;
        row[tree_columns.inconsistent] = false;
        row[tree_columns.sensitive] = true;
        for (const auto &it2 : it.second) {
            auto itt2 = store->append(itt->children());
            row = *itt2;
            row[tree_columns.name] = it2.second;
            row[tree_columns.uuid] = it2.first;
            row[tree_columns.sensitive] = true;
            row[tree_columns.save] = true;
            if (it2.second.size()) {
                std::string p = dir_parent->get_relative_path(Gio::File::create_for_path(it2.second));
                if (p.size() == 0) {
                    p = it2.second;
                    row[tree_columns.sensitive] = false;
                    row[tree_columns.save] = false;
                }
                row[tree_columns.display_name] = Glib::Markup::escape_text(p);
            }
            else {
                row[tree_columns.display_name] = "<i>not saved yet</i>";
                row[tree_columns.sensitive] = false;
                row[tree_columns.save] = false;
            }
            row[tree_columns.inconsistent] = false;
        }
    }
    tv->expand_all();
}

std::map<std::string, std::set<UUID>> ConfirmCloseDialog::get_files() const
{
    std::map<std::string, std::set<UUID>> r;
    Gtk::TreeModel::Row row;
    for (const auto &it : store->children()) {
        row = *it;
        auto &w = r[static_cast<std::string>(static_cast<Glib::ustring>(row[tree_columns.name]))];
        for (const auto &it2 : it->children()) {
            row = *it2;
            if (row[tree_columns.save])
                w.insert(row.get_value(tree_columns.uuid));
        }
    }
    return r;
}


ProcWaitDialog::ProcWaitDialog(PoolProjectManagerAppWindow *parent) : Gtk::Dialog("Closing", *parent, Gtk::DIALOG_MODAL)
{
    auto hb = Gtk::manage(new Gtk::HeaderBar);
    hb->set_show_close_button(false);
    hb->set_title("Closing");
    set_titlebar(*hb);
    hb->show_all();

    auto box2 = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 10));
    auto la = Gtk::manage(new Gtk::Label("Waiting"));
    box2->pack_start(*la, true, true, 0);
    auto sp = Gtk::manage(new Gtk::Spinner);
    sp->start();
    box2->pack_start(*sp, false, false, 0);

    box2->property_margin() = 10;
    box2->show_all();
    get_content_area()->pack_start(*box2, true, true, 0);

    parent->signal_process_exited().connect(sigc::track_obj(
            [parent, this](std::string filename, int status, bool needs_save) {
                if (parent->get_processes().size() == 0)
                    response(1);
            },
            *this));
}

} // namespace horizon
