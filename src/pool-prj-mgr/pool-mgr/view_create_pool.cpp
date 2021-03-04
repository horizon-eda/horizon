#include "pool-prj-mgr/pool-prj-mgr-app_win.hpp"
#include "pool-prj-mgr/pool-prj-mgr-app.hpp"
#include "view_create_pool.hpp"
#include "nlohmann/json.hpp"
#include "util/util.hpp"
#include "pool/pool.hpp"
#include <fstream>

namespace horizon {

PoolProjectManagerViewCreatePool::PoolProjectManagerViewCreatePool(const Glib::RefPtr<Gtk::Builder> &builder,
                                                                   class PoolProjectManagerAppWindow *w)
{
    builder->get_widget("create_pool_name_entry", pool_name_entry);
    builder->get_widget("create_pool_path_chooser", pool_path_chooser);

    pool_name_entry->signal_changed().connect(sigc::mem_fun(*this, &PoolProjectManagerViewCreatePool::update));
    pool_path_chooser->signal_file_set().connect(sigc::mem_fun(*this, &PoolProjectManagerViewCreatePool::update));
}


void PoolProjectManagerViewCreatePool::clear()
{
    pool_name_entry->set_text("");
    pool_path_chooser->unselect_all();
}


static void mkdir_pool(const std::string &bp, const std::string &p)
{
    auto path = Glib::build_filename(bp, p);
    Gio::File::create_for_path(path)->make_directory();
    auto ofs = make_ofstream(Glib::build_filename(path, ".keep"));
    ofs.close();
}

std::pair<bool, std::string> PoolProjectManagerViewCreatePool::create()
{
    bool r = false;
    std::string s;
    auto base_path = pool_path_chooser->get_filename();


    try {
        {
            Glib::Dir dir(base_path);
            for (const auto &it : dir) {
                (void)sizeof it;
                throw std::runtime_error("base path is not empty");
            }
        }
        auto pool_json = Glib::build_filename(base_path, "pool.json");
        PoolInfo pool_info;
        pool_info.name = pool_name_entry->get_text();
        pool_info.default_via = UUID();
        pool_info.uuid = UUID::random();
        pool_info.base_path = base_path;
        pool_info.save();

        for (const auto &[type, name] : Pool::type_names) {
            mkdir_pool(base_path, name);
        }
        mkdir_pool(base_path, "3d_models");

        {
            auto ofs = make_ofstream(Glib::build_filename(base_path, ".gitignore"));
            ofs << "*.db\n\
tmp/*.json\n\
3d_models/local\n\
";
        }

        s = pool_json;
        r = true;
    }
    catch (const std::exception &e) {
        r = false;
        auto top = dynamic_cast<Gtk::Window *>(pool_name_entry->get_ancestor(GTK_TYPE_WINDOW));
        Gtk::MessageDialog md(*top, "Error creating pool", false /* use_markup */, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK);
        md.set_secondary_text(e.what());
        md.run();
    }
    catch (const Glib::Error &e) {
        r = false;
        auto top = dynamic_cast<Gtk::Window *>(pool_name_entry->get_ancestor(GTK_TYPE_WINDOW));
        Gtk::MessageDialog md(*top, "Error creating pool", false /* use_markup */, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK);
        md.set_secondary_text(e.what());
        md.run();
    }

    return {r, s};
} // namespace horizon

void PoolProjectManagerViewCreatePool::update()
{
    bool valid = true;
    if (pool_name_entry->get_text().size() == 0)
        valid = false;
    if (pool_path_chooser->get_filenames().size() == 0)
        valid = false;
    signal_valid_change().emit(valid);
}

} // namespace horizon
