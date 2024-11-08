#include "pool-prj-mgr-app_win.hpp"
#include "pool-prj-mgr-app.hpp"
#include "preferences/preferences_window.hpp"
#include "pools_window/pools_window.hpp"
#include <iostream>
#include "util/util.hpp"
#include "pool/pool_manager.hpp"
#include "pool/project_pool.hpp"
#include "pool-mgr/pool_notebook.hpp"
#include "pool-mgr/pool_settings_box.hpp"
#include "util/pool_check_schema_update.hpp"
#include "util/gtk_util.hpp"
#include "widgets/recent_item_box.hpp"
#include "nlohmann/json.hpp"
#include "pool-mgr/editors/editor_window.hpp"
#include "prj-mgr/part_browser/part_browser_window.hpp"
#include "prj-mgr/pool_cache_cleanup_dialog.hpp"
#include "close_utils.hpp"
#include "schematic/schematic.hpp"
#include "board/board.hpp"
#include "welcome_window.hpp"
#include "output_window.hpp"
#include "util/str_util.hpp"
#include "autosave_recovery_dialog.hpp"
#include "util/item_set.hpp"
#include "pool/pool_cache_status.hpp"
#include "util/zmq_helper.hpp"
#include "pool-update/pool-update.hpp"
#include "pool_update_error_dialog.hpp"
#include "project/project.hpp"
#include "blocks/blocks_schematic.hpp"
#include "project/project.hpp"
#include <filesystem>
#include "util/fs_util.hpp"
#include "widgets/sqlite_shell.hpp"
#include "widgets/forced_pool_update_dialog.hpp"

#ifdef G_OS_WIN32
#include <winsock2.h>
#endif
#include "util/win32_undef.hpp"

namespace horizon {
namespace fs = std::filesystem;
PoolProjectManagerAppWindow::PoolProjectManagerAppWindow(BaseObjectType *cobject,
                                                         const Glib::RefPtr<Gtk::Builder> &refBuilder,
                                                         PoolProjectManagerApplication &aapp)
    : Gtk::ApplicationWindow(cobject), app(aapp), builder(refBuilder), state_store(this, "pool-mgr"),
      view_create_project(refBuilder, *this), view_project(refBuilder, *this), view_create_pool(refBuilder),
      sock_mgr(app.zctx, ZMQ_REP), zctx(app.zctx)
{
    builder->get_widget("stack", stack);
    builder->get_widget("button_open", button_open);
    builder->get_widget("button_close", button_close);
    builder->get_widget("button_update", button_update);
    builder->get_widget("button_cancel", button_cancel);
    builder->get_widget("button_create", button_create);
    builder->get_widget("button_new", button_new);
    builder->get_widget("button_save", button_save);
    builder->get_widget("header", header);
    builder->get_widget("recent_pools_listbox", recent_pools_listbox);
    builder->get_widget("recent_projects_listbox", recent_projects_listbox);
    builder->get_widget("recent_pools_search_entry", recent_pools_search_entry);
    builder->get_widget("recent_projects_search_entry", recent_projects_search_entry);
    builder->get_widget("recent_pools_placeholder_stack", recent_pools_placeholder_stack);
    builder->get_widget("recent_projects_placeholder_stack", recent_projects_placeholder_stack);
    builder->get_widget("pool_box", pool_box);
    builder->get_widget("pool_update_status_label", pool_update_status_label);
    builder->get_widget("pool_update_status_rev", pool_update_status_rev);
    builder->get_widget("pool_update_status_close_button", pool_update_status_close_button);
    builder->get_widget("pool_update_progress", pool_update_progress);
    builder->get_widget("info_bar", info_bar);
    builder->get_widget("info_bar_label", info_bar_label);
    builder->get_widget("show_output_button", show_output_button);
    builder->get_widget("menu_new_pool", menu_new_pool);
    builder->get_widget("menu_new_project", menu_new_project);
    builder->get_widget("hamburger_menu_button", hamburger_menu_button);
    builder->get_widget("info_bar_pool_not_added", info_bar_pool_not_added);
    builder->get_widget("info_bar_pool_doc", info_bar_pool_doc);
    builder->get_widget("info_bar_version", info_bar_version);
    builder->get_widget("version_label", version_label);
    builder->get_widget("info_bar_gitignore", info_bar_gitignore);
    builder->get_widget("info_bar_installation_uuid_mismatch", info_bar_installation_uuid_mismatch);

    set_view_mode(ViewMode::OPEN);

    {
        auto rb = Gtk::Builder::create();
        rb->add_from_resource("/org/horizon-eda/horizon/pool-prj-mgr/app_menu.ui");

        auto object = rb->get_object("appmenu");
        auto app_menu = Glib::RefPtr<Gio::MenuModel>::cast_dynamic(object);

        hamburger_menu_button->set_menu_model(app_menu);
    }

    pool_update_progress->set_pulse_step(.001);

    button_open->signal_clicked().connect(sigc::mem_fun(*this, &PoolProjectManagerAppWindow::handle_open));
    button_close->signal_clicked().connect(sigc::mem_fun(*this, &PoolProjectManagerAppWindow::handle_close));
    button_update->signal_clicked().connect(sigc::mem_fun(*this, &PoolProjectManagerAppWindow::handle_update));
    button_cancel->signal_clicked().connect(sigc::mem_fun(*this, &PoolProjectManagerAppWindow::handle_cancel));
    menu_new_project->signal_activate().connect(sigc::mem_fun(*this, &PoolProjectManagerAppWindow::handle_new_project));
    menu_new_pool->signal_activate().connect(sigc::mem_fun(*this, &PoolProjectManagerAppWindow::handle_new_pool));
    button_create->signal_clicked().connect(sigc::mem_fun(*this, &PoolProjectManagerAppWindow::handle_create));
    button_save->signal_clicked().connect(sigc::mem_fun(*this, &PoolProjectManagerAppWindow::handle_save));


    recent_listboxes.push_back(recent_pools_listbox);
    recent_listboxes.push_back(recent_projects_listbox);

    pool_update_status_close_button->signal_clicked().connect(
            [this] { pool_update_status_rev->set_reveal_child(false); });


    info_bar->signal_response().connect([this](int resp) {
        if (resp == Gtk::RESPONSE_CLOSE) {
            info_bar_hide(info_bar);
        }
    });
    info_bar_hide(info_bar);

    info_bar_pool_not_added->signal_response().connect([this](int resp) {
        if (resp == Gtk::RESPONSE_OK) {
            if (pool) {
                auto pools_window = app.show_pools_window();
                pools_window->add_pool(Glib::build_filename(pool->get_base_path(), "pool.json"));
            }
        }
        info_bar_hide(info_bar_pool_not_added);
    });
    info_bar_hide(info_bar_pool_not_added);

    info_bar_pool_doc->signal_response().connect([this](int resp) {
        if (resp == Gtk::RESPONSE_OK) {
            info_bar_hide(info_bar_pool_doc);
            app.user_config.pool_doc_info_bar_dismissed = true;
        }
    });
    info_bar_hide(info_bar_pool_doc);

    info_bar_hide(info_bar_gitignore);
    info_bar_gitignore->signal_response().connect([this](int resp) {
        if (resp == Gtk::RESPONSE_OK) {
            if (project) {
                Project::fix_gitignore(get_gitignore_filename());
            }
        }
        info_bar_hide(info_bar_gitignore);
    });

    info_bar_hide(info_bar_installation_uuid_mismatch);
    info_bar_installation_uuid_mismatch->signal_response().connect(
            [this](int resp) { info_bar_hide(info_bar_installation_uuid_mismatch); });

    show_output_button->signal_clicked().connect([this] {
        output_window->present();
        info_bar_hide(info_bar);
    });

    view_create_project.signal_valid_change().connect([this](bool v) { button_create->set_sensitive(v); });
    view_create_pool.signal_valid_change().connect([this](bool v) { button_create->set_sensitive(v); });

    sock_mgr.bind("tcp://127.0.0.1:*");
    sock_mgr_ep = zmq_helper::get_last_endpoint(sock_mgr);

    auto chan = zmq_helper::io_channel_from_socket(sock_mgr);

    Glib::signal_io().connect(sigc::track_obj(
                                      [this](Glib::IOCondition cond) {
                                          while (zmq_helper::can_recv(sock_mgr)) {
                                              zmq::message_t msg;
                                              if (zmq_helper::recv(sock_mgr, msg)) {
                                                  char *data = (char *)msg.data();
                                                  if (msg.size() < (UUID::size + 1))
                                                      throw std::runtime_error("received short message");
                                                  if (memcmp(data, app.ipc_cookie.get_bytes(), UUID::size) != 0)
                                                      throw std::runtime_error("IPC cookie didn't match");
                                                  json jrx = json::parse(data + UUID::size);
                                                  json jtx = handle_req(jrx);

                                                  std::string stx = jtx.dump();
                                                  zmq::message_t tx(stx.size() + 1);
                                                  memcpy(((uint8_t *)tx.data()), stx.c_str(), stx.size());
                                                  auto m = (char *)tx.data();
                                                  m[tx.size() - 1] = 0;
                                                  zmq_helper::send(sock_mgr, tx);
                                              }
                                          }
                                          return true;
                                      },
                                      *this),
                              chan, Glib::IO_IN | Glib::IO_HUP);


    Glib::signal_idle().connect_once(sigc::track_obj(
            [this] {
                update_recent_items();
                if (PoolManager::get().get_pools().size() == 0) {
                    auto w = WelcomeWindow::create(*this);
                    w->set_modal(true);
                    w->present();
                }
            },
            *this));

    for (auto &lb : recent_listboxes) {
        lb->set_header_func(sigc::ptr_fun(header_func_separator));
        lb->signal_row_activated().connect([this](Gtk::ListBoxRow *row) {
            auto ch = dynamic_cast<RecentItemBox *>(row->get_child());
            open_file_view(Gio::File::create_for_path(ch->path));
        });
    }

    output_window = OutputWindow::create();
    output_window->set_transient_for(*this);

    signal_key_press_event().connect([this](const GdkEventKey *ev) {
        if (ev->keyval == GDK_KEY_O && (ev->state & GDK_CONTROL_MASK)) {
            output_window->present();
            return true;
        }
        return false;
    });

    signal_key_press_event().connect([this](const GdkEventKey *ev) {
        if (ev->keyval == GDK_KEY_S && (ev->state & GDK_CONTROL_MASK)) {
            if (pool || project) {
                auto w = new SQLiteShellWindow(get_pool_base_path() + "/pool.db");
                w->set_transient_for(*this);
                w->signal_hide().connect([w] { delete w; });
                w->present();
                return true;
            }
        }
        return false;
    });

    recent_pools_search_entry->signal_search_changed().connect(
            [this] { update_recent_search(*recent_pools_search_entry, *recent_pools_listbox); });
    recent_projects_search_entry->signal_search_changed().connect(
            [this] { update_recent_search(*recent_projects_search_entry, *recent_projects_listbox); });
    recent_projects_search_entry->grab_focus();

    pool_update_dispatcher.connect(sigc::mem_fun(*this, &PoolProjectManagerAppWindow::handle_pool_update_progress));
    app.signal_pool_items_edited().connect(
            sigc::track_obj([this](const auto &filenames) { pool_update(filenames); }, *this));
    app.signal_pool_updated().connect(sigc::track_obj(
            [this](const auto &path) {
                if (part_browser_window)
                    part_browser_window->pool_updated(path);
                if (project && project->pool_directory == path)
                    view_project.update_pools_label();
                {
                    json j;
                    j["op"] = "pool-updated";
                    j["path"] = path;
                    app.send_json(0, j);
                }
            },
            *this));

    {
        Gtk::Button *button;
        builder->get_widget("manage_pools_button", button);
        button->signal_clicked().connect([this] { app.show_pools_window(); });
    }
    {
        Gtk::Button *button;
        builder->get_widget("open_project_button", button);
        button->signal_clicked().connect(sigc::mem_fun(*this, &PoolProjectManagerAppWindow::handle_open));
    }
    {
        Gtk::Button *button;
        builder->get_widget("new_project_button", button);
        button->signal_clicked().connect(sigc::mem_fun(*this, &PoolProjectManagerAppWindow::handle_new_project));
    }

    app.signal_recent_items_changed().connect(sigc::mem_fun(*this, &PoolProjectManagerAppWindow::update_recent_items));

    check_mtimes_dispatcher.connect([this] { pool_update(); });
}


class SetReset {
public:
    SetReset(bool &v) : value(v)
    {
        value = true;
    }

