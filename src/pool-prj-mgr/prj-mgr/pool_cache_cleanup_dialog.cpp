#include "pool_cache_cleanup_dialog.hpp"
#include "common/object_descr.hpp"
#include "pool/pool.hpp"
#include "pool/part.hpp"
#include "common/lut.hpp"
#include "util/sqlite.hpp"
#include <iostream>
#include "nlohmann/json.hpp"
#include "util/util.hpp"

namespace horizon {


void PoolCacheCleanupDialog::action_toggled(const Glib::ustring &path)
{
    auto it = item_store->get_iter(path);
    if (it) {
        Gtk::TreeModel::Row row = *it;
        row[list_columns.remove] = !row[list_columns.remove];
    }
}


PoolCacheCleanupDialog::PoolCacheCleanupDialog(Gtk::Window *parent, const std::set<std::string> &filenames,
                                               const std::set<std::string> &models_delete, class Pool *pool)
    : Gtk::Dialog("Clean up Pool Cache", *parent,
                  Gtk::DialogFlags::DIALOG_MODAL | Gtk::DialogFlags::DIALOG_USE_HEADER_BAR)

{
    add_button("Cancel", Gtk::ResponseType::RESPONSE_CANCEL);
    auto ok_button = add_button("Clean up", Gtk::ResponseType::RESPONSE_OK);
    ok_button->signal_clicked().connect(sigc::mem_fun(*this, &PoolCacheCleanupDialog::do_cleanup));
    set_default_response(Gtk::ResponseType::RESPONSE_OK);
    set_default_size(-1, 500);

    auto tv = Gtk::manage(new Gtk::TreeView);


    item_store = Gtk::ListStore::create(list_columns);
    tv->set_model(item_store);

    {
        auto cr_toggle = Gtk::manage(new Gtk::CellRendererToggle());
        cr_toggle->property_xalign() = 1;
        auto tvc = Gtk::manage(new Gtk::TreeViewColumn("Remove"));
        tvc->pack_start(*cr_toggle, false);
        tvc->add_attribute(cr_toggle->property_active(), list_columns.remove);
        cr_toggle->signal_toggled().connect(sigc::mem_fun(*this, &PoolCacheCleanupDialog::action_toggled));
        tv->append_column(*tvc);
    }
    {
        auto cr = Gtk::manage(new Gtk::CellRendererText());
        auto tvc = Gtk::manage(new Gtk::TreeViewColumn("Type", *cr));
        tvc->set_cell_data_func(*cr, [this](Gtk::CellRenderer *tcr, const Gtk::TreeModel::iterator &it) {
            Gtk::TreeModel::Row row = *it;
            auto mcr = dynamic_cast<Gtk::CellRendererText *>(tcr);
            mcr->property_text() = object_descriptions.at(row[list_columns.type]).name;
        });
        tv->append_column(*tvc);
    }
    tv->append_column("Name", list_columns.name);

    for (const auto &fi : filenames) {
        auto j_cache = load_json_from_file(fi);
        Gtk::TreeModel::Row row;
        row = *item_store->append();
        std::string type_str = j_cache.at("type");
        row[list_columns.filename] = fi;
        if (type_str == "part") {
            auto part = Part::new_from_file(fi, *pool);
            row[list_columns.name] = part.get_MPN() + " / " + part.get_manufacturer();
        }
        else {
            row[list_columns.name] = j_cache.at("name").get<std::string>();
        }
        ObjectType type = object_type_lut.lookup(type_str);
        row[list_columns.type] = type;
        row[list_columns.remove] = true;
    }
    for (const auto &fi : models_delete) {
        Gtk::TreeModel::Row row;
        row = *item_store->append();
        row[list_columns.filename] = fi;
        row[list_columns.name] = Glib::path_get_basename(fi);
        row[list_columns.type] = ObjectType::MODEL_3D;
        row[list_columns.remove] = true;
    }

    auto sc = Gtk::manage(new Gtk::ScrolledWindow);
    sc->set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC);
    sc->add(*tv);

    auto ol = Gtk::manage(new Gtk::Overlay);
    ol->add(*sc);

    if (filenames.size() == 0 && models_delete.size() == 0) {
        ok_button->set_sensitive(false);
        auto la = Gtk::manage(new Gtk::Label);
        la->set_markup("<i>There are no unused pool items in the cache.</i>");
        la->get_style_context()->add_class("dim-label");
        ol->add_overlay(*la);
        la->show();
    }

    get_content_area()->set_border_width(0);
    get_content_area()->pack_start(*ol, true, true, 0);
    get_content_area()->show_all();
}

void PoolCacheCleanupDialog::do_cleanup()
{
    for (auto &it : item_store->children()) {
        Gtk::TreeModel::Row row = *it;
        if (row[list_columns.remove]) {
            std::string filename = row[list_columns.filename];
            Gio::File::create_for_path(filename)->remove();
        }
    }
}

} // namespace horizon