    ~SetReset()
    {
        value = false;
    }

private:
    bool &value;
};

void PoolProjectManagerAppWindow::handle_pool_update_progress()
{
    if (in_pool_update_handler)
        return;
    SetReset rst(in_pool_update_handler);
    decltype(pool_update_status_queue) my_queue;
    {
        std::lock_guard<std::mutex> guard(pool_update_status_queue_mutex);
        my_queue.splice(my_queue.begin(), pool_update_status_queue);
    }
    if (my_queue.size()) {
        const auto last_info = pool_update_last_info;
        for (const auto &[last_status, last_filename, last_msg] : my_queue) {
            if (last_status == PoolUpdateStatus::DONE) {
                pool_updated(true);
                if (pool_update_filenames.size() == 0) // only for full update
                    pool_update_n_files_last = pool_update_n_files;
            }
            else if (last_status == PoolUpdateStatus::FILE) {
                pool_update_last_file = last_filename;
                pool_update_n_files++;
            }
            else if (last_status == PoolUpdateStatus::INFO) {
                pool_update_last_info = last_msg;
            }
            else if (last_status == PoolUpdateStatus::FILE_ERROR) {
                pool_update_error_queue.emplace_back(last_status, last_filename, last_msg);
            }
            else if (last_status == PoolUpdateStatus::ERROR) {
                set_pool_update_status_text(last_msg + " Last file: " + pool_update_last_file);
                pool_updated(false);
                return;
            }
        }
        if (pool_update_last_info != last_info)
            set_pool_update_status_text(pool_update_last_info);
        if (pool_update_n_files_last && pool_update_filenames.size() == 0) { // only for full update
            set_pool_update_progress((float)pool_update_n_files / pool_update_n_files_last);
        }
        else {
            set_pool_update_progress(-1);
        }
    }
}

void PoolProjectManagerAppWindow::pool_updated(bool success)
{
    pool_updating = false;
    if (pool_update_error_queue.size()) {
        auto top = dynamic_cast<Gtk::Window *>(get_ancestor(GTK_TYPE_WINDOW));
        PoolUpdateErrorDialog dia(top, pool_update_error_queue);
        dia.run();
    }
    set_pool_updating(false, success);
    if (success) {
        app.signal_pool_updated().emit(get_pool_base_path());
    }
}

void PoolProjectManagerAppWindow::pool_update(const std::vector<std::string> &filenames)
{
    if (pool_updating)
        return;

    if (!(pool || project))
        return;

    if (project) { // if there's an open pool window, delegate pool update to that one
        auto path = Glib::build_filename(project->pool_directory, "pool.json");
        for (auto ws : app.get_windows()) {
            auto wi = dynamic_cast<PoolProjectManagerAppWindow *>(ws);
            if (wi && wi->get_filename() == path) {
                return;
            }
        }
    }

    set_pool_updating(true, true);
    pool_update_n_files = 0;
    if (pool_update_n_files_last == 0) { // use estimate
        Pool my_pool(get_pool_base_path());
        SQLite::Query q(my_pool.db,
                        "SELECT SUM(x) FROM (SELECT COUNT(*) AS x FROM parts WHERE parametric_table != '' "
                        "UNION ALL SELECT COUNT(*) AS x FROM all_items_view)");
        if (q.step())
            pool_update_n_files_last = q.get<int>(0);
    }
    pool_updating = true;
    pool_update_filenames = filenames;
    pool_update_status_queue.clear();
    pool_update_error_queue.clear();
    std::thread thr(&PoolProjectManagerAppWindow::pool_update_thread, this);
    thr.detach();
}

void PoolProjectManagerAppWindow::pool_update_thread()
{
    std::cout << "hello from thread" << std::endl;

    try {
        clock_t begin = clock();
        horizon::pool_update(
                get_pool_base_path(),
                [this](PoolUpdateStatus st, std::string filename, std::string msg) {
                    {
                        std::lock_guard<std::mutex> guard(pool_update_status_queue_mutex);
                        pool_update_status_queue.emplace_back(st, filename, msg);
                    }
                    pool_update_dispatcher.emit();
                },
                true, pool_update_filenames);
        clock_t end = clock();
        double elapsed_secs = double(end - begin) / CLOCKS_PER_SEC;
        std::cout << "pool update took " << elapsed_secs << std::endl;
    }
    catch (const std::runtime_error &e) {
        {
            std::lock_guard<std::mutex> guard(pool_update_status_queue_mutex);
            pool_update_status_queue.emplace_back(PoolUpdateStatus::ERROR, "",
                                                  std::string("runtime exception: ") + e.what());
        }
        pool_update_dispatcher.emit();
    }
    catch (const std::exception &e) {
        {
            std::lock_guard<std::mutex> guard(pool_update_status_queue_mutex);
            pool_update_status_queue.emplace_back(PoolUpdateStatus::ERROR, "",
                                                  std::string("generic exception: ") + e.what());
        }
        pool_update_dispatcher.emit();
    }
    catch (const Glib::FileError &e) {
        {
            std::lock_guard<std::mutex> guard(pool_update_status_queue_mutex);
            pool_update_status_queue.emplace_back(PoolUpdateStatus::ERROR, "",
                                                  std::string("file exception: ") + e.what());
        }
        pool_update_dispatcher.emit();
    }
    catch (...) {
        {
            std::lock_guard<std::mutex> guard(pool_update_status_queue_mutex);
            pool_update_status_queue.emplace_back(PoolUpdateStatus::ERROR, "", "unknown exception");
        }
        pool_update_dispatcher.emit();
    }
}


PoolProjectManagerProcess *PoolProjectManagerAppWindow::find_process(const std::string &filename)
{
    for (auto &it : processes) {
        if (it.second.get_filename() == filename)
            return &it.second;
    }
    return nullptr;
}

PoolProjectManagerProcess *PoolProjectManagerAppWindow::find_top_schematic_process()
{
    if (!project)
        return nullptr;
    return find_process(project->blocks_filename);
}

PoolProjectManagerProcess *PoolProjectManagerAppWindow::find_board_process()
{
    if (!project)
        return nullptr;
    return find_process(project->board_filename);
}

bool PoolProjectManagerAppWindow::check_pools()
{
    if (PoolManager::get().get_pools().size() == 0) {
        Gtk::MessageDialog md(*this, "No pools set up", false, Gtk::MESSAGE_INFO, Gtk::BUTTONS_OK);
        md.set_secondary_text(
                "You haven't set up any pools, add some in the preferences "
                "dialog");
        md.run();
        return false;
    }
    return true;
}

json PoolProjectManagerAppWindow::handle_req(const json &j)
{
    const auto op = j.at("op").get<std::string>();
    guint32 timestamp = j.value("time", 0);
    std::string token = j.value("token", "");
    if (op == "part-placed") {
        UUID part = j.at("part").get<std::string>();
        part_browser_window->placed_part(part);
    }
    else if (op == "show-browser") {
        part_browser_window->set_entity({}, "");
        activate_window(part_browser_window, timestamp, token);
        part_browser_window->focus_search();
    }
    else if (op == "show-browser-assign") {
        const UUID entity_uuid = j.at("entity").get<std::string>();
        part_browser_window->set_entity(entity_uuid, j.at("hint").get<std::string>());
        activate_window(part_browser_window, timestamp, token);
        part_browser_window->focus_search();
    }
    else if (op == "schematic-select") {
        if (auto proc = find_board_process()) {
            auto pid = proc->proc->get_pid();
            json tx;
            tx["op"] = "highlight";
            tx["objects"] = j.at("selection");
            app.send_json(pid, tx);
        }
        const json &o = j.at("selection");
        bool can_assign = false;
        for (auto it = o.cbegin(); it != o.cend(); ++it) {
            auto type = static_cast<ObjectType>(it.value().at("type").get<int>());
            if (type == ObjectType::COMPONENT)
                can_assign = true;
        }
        part_browser_window->set_can_assign(can_assign);
    }
    else if (op == "board-select") {
        if (auto proc = find_top_schematic_process()) {
            auto pid = proc->proc->get_pid();
            json tx;
            tx["op"] = "highlight";
            tx["objects"] = j.at("selection");
            app.send_json(pid, tx);
        }
    }
    else if (op == "to-board") {
        if (auto proc = find_board_process()) {
            auto pid = proc->proc->get_pid();
            json tx;
            tx["op"] = "place";
            tx["time"] = timestamp;
            tx["token"] = token;
            tx["components"] = j.at("selection");
            app.send_json(pid, tx);
        }
    }
    else if (op == "show-in-browser") {
        part_browser_window->go_to_part(UUID(j.at("part").get<std::string>()));
        activate_window(part_browser_window, timestamp, token);
    }
    else if (op == "has-board") {
        return find_board_process() != nullptr;
    }
    else if (op == "has-schematic") {
        return find_top_schematic_process() != nullptr;
    }
    else if (op == "get-board-pid") {
        if (auto proc = find_board_process())
            return proc->proc->get_pid();
        else
            return -1;
    }
    else if (op == "get-schematic-pid") {
        if (auto proc = find_top_schematic_process())
            return proc->proc->get_pid();
        else
            return -1;
    }
    else if (op == "reload-netlist") {
        if (auto proc = find_board_process()) {
            auto pid = proc->proc->get_pid();
            json tx;
            tx["op"] = "reload-netlist";
            tx["time"] = timestamp;
            tx["token"] = token;
            app.send_json(pid, tx);
        }
    }
    else if (op == "present-board") {
        if (auto proc = find_board_process()) {
            auto pid = proc->proc->get_pid();
            json tx;
            tx["op"] = "present";
            tx["time"] = timestamp;
            tx["token"] = token;
            app.send_json(pid, tx);
        }
        else {
            view_project.open_board();
        }
    }
    else if (op == "present-schematic") {
        if (auto proc = find_top_schematic_process()) {
            auto pid = proc->proc->get_pid();
            json tx;
            tx["op"] = "present";
            tx["time"] = timestamp;
            tx["token"] = token;
            app.send_json(pid, tx);
        }
        else {
            view_project.open_top_schematic();
        }
    }
    else if (op == "needs-save") {
        const auto pid = j.at("pid").get<int>();
        const auto needs_save = j.at("needs_save").get<bool>();
        const auto filename = j.at("filename").get<std::string>();
        std::cout << "needs save " << pid << " " << needs_save << std::endl;
        if (!needs_save) {
            for (auto &it : processes) {
                if (it.second.proc && it.second.proc->get_pid() == pid) {
                    it.second.set_filename(filename);
                }
            }
        }
        pids_need_save[pid] = needs_save;
        if (project && !needs_save)
            view_project.update_meta();
        if (!needs_save) {
            s_signal_process_saved.emit(filename);
            if (auto proc = find_top_schematic_process()) {
                if (proc->proc->get_pid() == pid) { // schematic got saved
                    if (auto proc_board = find_board_process()) {
                        auto board_pid = proc_board->proc->get_pid();
                        json tx;
                        tx["op"] = "reload-netlist-hint";
                        app.send_json(board_pid, tx);
                    }
                }
            }
        }
    }
    else if (op == "ready") {
        const auto pid = j.at("pid").get<int>();
        for (auto &it : processes) {
            if (it.second.proc && it.second.proc->get_pid() == pid) {
                it.second.signal_ready().emit();
                break;
            }
        }
    }
    else if (op == "edit") {
        auto type = object_type_lut.lookup(j.at("type"));
        UUID uu(j.at("uuid").get<std::string>());
        try {
            UUID this_pool_uuid = pool->get_pool_info().uuid;
            UUID item_pool_uuid;
            auto path = pool->get_filename(type, uu, &item_pool_uuid);
            PoolProjectManagerProcess::Type ptype;
            switch (type) {
            case ObjectType::PADSTACK:
                ptype = PoolProjectManagerProcess::Type::IMP_PADSTACK;
                break;
            case ObjectType::UNIT:
                ptype = PoolProjectManagerProcess::Type::UNIT;
                break;
            default:
                return nullptr;
            }
            const bool spawn_read_only =
                    (item_pool_uuid != Pool::tmp_pool_uuid) && this_pool_uuid && (item_pool_uuid != this_pool_uuid);
            spawn(ptype, {path}, spawn_read_only ? SpawnFlags::READ_ONLY : SpawnFlags::NONE);
        }
        catch (const std::exception &e) {
            Gtk::MessageDialog md(*this, "Can't open editor", false /* use_markup */, Gtk::MESSAGE_ERROR,
                                  Gtk::BUTTONS_OK);
            md.set_secondary_text(e.what());
            md.run();
        }
    }
    else if (op == "preferences") {
        auto win = app.show_preferences_window(timestamp, token);
        if (j.count("page"))
            win->show_page(j.at("page").get<std::string>());
    }
    else if (op == "show-in-pool-mgr") {
        UUID uu = j.at("uuid").get<std::string>();
        UUID pool_uu = j.at("pool_uuid").get<std::string>();
        ObjectType type = object_type_lut.lookup(j.at("type"));
        std::string pool_path;
        if (pool_uu == PoolInfo::project_pool_uuid && project) {
            pool_path = project->pool_directory;
        }
        else if (auto pool2 = PoolManager::get().get_by_uuid(pool_uu)) {
            pool_path = pool2->base_path;
        }
        if (pool_path.size()) {
            auto &win = app.open_pool_or_project(Glib::build_filename(pool_path, "pool.json"), timestamp, token);
            win.pool_notebook_go_to(type, uu);
        }
    }
    else if (op == "backannotate") {
        if (auto proc = find_top_schematic_process()) {
            auto pid = proc->proc->get_pid();
            json tx;
            tx["op"] = "backannotate";
            tx["connections"] = j.at("connections");
            tx["time"] = timestamp;
            tx["token"] = token;
            app.send_json(pid, tx);
        }
    }
    else if (op == "symbol-select") {
        UUID unit_uuid = j.at("unit").get<std::string>();
        for (auto &it_proc : processes) {
            if (it_proc.second.win && it_proc.second.win->get_object_type() == ObjectType::UNIT
                && it_proc.second.win->get_uuid() == unit_uuid) {
                ItemSet sel;
                for (const auto &pin : j.at("pins")) {
                    sel.emplace(ObjectType::SYMBOL_PIN, pin.get<std::string>());
                }
                it_proc.second.win->select(sel);
                break;
            }
        }
    }
    else if (op == "focus") {
        activate_window(this, timestamp, token);
    }
    else if (op == "open-project") {
        const auto &filename = j.at("filename").get<std::string>();
        app.open_pool_or_project(filename, timestamp, token);
    }
    else if (op == "update-pool") {
        const auto filenames = j.at("filenames").get<std::vector<std::string>>();
        app.signal_pool_items_edited().emit(filenames);
    }
    return nullptr;
}


void PoolProjectManagerAppWindow::set_pool_updating(bool v, bool success)
{
    button_update->set_sensitive(!v);
    pool_update_status_close_button->set_visible(!success);
    if (success) {
        if (v) { // show immediately
            pool_update_conn = Glib::signal_timeout().connect(
                    [this] {
                        pool_update_status_rev->set_reveal_child(true);
                        return false;
                    },
                    500);
        }
        else {
            pool_update_conn.disconnect();
            Glib::signal_timeout().connect(
                    [this] {
                        pool_update_status_rev->set_reveal_child(false);
                        return false;
                    },
                    500);
        }
    }
    if (!get_realized())
        return;
    auto win = get_window();
    if (v)
        win->set_cursor(Gdk::Cursor::create(win->get_display(), "progress"));
    else
        win->set_cursor();
}

void PoolProjectManagerAppWindow::set_pool_update_status_text(const std::string &txt)
{
    pool_update_status_label->set_markup("<b>Updating pool:</b> " + txt);
}

void PoolProjectManagerAppWindow::set_pool_update_progress(float progress)
{
    if (progress < 0) {
        pool_update_progress->pulse();
    }
    else {
        pool_update_progress->set_fraction(progress);
    }
}

PoolProjectManagerAppWindow::~PoolProjectManagerAppWindow()
{
    if (output_window)
        delete output_window;
    cleanup();
}

void PoolProjectManagerAppWindow::cleanup()
{
    if (part_browser_window) {
        delete part_browser_window;
        part_browser_window = nullptr;
    }
}

void PoolProjectManagerAppWindow::handle_recent()
{
    // auto file = Gio::File::create_for_uri(recent_chooser->get_current_uri());
    // open_file_view(file);
}

void PoolProjectManagerAppWindow::handle_cancel()
{
    set_view_mode(ViewMode::OPEN);
}

void PoolProjectManagerAppWindow::save_project()
{
    if (project_read_only)
        return;
    set_version_info("");
    save_json_to_file(project_filename, project->serialize());
    project_needs_save = false;
}

void PoolProjectManagerAppWindow::handle_save()
{
    if (project) {
        save_project();
        app.send_json(0, {{"op", "save"}});
    }
}

void PoolProjectManagerAppWindow::handle_open()
{
    GtkFileChooserNative *native = gtk_file_chooser_native_new("Open Pool or Project", GTK_WINDOW(gobj()),
                                                               GTK_FILE_CHOOSER_ACTION_OPEN, "_Open", "_Cancel");
    auto chooser = Glib::wrap(GTK_FILE_CHOOSER(native));
    auto filter = Gtk::FileFilter::create();
    filter->set_name("Horizon pools & projects");
    filter->add_pattern("pool.json");
    filter->add_pattern("*.hprj");
    chooser->add_filter(filter);

    if (gtk_native_dialog_run(GTK_NATIVE_DIALOG(native)) == GTK_RESPONSE_ACCEPT) {
        auto file = chooser->get_file();
        open_file_view(file);
    }
}

void PoolProjectManagerAppWindow::handle_close()
{
    close_pool_or_project();
}

void PoolProjectManagerAppWindow::handle_update()
{
    pool_update();
}

void PoolProjectManagerAppWindow::handle_new_project()
{
    if (!check_pools())
        return;
    view_create_project.clear();
    view_create_project.focus();
    set_view_mode(ViewMode::CREATE_PROJECT);
}

void PoolProjectManagerAppWindow::handle_new_pool()
{
    set_view_mode(ViewMode::CREATE_POOL);
}

void PoolProjectManagerAppWindow::handle_create()
{
    if (view_mode == ViewMode::CREATE_PROJECT) {
        const auto r = view_create_project.create();
        if (r) {
            view_create_project.clear();
            open_file_view(Gio::File::create_for_path(*r));
        }
    }
    else if (view_mode == ViewMode::CREATE_POOL) {
        const auto r = view_create_pool.create();
        if (r) {
            view_create_pool.clear();
            PoolManager::get().add_pool(Glib::path_get_dirname(*r));
            open_file_view(Gio::File::create_for_path(*r));
        }
    }
}

std::optional<std::string> PoolProjectManagerAppWindow::peek_recent_name(const std::string &path)
{
    try {
        const auto ppath = fs::u8path(path);
        auto mtime = fs::last_write_time(ppath);
        if (Glib::path_get_basename(path) != "pool.json") {
            for (const auto &p : fs::directory_iterator(ppath.parent_path())) {
                if (fs::is_regular_file(p) && endswith(p.path().u8string(), ".json"))
                    mtime = std::max(mtime, fs::last_write_time(p));
            }
        }
        const auto mtime_i = mtime.time_since_epoch().count();
        if (app.user_config.recent_items_title_cache.count(path)) {
            auto &it = app.user_config.recent_items_title_cache.at(path);
            if (mtime_i <= it.mtime) // cache is okay
                return it.title;
        }
        {
            const auto name = peek_recent_name_from_path(path);
            if (name) {
                auto &it = app.user_config.recent_items_title_cache[path];
                it.title = name.value();
                it.mtime = mtime_i;
                return it.title;
            }
            return {};
        }
    }
    catch (...) {
        return {};
    }
}

std::optional<std::string> PoolProjectManagerAppWindow::peek_recent_name_from_path(const std::string &path)
{
    try {
        if (Glib::path_get_basename(path) == "pool.json") {
            PoolInfo pool_info(Glib::path_get_dirname(path));
            if (pool_info.is_project_pool())
                return {};
            else
                return pool_info.name;
        }
        else {
            auto prj = Project::new_from_file(path);
            return prj.peek_title();
        }
    }
    catch (...) {
        return {};
    }
}

static std::vector<std::pair<std::string, Glib::DateTime>>
recent_sort(const std::map<std::string, Glib::DateTime> &recent_items)
{
    std::vector<std::pair<std::string, Glib::DateTime>> recent_items_sorted;

    recent_items_sorted.reserve(recent_items.size());
    for (const auto &it : recent_items) {
        recent_items_sorted.emplace_back(it.first, it.second);
    }
    std::sort(recent_items_sorted.begin(), recent_items_sorted.end(),
              [](const auto &a, const auto &b) { return a.second.to_unix() > b.second.to_unix(); });
    return recent_items_sorted;
}

void PoolProjectManagerAppWindow::update_recent_items()
{
    {
        for (auto &lb : recent_listboxes) {
            auto chs = lb->get_children();
            for (auto it : chs) {
                delete it;
            }
        }
    }
    std::vector<std::pair<std::string, Glib::DateTime>> recent_items_sorted = recent_sort(app.user_config.recent_items);
    for (const auto &it : recent_items_sorted) {
        const std::string &path = it.first;
        if (const auto name = peek_recent_name(path)) {
            auto box = Gtk::manage(new RecentItemBox(*name, it.first, it.second));
            if (endswith(path, "pool.json"))
                recent_pools_listbox->append(*box);
            else
                recent_projects_listbox->append(*box);
            box->show();
            box->signal_remove().connect([this, box] {
                app.user_config.recent_items.erase(box->path);
                Glib::signal_idle().connect_once([this] { update_recent_items(); });
            });
        }
    }
    update_recent_placeholder(*recent_pools_placeholder_stack, *recent_pools_listbox);
    update_recent_placeholder(*recent_projects_placeholder_stack, *recent_projects_listbox);
    update_recent_searches();
}

void PoolProjectManagerAppWindow::prepare_close()
{
    if (pool_notebook)
        pool_notebook->prepare_close();
}

bool PoolProjectManagerAppWindow::close_pool_or_project()
{
    std::vector<PoolProjectManagerApplication::CloseOrHomeWindow> windows;
    windows.push_back({*this, false});
    if (project) {
        auto pool_path = Glib::build_filename(project->pool_directory, "pool.json");
        for (auto ws : app.get_windows()) {
            auto wi = dynamic_cast<PoolProjectManagerAppWindow *>(ws);
            if (wi && wi->get_filename() == pool_path) {
                windows.push_back({*wi, true});
                break;
            }
        }
    }
    return app.close_windows(windows);
}


void PoolProjectManagerAppWindow::wait_for_all_processes()
{
    if (!processes.size())
        return;
    ProcWaitDialog dia(this);
    while (dia.run() != 1) {
    }
}

bool PoolProjectManagerAppWindow::really_close_pool_or_project()
{
    if (pool_notebook) {
        if (processes.size() || pool_notebook->get_close_prohibited()) {
            Gtk::MessageDialog md(*this, "Can't close right now", false /* use_markup */, Gtk::MESSAGE_ERROR,
                                  Gtk::BUTTONS_OK);
            md.set_secondary_text("Close all running editors first");
            md.run();
            return false;
        }
        if (pool_notebook)
            pool_notebook->prepare_close();
        app.add_recent_item(Glib::build_filename(pool->get_base_path(), "pool.json"));
        delete pool_notebook;
        info_bar_hide(info_bar_pool_not_added);
        pool_notebook = nullptr;
        set_view_mode(ViewMode::OPEN);
    }
    if (project) {
        if (processes.size()) {
            Gtk::MessageDialog md(*this, "Can't close right now", false /* use_markup */, Gtk::MESSAGE_ERROR,
                                  Gtk::BUTTONS_OK);
            md.set_secondary_text("Close all running editors first");
            md.run();
            return false;
        }
        app.add_recent_item(project_filename);
        info_bar_hide(info_bar_gitignore);
        project.reset();
        cleanup();
        set_view_mode(ViewMode::OPEN);
    }
    else {
        set_view_mode(ViewMode::OPEN);
    }
    output_window->clear_all();
    if (check_mtimes_thread.joinable())
        check_mtimes_thread.join();
    return true;
}

void PoolProjectManagerAppWindow::set_view_mode(ViewMode mode)
{
    button_open->hide();
    button_close->hide();
    button_update->hide();
    button_cancel->hide();
    button_save->hide();
    button_create->hide();
    button_new->hide();
    header->set_subtitle("");
    header->set_show_close_button(true);
    hamburger_menu_button->hide();
    view_mode = mode;

    switch (mode) {
    case ViewMode::OPEN:
        stack->set_visible_child("open");
        button_open->show();
        button_new->show();
        hamburger_menu_button->show();
        header->set_title("Horizon EDA");
        update_recent_items();
        set_version_info("");
        info_bar_hide(info_bar_installation_uuid_mismatch);
        clear_recent_searches();
        break;

    case ViewMode::POOL:
        stack->set_visible_child("pool");
        button_close->show();
        button_update->show();
        hamburger_menu_button->show();
        header->set_title("Pool manager");
        if (!app.user_config.pool_doc_info_bar_dismissed)
            info_bar_show(info_bar_pool_doc);
        break;

    case ViewMode::PROJECT:
        stack->set_visible_child("project");
        button_close->show();
        button_save->show();
        hamburger_menu_button->show();
        header->set_title("Project manager");
        break;

    case ViewMode::CREATE_PROJECT:
        stack->set_visible_child("create_project");
        header->set_show_close_button(false);
        button_cancel->show();
        button_create->show();
        view_create_project.populate_pool_combo();
        view_create_project.update();
        header->set_title("Horizon EDA");
        break;

    case ViewMode::CREATE_POOL:
        stack->set_visible_child("create_pool");
        header->set_show_close_button(false);
        button_cancel->show();
        button_create->show();
        header->set_title("Horizon EDA");
        view_create_pool.update();
        break;
    }
}

PoolProjectManagerAppWindow::ViewMode PoolProjectManagerAppWindow::get_view_mode() const
{
    return view_mode;
}

UUID PoolProjectManagerAppWindow::get_pool_uuid() const
{
    if (pool_notebook)
        return pool_notebook->get_pool_uuid();
    else
        return UUID();
}

void PoolProjectManagerAppWindow::pool_notebook_go_to(ObjectType type, const UUID &uu)
{
    if (pool_notebook)
        pool_notebook->go_to(type, uu);
}

void PoolProjectManagerAppWindow::pool_notebook_show_settings_tab()
{
    if (pool_notebook)
        pool_notebook->show_settings_tab();
}

PoolProjectManagerAppWindow *PoolProjectManagerAppWindow::create(PoolProjectManagerApplication &app)
{
    // Load the Builder file and instantiate its widgets.
    std::vector<Glib::ustring> widgets = {"app_window",        "sg_dest",         "sg_repo",           "menu1",
                                          "sg_base_path",      "sg_project_name", "sg_project_title",  "sg_pool_name",
                                          "sg_pool_base_path", "sg_prj_pool",     "sg_prj_open_change"};
    auto refBuilder = Gtk::Builder::create_from_resource("/org/horizon-eda/horizon/pool-prj-mgr/window.ui", widgets);

    PoolProjectManagerAppWindow *window = nullptr;
    refBuilder->get_widget_derived("app_window", window, app);

    if (!window)
        throw std::runtime_error("No \"app_window\" object in window.ui");
    return window;
}

bool PoolProjectManagerAppWindow::check_schema_update(const std::string &base_path)
{
    const auto r = pool_check_schema_update(base_path, *this);
    if (r == CheckSchemaUpdateResult::INSTALLATION_UUID_MISMATCH)
        info_bar_show(info_bar_installation_uuid_mismatch);

    if (r != CheckSchemaUpdateResult::NO_UPDATE) {
        app.signal_pool_updated().emit(base_path);
        return true;
    }
    return false;
}


static bool check_pool_update_inc(const std::string &base_path)
{
    Pool my_pool(base_path);
    for (auto &[bp, uu] : my_pool.get_actually_included_pools(false)) {
        PoolInfo pool_info(bp);
        if (pool_info.is_usable()) {
            {
                SQLite::Query q(my_pool.db, "ATTACH ? as inc");
                q.bind(1, (fs::u8path(bp) / "pool.db").u8string());
                q.step();
            }
            {
                SQLite::Query q(my_pool.db,
                                "SELECT a.time > b.time FROM inc.last_updated AS a LEFT JOIN last_updated AS b");
                if (q.step()) {
                    if (q.get<int>(0)) {
                        Logger::log_info("pool " + base_path + " got updated", Logger::Domain::POOL_UPDATE,
                                         "beacause included pool " + bp + " has been updated");
                        return true;
                    }
                }
            }

            my_pool.db.execute("DETACH inc");
        }
    }
    return false;
}


bool PoolProjectManagerAppWindow::migrate_project(const std::string &path)
{
    auto prj = Project::new_from_file(path);
    if (prj.version.get_file() < 1) {
        Gtk::MessageDialog md(*this, "Migrate project?", false /* use_markup */, Gtk::MESSAGE_QUESTION,
                              Gtk::BUTTONS_NONE);
        md.set_secondary_text(
                "This project has been created with an older version of Horizon EDA and needs to be migrated to be "
                "compatible with this version.\n"
                "You'll still be able to open this project with older versions of Horizon EDA.");
        md.add_button("Don't open", Gtk::RESPONSE_CANCEL);
        md.add_button("Migrate", Gtk::RESPONSE_OK);
        if (md.run() == Gtk::RESPONSE_OK) {
            const json j_prj = load_json_from_file(path);
            const UUID pool_uuid = j_prj.at("pool_uuid").get<std::string>();
            const auto cache_directory =
                    fs::u8path(path).parent_path() / fs::u8path(j_prj.at("pool_cache_directory").get<std::string>());

            PoolInfo info;
            info.uuid = PoolInfo::project_pool_uuid;
            info.name = "Project pool";
            info.base_path = prj.pool_directory;
            info.pools_included = {pool_uuid};
            ProjectPool::create_directories(info.base_path);
            info.save();
            save_json_to_file(path, prj.serialize());
            prj.create_blocks();

            // copy pool items
            for (const auto &file : fs::directory_iterator(cache_directory)) {
                const auto filename = file.path().u8string();
                if (file.is_regular_file() && endswith(filename, ".json")) {
                    const auto j = load_json_from_file(filename);
                    const auto type = object_type_lut.lookup(j.at("type").get<std::string>());
                    const UUID uu = j.at("uuid").get<std::string>();
                    if (type != ObjectType::PACKAGE) {
                        const auto dest = fs::u8path(info.base_path) / fs::u8path(Pool::type_names.at(type)) / "cache"
                                          / ((std::string)uu + ".json");
                        fs::copy(file.path(), dest);
                    }
                    else {
                        const auto dest_dir = fs::u8path(info.base_path) / fs::u8path(Pool::type_names.at(type))
                                              / "cache" / ((std::string)uu);
                        fs::create_directories(dest_dir);
                        const auto dest = dest_dir / "package.json";
                        json j_pkg = j;
                        ProjectPool::patch_package(j_pkg, pool_uuid);
                        save_json_to_file((dest_dir / "package.json").u8string(), j_pkg);
                    }
                }
            }

            // copy 3D models
            {
                const auto models_dir = cache_directory / "3d_models";
                if (fs::is_directory(models_dir)) {
                    for (auto &it : fs::directory_iterator(models_dir)) {
                        const auto pool_dir_name = it.path().filename().string();
                        if (pool_dir_name.find("pool_") == 0 && pool_dir_name.size() == 41) {
                            const UUID model_pool_uuid(pool_dir_name.substr(5));
                            fs::copy(it,
                                     fs::u8path(prj.pool_directory) / "3d_models" / "cache"
                                             / (std::string)model_pool_uuid,
                                     fs::copy_options::recursive);
                        }
                    }
                }
            }

            return true;
        }
        return false;
    }
    return true;
}

#if GTK_CHECK_VERSION(3, 24, 0)
gboolean PoolProjectManagerAppWindow::part_browser_key_pressed(GtkEventControllerKey *controller, guint keyval,
                                                               guint keycode, GdkModifierType state, gpointer user_data)
{
    if (keyval != GDK_KEY_Escape)
        return FALSE;

    auto self = reinterpret_cast<PoolProjectManagerAppWindow *>(user_data);
    if (auto proc = self->find_top_schematic_process()) {
        auto pid = proc->proc->get_pid();
        json tx;
        tx["op"] = "present";
        tx["time"] = gtk_get_current_event_time();
        tx["token"] = get_activation_token(self->part_browser_window);
        self->app.send_json(pid, tx);
    }

    return TRUE;
}
#endif

void PoolProjectManagerAppWindow::check_mtimes(const std::string &path)
{
    try {
        Pool my_pool(path);
        int64_t last_update_time = 0;
        {
            SQLite::Query q(my_pool.db, "SELECT time FROM last_updated");
            if (q.step())
                last_update_time = q.get<sqlite3_int64>(0);
            else
                return;
        }

        for (auto &[bp, uu] : my_pool.get_actually_included_pools(true)) {
            PoolInfo pool_info(bp);
            for (const auto &[type, name] : IPool::type_names) {
                const auto p = fs::u8path(bp) / name;
                if (!fs::is_directory(p))
                    continue;
                for (const auto &ent : fs::recursive_directory_iterator(p)) {
                    if (!ent.is_regular_file())
                        continue;
                    if (endswith(ent.path().filename().u8string(), ".json")) {
                        const auto mtime = ent.last_write_time().time_since_epoch().count();
                        if (mtime > last_update_time) {
                            Logger::log_info("pool " + path + " got updated", Logger::Domain::POOL_UPDATE,
                                             "beacause " + ent.path().u8string() + " has been modified");
                            check_mtimes_dispatcher.emit();
                            return;
                        }
                    }
                }
            }
        }
    }
    catch (const std::exception &e) {
        Logger::log_critical("exception thrown in check_mtimes", Logger::Domain::POOL_UPDATE, e.what());
    }
    catch (...) {
        Logger::log_critical("unknown exception thrown in check_mtimes", Logger::Domain::POOL_UPDATE);
    }
}


void PoolProjectManagerAppWindow::check_pool_update(const std::string &base_path)
{
    bool updated = false;
    if (check_schema_update(base_path)) {
        // it's okay, pool got updated
        updated = true;
    }

    { // check that all included pools are usable
        Pool my_pool(base_path);
        for (auto &[bp, uu] : my_pool.get_actually_included_pools(false)) {
            PoolInfo pool_info(bp);
            if (!pool_info.is_usable()) {
                ForcedPoolUpdateDialog dia(bp, *this);
                while (dia.run() != 1) {
                }
            }
        }
    }

    if (check_pool_update_inc(base_path)) {
        pool_update();
        updated = true;
    }

    if (!updated) {
        check_mtimes_thread = std::thread(&PoolProjectManagerAppWindow::check_mtimes, this, base_path);
    }
}

void PoolProjectManagerAppWindow::open_file_view(const Glib::RefPtr<Gio::File> &file)
{
    auto path = file->get_path();
    app.add_recent_item(path);

    if (app.present_existing_window(path))
        return;

    pool_update_n_files_last = 0;

    auto basename = file->get_basename();
    if (basename == "pool.json") {
        try {
            if (!file->query_exists()) {
                throw std::runtime_error("pool.json not found");
            }
            auto pool_base_path = file->get_parent()->get_path();
            check_pool_update(pool_base_path);
            set_view_mode(ViewMode::POOL);
            pool_notebook = new PoolNotebook(pool_base_path, *this);
            if (!PoolManager::get().get_pools().count(pool_base_path) && !pool->get_pool_info().is_project_pool()) {
                info_bar_show(info_bar_pool_not_added);
            }
            pool_box->pack_start(*pool_notebook, true, true, 0);
            pool_notebook->show();
            header->set_subtitle(pool_base_path);

            {
                auto &settings_box = pool_notebook->get_pool_settings_box();
                set_version_info(settings_box.get_version_message());
                settings_box.signal_changed().connect(
                        [this, &settings_box] { set_version_info(settings_box.get_version_message()); });
            }

            if (pool->get_pool_info().is_project_pool()) {
                button_close->hide();
                auto project_path = Glib::path_get_dirname(pool_base_path);
                for (auto win : app.get_windows()) {
                    if (auto w = dynamic_cast<PoolProjectManagerAppWindow *>(win)) {
                        if (w->get_view_mode() == PoolProjectManagerAppWindow::ViewMode::PROJECT) {
                            const auto win_path = Glib::path_get_dirname(w->get_filename());
                            if (win_path == project_path) {
                                set_title(w->get_project_title() + " – Pool");
                                break;
                            }
                        }
                    }
                }
            }
        }
        catch (const std::exception &e) {
            Gtk::MessageDialog md(*this, "Error opening pool", false /* use_markup */, Gtk::MESSAGE_ERROR,
                                  Gtk::BUTTONS_OK);
            md.set_secondary_text(e.what());
            md.run();
            close_pool_or_project();
        }
    }
    else {
        if (!migrate_project(path)) {
            return;
        }
        project = std::make_unique<Project>(Project::new_from_file(path));
        project_filename = path;
        project_needs_save = false;

        check_pool_update(project->pool_directory);

        {
            project_read_only = false;
            const auto &version = project->version;
            const auto version_msg = version.get_message(ObjectType::PROJECT);
            if (version_msg.size()) {
                set_version_info(
                        version_msg
                        + " This only applies to the project file. Board and Schematic might have different versions.");
            }
            else {
                set_version_info("");
            }

            if (version.get_app() < version.get_file()) {
                project_read_only = true;
            }
            if (version.get_file() < 2) {
                // need to create blocks.json
                project->create_blocks();
                project_needs_save = true;
            }
        }

        if (Project::gitignore_needs_fixing(get_gitignore_filename())) {
            info_bar_show(info_bar_gitignore);
        }

        view_project.label_project_directory->set_text(Glib::path_get_dirname(project_filename));

        view_project.reset_pool_cache_status();

        part_browser_window = PartBrowserWindow::create(this, project->pool_directory, app.user_config.part_favorites);
        part_browser_window->signal_place_part().connect(
                sigc::mem_fun(*this, &PoolProjectManagerAppWindow::handle_place_part));
        part_browser_window->signal_assign_part().connect(
                sigc::mem_fun(*this, &PoolProjectManagerAppWindow::handle_assign_part));

#if GTK_CHECK_VERSION(3, 24, 0)
        {
            auto ctrl = gtk_event_controller_key_new(GTK_WIDGET(part_browser_window->gobj()));
            gtk_event_controller_set_propagation_phase(ctrl, GTK_PHASE_CAPTURE);
            g_signal_connect(ctrl, "key-pressed", G_CALLBACK(part_browser_key_pressed), this);
        }
#endif

        set_view_mode(ViewMode::PROJECT);

        view_project.update_meta();
        view_project.update_pools_label();
    }
}

const std::string &PoolProjectManagerAppWindow::get_project_title() const
{
    if (!project)
        throw std::runtime_error("not a project");
    return project_title;
}

void PoolProjectManagerAppWindow::open_pool(const std::string &pool_json)
{
    open_file_view(Gio::File::create_for_path(pool_json));
}

void PoolProjectManagerAppWindow::handle_place_part(const UUID &uu)
{
    if (auto proc = find_top_schematic_process()) {
        auto pid = proc->proc->get_pid();
        allow_set_foreground_window(pid);
        auto token = get_activation_token(part_browser_window);
        app.send_json(pid, {{"op", "place-part"},
                            {"part", (std::string)uu},
                            {"time", gtk_get_current_event_time()},
                            {"token", token}});
    }
}

void PoolProjectManagerAppWindow::handle_assign_part(const UUID &uu)
{
    if (auto proc = find_top_schematic_process()) {
        auto pid = proc->proc->get_pid();
        allow_set_foreground_window(pid);
        auto token = get_activation_token(part_browser_window);
        app.send_json(pid, {{"op", "assign-part"},
                            {"part", (std::string)uu},
                            {"time", gtk_get_current_event_time()},
                            {"token", token}});
    }
}

bool PoolProjectManagerAppWindow::on_delete_event(GdkEventAny *ev)
{
    // returning false will destroy the window
    std::cout << "close" << std::endl;
    return !close_pool_or_project();
}

std::string PoolProjectManagerAppWindow::get_pool_base_path() const
{
    if (pool)
        return pool->get_base_path();
    else if (project)
        return project->pool_directory;
    else
        throw std::runtime_error("can't locate pool");
}

PoolProjectManagerAppWindow::SpawnResult PoolProjectManagerAppWindow::spawn(PoolProjectManagerProcess::Type type,
                                                                            const std::vector<std::string> &args,
                                                                            SpawnFlags flags)
{
    const auto pool_base_path = get_pool_base_path();

    if (find_process(args.at(0)) == nullptr || args.at(0).size() == 0) { // need to launch imp

        std::vector<std::string> env = {"HORIZON_POOL=" + pool_base_path,
                                        "HORIZON_EP_BROADCAST=" + app.get_ep_broadcast(),
                                        "HORIZON_EP_MGR=" + sock_mgr_ep, "HORIZON_MGR_PID=" + std::to_string(getpid()),
                                        "HORIZON_IPC_COOKIE=" + (std::string)app.ipc_cookie};
        std::string filename = args.at(0);
        if (filename.size()) {
            if (!Glib::file_test(filename, Glib::FILE_TEST_IS_REGULAR)) {
                auto top = dynamic_cast<Gtk::Window *>(get_ancestor(GTK_TYPE_WINDOW));
                Gtk::MessageDialog md(*top, "File not found", false /* use_markup */, Gtk::MESSAGE_ERROR,
                                      Gtk::BUTTONS_OK);
                md.set_secondary_text("Try updating the pool");
                md.run();
                return {nullptr, false};
            }
        }

        if (!check_autosave(type, args))
            return {nullptr, false};

        auto uu = UUID::random();
        const bool read_only = static_cast<int>(flags) & static_cast<int>(SpawnFlags::READ_ONLY);
        const bool is_temp = static_cast<int>(flags) & static_cast<int>(SpawnFlags::TEMP);
        auto &proc =
                processes
                        .emplace(std::piecewise_construct, std::forward_as_tuple(uu),
                                 std::forward_as_tuple(uu, type, args, env, pool, pool_parametric, read_only, is_temp))
                        .first->second;
        if (proc.win && pool_notebook) {
            proc.win->signal_goto().connect(sigc::mem_fun(pool_notebook, &PoolNotebook::go_to));
            proc.win->signal_open_item().connect(sigc::mem_fun(pool_notebook, &PoolNotebook::edit_item));
        }
        proc.signal_exited().connect([uu, this](int status, bool need_update) {
            auto real_filename = processes.at(uu).get_filename();
            processes.erase(uu);
            s_signal_process_exited.emit(real_filename, status, need_update);
            if (status != 0) {
                info_bar_label->set_text("Editor for '" + real_filename + "' exited with status "
                                         + std::to_string(status));
                show_output_button->set_visible(app.get_preferences().capture_output);
                info_bar_show(info_bar);
            }
        });

        if (proc.proc)
            info_bar_hide(info_bar);

        proc.signal_output().connect([uu, this](std::string line, bool err) {
            trim(line);
            if (line.size()) {
                auto real_filename = processes.at(uu).get_filename();
                output_window->handle_output("[" + real_filename + "] " + line, err);
            }
        });
        if (proc.win)
            proc.win->signal_saved().connect([this](auto fn) { s_signal_process_saved.emit(fn); });

        return {&proc, true};
    }
    else { // present imp
        auto proc = find_process(args.at(0));
        if (proc->proc) {
            auto pid = proc->proc->get_pid();
            allow_set_foreground_window(pid);
            auto token = get_activation_token(this);
            app.send_json(pid, {{"op", "present"}, {"time", gtk_get_current_event_time()}, {"token", token}});
        }
        else {
            proc->win->present();
        }
        return {proc, false};
    }
}

bool PoolProjectManagerAppWindow::check_autosave(PoolProjectManagerProcess::Type type,
                                                 const std::vector<std::string> &filenames)
{
    if (filenames.size() == 0)
        return true;

    std::vector<std::string> my_filenames = filenames;
    my_filenames.resize(1);
    if (type == PoolProjectManagerProcess::Type::IMP_SCHEMATIC) {
        // is blocks file, get filename
        const auto filenames_from_blocks = BlocksBase::peek_filenames(my_filenames.front());
        my_filenames.insert(my_filenames.end(), filenames_from_blocks.begin(), filenames_from_blocks.end());
    }

    bool have_autosave = std::all_of(my_filenames.begin(), my_filenames.end(), [](auto &x) {
        return Glib::file_test(x + ".autosave", Glib::FILE_TEST_IS_REGULAR);
    });
    if (!have_autosave)
        return true;

    AutosaveRecoveryDialog dia(this);
    if (dia.run() != GTK_RESPONSE_OK) {
        return false;
    }
    auto result = dia.get_result();

    switch (result) {
    case AutosaveRecoveryDialog::Result::KEEP:
        // nop
        break;
    case AutosaveRecoveryDialog::Result::DELETE:
        for (const auto &fn : my_filenames) {
            auto fn_autosave = fn + ".autosave";
            if (Glib::file_test(fn_autosave, Glib::FILE_TEST_IS_REGULAR)) {
                Gio::File::create_for_path(fn_autosave)->remove();
            }
        }
        break;

    case AutosaveRecoveryDialog::Result::USE:
        for (const auto &fn : my_filenames) {
            Gio::File::create_for_path(fn)->move(Gio::File::create_for_path(fn + ".bak"), Gio::FILE_COPY_OVERWRITE);
            auto fn_autosave = fn + ".autosave";
            if (Glib::file_test(fn_autosave, Glib::FILE_TEST_IS_REGULAR)) {
                Gio::File::create_for_path(fn_autosave)->move(Gio::File::create_for_path(fn), Gio::FILE_COPY_OVERWRITE);
            }
        }
        break;
    }

    return true;
}

std::string PoolProjectManagerAppWindow::get_proc_filename(const UUID &uu)
{
    if (uu == uuid_pool_manager || uu == uuid_project_manager)
        return get_filename();
    else
        return processes.at(uu).get_filename();
}

void PoolProjectManagerAppWindow::process_save(const UUID &uu)
{
    if (pool_notebook && uu == uuid_pool_manager) {
        pool_notebook->save();
        return;
    }
    if (project && uu == uuid_project_manager) {
        save_project();
        return;
    }
    auto &proc = processes.at(uu);
    if (proc.win) {
        proc.win->save();
    }
    else if (proc.proc) {
        auto pid = proc.proc->get_pid();
        app.send_json(pid, {{"op", "save"}});
    }
}

void PoolProjectManagerAppWindow::process_close(const UUID &uu)
{
    auto &proc = processes.at(uu);
    if (proc.win) {
        proc.win->force_close();
    }
    else if (proc.proc) {
        auto pid = proc.proc->get_pid();
        app.send_json(pid, {{"op", "close"}});
    }
}

std::string PoolProjectManagerAppWindow::get_filename() const
{
    if (pool_notebook)
        return Glib::build_filename(pool->get_base_path(), "pool.json");
    else if (project)
        return project_filename;
    else
        return "";
}

std::string PoolProjectManagerAppWindow::get_gitignore_filename() const
{
    return (fs::u8path(project_filename).parent_path() / ".gitignore").u8string();
}

PoolProjectManagerAppWindow::ClosePolicy PoolProjectManagerAppWindow::get_close_policy()
{
    PoolProjectManagerAppWindow::ClosePolicy r;
    if (pool_updating) {
        r.can_close = false;
        r.reason = "Pool is updating";
        return r;
    }
    if (pool_notebook) {
        auto close_prohibited = pool_notebook->get_close_prohibited();
        r.can_close = !close_prohibited;
        r.reason = "Pool is busy or dialog is open";
        if (pool_notebook->get_needs_save())
            r.procs_need_save.push_back(uuid_pool_manager);
    }
    if (project) {
        if (project_needs_save) {
            r.procs_need_save.push_back(uuid_project_manager);
        }
    }
    for (auto &proc : processes) {
        bool needs_save = true;
        if (proc.second.win) {
            needs_save = proc.second.win->get_needs_save();
        }
        else {
            auto pid = proc.second.proc->get_pid();
            if (pids_need_save.count(pid))
                needs_save = pids_need_save.at(pid);
            else
                needs_save = false;
        }
        if (needs_save) {
            r.procs_need_save.push_back(proc.first);
        }
    }
    return r;
}

std::map<UUID, PoolProjectManagerProcess *> PoolProjectManagerAppWindow::get_processes()
{
    std::map<UUID, PoolProjectManagerProcess *> r;
    for (auto &it : processes) {
        r.emplace(it.first, &it.second);
    }
    return r;
}

bool PoolProjectManagerAppWindow::cleanup_pool_cache(Gtk::Window *parent)
{
    if (project == nullptr)
        return false;

    auto pol = get_close_policy();
    if (pol.procs_need_save.size()) {
        Gtk::MessageDialog md(*parent, "Close or save all open files first", false, Gtk::MESSAGE_INFO, Gtk::BUTTONS_OK);
        md.run();
        return false;
    }
    ItemSet items_needed;

    ProjectPool project_pool(project->pool_directory, false);
    auto blocks = BlocksSchematic::new_from_file(project->blocks_filename, project_pool);
    auto flat_block = blocks.get_top_block_item().block.flatten();
    auto board = Board::new_from_file(project->board_filename, flat_block, project_pool);
    board.expand();
    for (auto &[uu, block] : blocks.blocks) {
        block.schematic.expand();
        {
            ItemSet items = block.block.get_pool_items_used();
            items_needed.insert(items.begin(), items.end());
        }
        {
            ItemSet items = block.schematic.get_pool_items_used();
            items_needed.insert(items.begin(), items.end());
        }
    }
    {
        auto items = board.get_pool_items_used();
        items_needed.insert(items.begin(), items.end());
    }

    std::set<std::string> models_needed;
    for (const auto &[uu, pkg] : board.packages) {
        auto model = pkg.package.get_model(pkg.model);
        if (model) {
            models_needed.emplace(model->filename);
        }
    }

    const auto model_cache_dir = fs::u8path("3d_models") / "cache";
    std::set<std::string> models_cached;
    for (auto &p : fs::recursive_directory_iterator(fs::u8path(project_pool.get_base_path()) / model_cache_dir)) {
        if (p.is_regular_file())
            models_cached.emplace(fs::relative(p, fs::u8path(project_pool.get_base_path())).u8string());
    }

    std::map<std::pair<ObjectType, UUID>, std::string> items_cached;
    auto status = PoolCacheStatus::from_project_pool(project_pool);
    for (const auto &item : status.items) {
        if (item.type != ObjectType::MODEL_3D) {
            items_cached.emplace(std::piecewise_construct, std::forward_as_tuple(item.type, item.uuid),
                                 std::forward_as_tuple(item.filename_cached));
        }
    }

    std::set<std::string> files_to_delete;
    for (const auto &it : items_cached) {
        if (items_needed.count(it.first) == 0) {
            files_to_delete.insert(it.second);
        }
    }
    std::set<std::string> models_to_delete;
    for (const auto &it : models_cached) {
        if (models_needed.count(it) == 0) {
            models_to_delete.insert((fs::u8path(project_pool.get_base_path()) / it).u8string());
        }
    }

    PoolCacheCleanupDialog dia(parent, files_to_delete, models_to_delete, project_pool);
    return dia.run() == Gtk::RESPONSE_OK;
}

void PoolProjectManagerAppWindow::update_pool_cache_status_now()
{
    // kept for when the pool cache monitor makes a comeback
}

void PoolProjectManagerAppWindow::set_version_info(const std::string &s)
{
    if (s.size()) {
        info_bar_show(info_bar_version);
        version_label->set_markup(s);
    }
    else {
        info_bar_hide(info_bar_version);
    }
}

void PoolProjectManagerAppWindow::update_recent_search(Gtk::SearchEntry &entry, Gtk::ListBox &lb)
{
    std::string needle = entry.get_text();
    trim(needle);
    const auto uneedle = Glib::ustring(needle).casefold();
    for (auto child : lb.get_children()) {
        if (auto row = dynamic_cast<Gtk::ListBoxRow *>(child)) {
            if (auto box = dynamic_cast<RecentItemBox *>(row->get_child())) {
                const bool visible = (uneedle.size() == 0)
                                     || (Glib::ustring(box->get_name()).casefold().find(needle) != Glib::ustring::npos);
                row->set_visible(visible);
            }
        }
    }
}

void PoolProjectManagerAppWindow::update_recent_placeholder(Gtk::Stack &stack, Gtk::ListBox &lb)
{
    const bool has_children = lb.get_children().size();
    if (has_children)
        stack.set_visible_child("search");
    else
        stack.set_visible_child("none");
}

void PoolProjectManagerAppWindow::update_recent_searches()
{
    update_recent_search(*recent_pools_search_entry, *recent_pools_listbox);
    update_recent_search(*recent_projects_search_entry, *recent_projects_listbox);
}

void PoolProjectManagerAppWindow::clear_recent_searches()
{
    recent_pools_search_entry->set_text("");
    recent_projects_search_entry->set_text("");
}

} // namespace horizon
