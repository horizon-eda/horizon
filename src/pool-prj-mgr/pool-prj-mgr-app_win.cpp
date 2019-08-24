#include "pool-prj-mgr-app_win.hpp"
#include "pool-prj-mgr-app.hpp"
#include <iostream>
#include <thread>
#include "util/util.hpp"
#include "pool/pool_manager.hpp"
#include "pool-mgr/pool_notebook.hpp"
#include "pool-update/pool-update.hpp"
#include "util/gtk_util.hpp"
#include "util/recent_util.hpp"
#include "widgets/recent_item_box.hpp"
#include "nlohmann/json.hpp"
#include "pool-mgr/editors/editor_window.hpp"
#include "prj-mgr/part_browser/part_browser_window.hpp"
#include "prj-mgr/pool_cache_window.hpp"
#include "prj-mgr/pool_cache_cleanup_dialog.hpp"
#include "close_utils.hpp"
#include "schematic/schematic.hpp"
#include "board/board.hpp"
#include "widgets/pool_chooser.hpp"

namespace horizon {
PoolProjectManagerAppWindow::PoolProjectManagerAppWindow(BaseObjectType *cobject,
                                                         const Glib::RefPtr<Gtk::Builder> &refBuilder,
                                                         PoolProjectManagerApplication *app)
    : Gtk::ApplicationWindow(cobject), builder(refBuilder), state_store(this, "pool-mgr"),
      view_create_project(refBuilder, this), view_project(refBuilder, this), view_create_pool(refBuilder, this),
      sock_mgr(app->zctx, ZMQ_REP), zctx(app->zctx)
{
    builder->get_widget("stack", stack);
    builder->get_widget("button_open", button_open);
    builder->get_widget("button_close", button_close);
    builder->get_widget("button_update", button_update);
    builder->get_widget("spinner_update", spinner_update);
    builder->get_widget("button_download", button_download);
    builder->get_widget("button_do_download", button_do_download);
    builder->get_widget("button_cancel", button_cancel);
    builder->get_widget("button_create", button_create);
    builder->get_widget("button_new", button_new);
    builder->get_widget("button_save", button_save);
    builder->get_widget("header", header);
    builder->get_widget("recent_pools_listbox", recent_pools_listbox);
    builder->get_widget("recent_projects_listbox", recent_projects_listbox);
    builder->get_widget("pool_box", pool_box);
    builder->get_widget("pool_update_status_label", pool_update_status_label);
    builder->get_widget("pool_update_status_rev", pool_update_status_rev);
    builder->get_widget("pool_update_status_close_button", pool_update_status_close_button);
    builder->get_widget("pool_update_progress", pool_update_progress);
    builder->get_widget("download_revealer", download_revealer);
    builder->get_widget("download_label", download_label);
    builder->get_widget("download_spinner", download_spinner);
    builder->get_widget("download_gh_repo_entry", download_gh_repo_entry);
    builder->get_widget("download_gh_username_entry", download_gh_username_entry);
    builder->get_widget("download_dest_dir_button", download_dest_dir_button);
    builder->get_widget("download_dest_dir_entry", download_dest_dir_entry);
    builder->get_widget("info_bar", info_bar);
    builder->get_widget("info_bar_label", info_bar_label);
    builder->get_widget("menu_new_pool", menu_new_pool);
    builder->get_widget("menu_new_project", menu_new_project);
    builder->get_widget("hamburger_menu_button", hamburger_menu_button);
    set_view_mode(ViewMode::OPEN);

    {
        auto rb = Gtk::Builder::create();
        rb->add_from_resource("/net/carrotIndustries/horizon/pool-prj-mgr/app_menu.ui");

        auto object = rb->get_object("appmenu");
        auto app_menu = Glib::RefPtr<Gio::MenuModel>::cast_dynamic(object);

        hamburger_menu_button->set_menu_model(app_menu);
    }

    pool_update_progress->set_pulse_step(.001);

    button_open->signal_clicked().connect(sigc::mem_fun(*this, &PoolProjectManagerAppWindow::handle_open));
    button_close->signal_clicked().connect(sigc::mem_fun(*this, &PoolProjectManagerAppWindow::handle_close));
    button_update->signal_clicked().connect(sigc::mem_fun(*this, &PoolProjectManagerAppWindow::handle_update));
    button_download->signal_clicked().connect(sigc::mem_fun(*this, &PoolProjectManagerAppWindow::handle_download));
    button_do_download->signal_clicked().connect(
            sigc::mem_fun(*this, &PoolProjectManagerAppWindow::handle_do_download));
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
#if GTK_CHECK_VERSION(3, 22, 29)
            gtk_info_bar_set_revealed(info_bar->gobj(), false);
#endif
            info_bar->hide();
        }
    });
    info_bar->hide();

    view_create_project.signal_valid_change().connect([this](bool v) { button_create->set_sensitive(v); });
    view_create_pool.signal_valid_change().connect([this](bool v) { button_create->set_sensitive(v); });

    download_status_dispatcher.attach(download_revealer);
    download_status_dispatcher.attach(download_label);
    download_status_dispatcher.attach(download_spinner);

    download_status_dispatcher.signal_notified().connect([this](const StatusDispatcher::Notification &n) {
        auto is_busy = n.status == StatusDispatcher::Status::BUSY;
        button_cancel->set_sensitive(!is_busy);
        button_do_download->set_sensitive(!is_busy);
        if (n.status == StatusDispatcher::Status::DONE) {
            PoolManager::get().add_pool(download_dest_dir_entry->get_text());
            open_file_view(
                    Gio::File::create_for_path(Glib::build_filename(download_dest_dir_entry->get_text(), "pool.json")));
        }
    });

    download_dest_dir_button->signal_clicked().connect([this] {
        GtkFileChooserNative *native = gtk_file_chooser_native_new(
                "Select", GTK_WINDOW(this->gobj()), GTK_FILE_CHOOSER_ACTION_CREATE_FOLDER, "Set", "_Cancel");
        auto chooser = Glib::wrap(GTK_FILE_CHOOSER(native));
        chooser->set_filename(download_dest_dir_entry->get_text());

        if (gtk_native_dialog_run(GTK_NATIVE_DIALOG(native)) == GTK_RESPONSE_ACCEPT) {
            download_dest_dir_entry->set_text(chooser->get_filename());
        }
    });

    sock_mgr.bind("tcp://127.0.0.1:*");
    {
        char ep[1024];
        size_t sz = sizeof(ep);
        sock_mgr.getsockopt(ZMQ_LAST_ENDPOINT, ep, &sz);
        sock_mgr_ep = ep;
    }

    Glib::RefPtr<Glib::IOChannel> chan;
#ifdef G_OS_WIN32
    SOCKET fd = sock_mgr.getsockopt<SOCKET>(ZMQ_FD);
    chan = Glib::IOChannel::create_from_win32_socket(fd);
#else
    int fd = sock_mgr.getsockopt<int>(ZMQ_FD);
    chan = Glib::IOChannel::create_from_fd(fd);
#endif

    Glib::signal_io().connect(sigc::track_obj(
                                      [this](Glib::IOCondition cond) {
                                          while (sock_mgr.getsockopt<int>(ZMQ_EVENTS) & ZMQ_POLLIN) {
                                              zmq::message_t msg;
                                              sock_mgr.recv(&msg);
                                              char *data = (char *)msg.data();
                                              json jrx = json::parse(data);
                                              json jtx = handle_req(jrx);

                                              std::string stx = jtx.dump();
                                              zmq::message_t tx(stx.size() + 1);
                                              memcpy(((uint8_t *)tx.data()), stx.c_str(), stx.size());
                                              auto m = (char *)tx.data();
                                              m[tx.size() - 1] = 0;
                                              sock_mgr.send(tx);
                                          }
                                          return true;
                                      },
                                      *this),
                              chan, Glib::IO_IN | Glib::IO_HUP);


    Glib::signal_idle().connect_once([this] {
        update_recent_items();
        check_pools();
    });

    for (auto &lb : recent_listboxes) {
        lb->set_header_func(sigc::ptr_fun(header_func_separator));
        lb->signal_row_activated().connect([this](Gtk::ListBoxRow *row) {
            auto ch = dynamic_cast<RecentItemBox *>(row->get_child());
            open_file_view(Gio::File::create_for_path(ch->path));
        });
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
    return find_process(project->get_top_block().schematic_filename);
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
    std::string op = j.at("op");
    auto app = Glib::RefPtr<PoolProjectManagerApplication>::cast_dynamic(get_application());
    guint32 timestamp = j.value("time", 0);
    if (op == "part-placed") {
        UUID part = j.at("part").get<std::string>();
        part_browser_window->placed_part(part);
    }
    else if (op == "show-browser") {
        part_browser_window->present(timestamp);
    }
    else if (op == "schematic-select") {
        if (auto proc = find_board_process()) {
            auto pid = proc->proc->get_pid();
            json tx;
            tx["op"] = "highlight";
            tx["objects"] = j.at("selection");
            app->send_json(pid, tx);
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
            app->send_json(pid, tx);
        }
    }
    else if (op == "to-board") {
        if (auto proc = find_board_process()) {
            auto pid = proc->proc->get_pid();
            json tx;
            tx["op"] = "place";
            tx["time"] = timestamp;
            tx["components"] = j.at("selection");
            app->send_json(pid, tx);
        }
    }
    else if (op == "show-in-browser") {
        part_browser_window->go_to_part(UUID(j.at("part").get<std::string>()));
        part_browser_window->present(timestamp);
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
            app->send_json(pid, tx);
        }
    }
    else if (op == "present-board") {
        if (auto proc = find_board_process()) {
            auto pid = proc->proc->get_pid();
            json tx;
            tx["op"] = "present";
            tx["time"] = timestamp;
            app->send_json(pid, tx);
        }
    }
    else if (op == "present-schematic") {
        if (auto proc = find_top_schematic_process()) {
            auto pid = proc->proc->get_pid();
            json tx;
            tx["op"] = "present";
            tx["time"] = timestamp;
            app->send_json(pid, tx);
        }
    }
    else if (op == "needs-save") {
        int pid = j.at("pid");
        bool needs_save = j.at("needs_save");
        std::cout << "needs save " << pid << " " << needs_save << std::endl;
        pids_need_save[pid] = needs_save;
    }
    else if (op == "edit") {
        auto type = object_type_lut.lookup(j.at("type"));
        UUID uu(j.at("uuid").get<std::string>());
        try {
            UUID this_pool_uuid = PoolManager::get().get_pools().at(pool->get_base_path()).uuid;
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
            spawn(ptype, {path}, {}, this_pool_uuid && (item_pool_uuid != this_pool_uuid));
        }
        catch (const std::exception &e) {
            Gtk::MessageDialog md(*this, "Can't open editor", false /* use_markup */, Gtk::MESSAGE_ERROR,
                                  Gtk::BUTTONS_OK);
            md.set_secondary_text(e.what());
            md.run();
        }
    }
    else if (op == "preferences") {
        app->show_preferences_window(timestamp);
    }
    else if (op == "show-in-pool-mgr") {
        UUID uu = j.at("uuid").get<std::string>();
        UUID pool_uu = j.at("pool_uuid").get<std::string>();
        ObjectType type = object_type_lut.lookup(j.at("type"));
        bool shown = false;
        for (auto win : app->get_windows()) {
            auto w = dynamic_cast<PoolProjectManagerAppWindow *>(win);
            if (w) {
                if (w->get_view_mode() == ViewMode::POOL && (w->get_pool_uuid() == pool_uu)) {
                    w->pool_notebook_go_to(type, uu);
                    w->present(timestamp);
                    shown = true;
                }
            }
        }
        if (!shown) { // need to open
            auto pool2 = PoolManager::get().get_by_uuid(pool_uu);
            if (pool2) {
                auto pool_json = Glib::build_filename(pool2->base_path, "pool.json");
                app->open_pool(pool_json, type, uu, timestamp);
            }
        }
    }
    else if (op == "backannotate") {
        if (auto proc = find_top_schematic_process()) {
            auto pid = proc->proc->get_pid();
            json tx;
            tx["op"] = "backannotate";
            tx["connections"] = j.at("connections");
            tx["time"] = timestamp;
            app->send_json(pid, tx);
        }
    }
    return nullptr;
}


void PoolProjectManagerAppWindow::set_pool_updating(bool v, bool success)
{
    button_update->set_sensitive(!v);
    pool_update_status_close_button->set_visible(!success);
    if (success) {
        if (v) { // show immediately
            pool_update_status_rev->set_reveal_child(v);
        }
        else {
            Glib::signal_timeout().connect(
                    [this] {
                        pool_update_status_rev->set_reveal_child(false);
                        return false;
                    },
                    500);
        }
    }
    if (v)
        spinner_update->start();
    else
        spinner_update->stop();
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
    if (part_browser_window)
        delete part_browser_window;
    if (pool_cache_window)
        delete pool_cache_window;
}

void PoolProjectManagerAppWindow::handle_recent()
{
    // auto file = Gio::File::create_for_uri(recent_chooser->get_current_uri());
    // open_file_view(file);
}

void PoolProjectManagerAppWindow::handle_download()
{
    set_view_mode(ViewMode::DOWNLOAD);
}

void PoolProjectManagerAppWindow::handle_cancel()
{
    set_view_mode(ViewMode::OPEN);
}

void PoolProjectManagerAppWindow::save_project()
{
    project->title = view_project.entry_project_title->get_text();
    save_json_to_file(project_filename, project->serialize());
    project_needs_save = false;
}

void PoolProjectManagerAppWindow::handle_save()
{
    if (project) {
        save_project();
        auto app = Glib::RefPtr<PoolProjectManagerApplication>::cast_dynamic(get_application());
        app->send_json(0, {{"op", "save"}});
    }
}

void PoolProjectManagerAppWindow::handle_open()
{
    GtkFileChooserNative *native = gtk_file_chooser_native_new("Open Pool", GTK_WINDOW(gobj()),
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
    if (pool_notebook) {
        pool_notebook->pool_update();
    }
}

void PoolProjectManagerAppWindow::handle_new_project()
{
    if (!check_pools())
        return;
    set_view_mode(ViewMode::CREATE_PROJECT);
}

void PoolProjectManagerAppWindow::handle_new_pool()
{
    set_view_mode(ViewMode::CREATE_POOL);
}

void PoolProjectManagerAppWindow::handle_create()
{
    if (view_mode == ViewMode::CREATE_PROJECT) {
        auto r = view_create_project.create();
        if (r.first) {
            view_create_project.clear();
            open_file_view(Gio::File::create_for_path(r.second));
        }
    }
    else if (view_mode == ViewMode::CREATE_POOL) {
        auto r = view_create_pool.create();
        if (r.first) {
            view_create_pool.clear();
            PoolManager::get().add_pool(Glib::path_get_dirname(r.second));
            open_file_view(Gio::File::create_for_path(r.second));
        }
    }
}

void PoolProjectManagerAppWindow::update_recent_items()
{
    auto app = Glib::RefPtr<PoolProjectManagerApplication>::cast_dynamic(get_application());
    if (!app)
        return;
    {
        for (auto &lb : recent_listboxes) {
            auto chs = lb->get_children();
            for (auto it : chs) {
                delete it;
            }
        }
    }
    std::vector<std::pair<std::string, Glib::DateTime>> recent_items_sorted = recent_sort(app->recent_items);
    for (const auto &it : recent_items_sorted) {
        const std::string &path = it.first;
        std::string name;
        try {
            auto ifs = make_ifstream(path);
            if (ifs.is_open()) {
                json k;
                ifs >> k;
                if (endswith(path, "pool.json"))
                    name = k.at("name").get<std::string>();
                else
                    name = k.at("title").get<std::string>();
            }
            ifs.close();
        }
        catch (...) {
            name = "error opening!";
        }
        auto box = Gtk::manage(new RecentItemBox(name, it.first, it.second));
        if (endswith(path, "pool.json"))
            recent_pools_listbox->append(*box);
        else
            recent_projects_listbox->append(*box);
        box->show();
    }
}

void PoolProjectManagerAppWindow::prepare_close()
{
    if (pool_notebook)
        pool_notebook->prepare_close();
}

bool PoolProjectManagerAppWindow::close_pool_or_project()
{
    auto app = Glib::RefPtr<PoolProjectManagerApplication>::cast_dynamic(get_application());
    auto pol = get_close_policy();

    if (!pol.can_close) {
        Gtk::MessageDialog md(*this, "Can't close right now", false /* use_markup */, Gtk::MESSAGE_ERROR,
                              Gtk::BUTTONS_OK);
        md.set_secondary_text(pol.reason);
        md.run();
        return false;
    }
    else if (pol.procs_need_save.size()) {
        ConfirmCloseDialog dia(this);
        std::map<std::string, std::map<UUID, std::string>> files;
        auto &this_files = files[get_filename()];
        for (const auto &it : pol.procs_need_save) {
            this_files[it] = get_proc_filename(it);
        }
        dia.set_files(files);
        auto r = dia.run();
        if (r == ConfirmCloseDialog::RESPONSE_NO_SAVE || r == ConfirmCloseDialog::RESPONSE_SAVE) { // save
            prepare_close();
            auto files_from_dia = dia.get_files();
            auto &this_files_from_dia = files_from_dia.at(get_filename());
            if (r == ConfirmCloseDialog::RESPONSE_SAVE) {
                for (auto &it : this_files_from_dia) { // files that need save
                    process_save(it);
                }
            }
            std::set<UUID> open_procs;
            for (auto &it : processes) {
                open_procs.emplace(it.first);
            }
            for (auto &it : open_procs) {
                process_close(it);
            }
            wait_for_all_processes();
            return really_close_pool_or_project();
        }
        else {
            return false;
        }
    }
    else {
        prepare_close();
        std::set<UUID> open_procs;
        for (auto &it : processes) {
            open_procs.emplace(it.first);
        }
        for (auto &it : open_procs) {
            process_close(it);
        }
        wait_for_all_processes();
        return really_close_pool_or_project();
    }

    return really_close_pool_or_project();
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
    auto app = Glib::RefPtr<PoolProjectManagerApplication>::cast_dynamic(get_application());
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
        app->recent_items[Glib::build_filename(pool->get_base_path(), "pool.json")] =
                Glib::DateTime::create_now_local();
        delete pool_notebook;
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
        app->recent_items[project_filename] = Glib::DateTime::create_now_local();
        project.reset();
        if (part_browser_window)
            delete part_browser_window;
        part_browser_window = nullptr;
        if (pool_cache_window)
            delete pool_cache_window;
        pool_cache_window = nullptr;
        set_view_mode(ViewMode::OPEN);
    }
    else {
        set_view_mode(ViewMode::OPEN);
    }
    return true;
}

void PoolProjectManagerAppWindow::set_view_mode(ViewMode mode)
{
    button_open->hide();
    button_close->hide();
    button_update->hide();
    button_download->hide();
    button_do_download->hide();
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
        button_download->show();
        button_new->show();
        hamburger_menu_button->show();
        header->set_title("Horizon EDA");
        update_recent_items();
        break;

    case ViewMode::POOL:
        stack->set_visible_child("pool");
        button_close->show();
        button_update->show();
        hamburger_menu_button->show();
        header->set_title("Pool manager");
        break;

    case ViewMode::DOWNLOAD:
        stack->set_visible_child("download");
        button_cancel->show();
        button_do_download->show();
        header->set_show_close_button(false);
        download_revealer->set_reveal_child(true);
        download_revealer->set_reveal_child(false);
        header->set_title("Horizon EDA");
        download_dest_dir_entry->set_text(
                Glib::build_filename(Glib::get_user_special_dir(Glib::USER_DIRECTORY_DOCUMENTS), "horizon-pool"));
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
        view_create_project.populate_pool_combo(get_application());
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

PoolProjectManagerAppWindow *PoolProjectManagerAppWindow::create(PoolProjectManagerApplication *app)
{
    // Load the Builder file and instantiate its widgets.
    std::vector<Glib::ustring> widgets = {"app_window",        "sg_dest",         "sg_repo",          "menu1",
                                          "sg_base_path",      "sg_project_name", "sg_project_title", "sg_pool_name",
                                          "sg_pool_base_path", "sg_prj_pool"};
    auto refBuilder =
            Gtk::Builder::create_from_resource("/net/carrotIndustries/horizon/pool-prj-mgr/window.ui", widgets);

    PoolProjectManagerAppWindow *window = nullptr;
    refBuilder->get_widget_derived("app_window", window, app);

    if (!window)
        throw std::runtime_error("No \"app_window\" object in window.ui");
    return window;
}

class ForcedPoolUpdateDialog : public Gtk::Dialog {
public:
    ForcedPoolUpdateDialog(const std::string &bp, Gtk::Window *parent);

private:
    std::string base_path;
    Glib::Dispatcher dispatcher;
    std::mutex pool_update_status_queue_mutex;
    std::deque<std::tuple<PoolUpdateStatus, std::string, std::string>> pool_update_status_queue;
    Gtk::Label *filename_label = nullptr;
    void pool_update_thread();
};

ForcedPoolUpdateDialog::ForcedPoolUpdateDialog(const std::string &bp, Gtk::Window *parent)
    : Gtk::Dialog("Pool updating", *parent, Gtk::DIALOG_MODAL), base_path(bp)
{
    auto hb = Gtk::manage(new Gtk::HeaderBar);
    hb->set_show_close_button(false);
    hb->set_title("Pool updating");
    set_titlebar(*hb);
    hb->show_all();

    auto box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL, 10));
    {
        auto box2 = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 10));
        auto la = Gtk::manage(new Gtk::Label("Updating pool for new schema"));
        box2->pack_start(*la, true, true, 0);
        auto sp = Gtk::manage(new Gtk::Spinner);
        sp->start();
        box2->pack_start(*sp, false, false, 0);
        box->pack_start(*box2, false, false, 0);
    }
    filename_label = Gtk::manage(new Gtk::Label);
    filename_label->get_style_context()->add_class("dim-label");
    filename_label->set_ellipsize(Pango::ELLIPSIZE_START);
    box->pack_start(*filename_label, false, false, 0);

    box->property_margin() = 10;
    box->show_all();
    get_content_area()->pack_start(*box, true, true, 0);

    auto thr = std::thread(&ForcedPoolUpdateDialog::pool_update_thread, this);
    thr.detach();

    dispatcher.connect([this] {
        std::lock_guard<std::mutex> guard(pool_update_status_queue_mutex);
        while (pool_update_status_queue.size()) {
            std::string last_filename;
            std::string last_msg;
            PoolUpdateStatus last_status;

            std::tie(last_status, last_filename, last_msg) = pool_update_status_queue.front();
            filename_label->set_text(last_filename);
            if (last_status == PoolUpdateStatus::DONE) {
                response(1);
            }

            pool_update_status_queue.pop_front();
        }
    });
}

void ForcedPoolUpdateDialog::pool_update_thread()
{
    pool_update(
            base_path,
            [this](PoolUpdateStatus st, std::string filename, std::string msg) {
                {
                    std::lock_guard<std::mutex> guard(pool_update_status_queue_mutex);
                    pool_update_status_queue.emplace_back(st, filename, msg);
                }
                dispatcher.emit();
            },
            true);
}

bool PoolProjectManagerAppWindow::check_schema_update(const std::string &base_path)
{
    bool update_required = false;
    try {
        Pool my_pool(base_path);
        int user_version = my_pool.db.get_user_version();
        int required_version = my_pool.get_required_schema_version();
        update_required = user_version != required_version;
    }
    catch (...) {
        update_required = true;
    }
    if (update_required) {
        // Gtk::MessageDialog md("Schema update required", false /* use_markup */, Gtk::MESSAGE_ERROR,
        // Gtk::BUTTONS_OK); md.run(); horizon::pool_update(base_path);
        ForcedPoolUpdateDialog dia(base_path, this);
        while (dia.run() != 1) {
        }
    }
    return true;
}

void PoolProjectManagerAppWindow::open_file_view(const Glib::RefPtr<Gio::File> &file)
{
    auto path = file->get_path();
    auto app = Glib::RefPtr<PoolProjectManagerApplication>::cast_dynamic(get_application());
    app->recent_items[path] = Glib::DateTime::create_now_local();

    auto windows = dynamic_cast_vector<PoolProjectManagerAppWindow *>(app->get_windows());
    for (auto &win : windows) {
        if (win->get_filename() == path) {
            win->present();
            return;
        }
    }

    auto basename = file->get_basename();
    if (basename == "pool.json") {
        try {
            auto pool_base_path = file->get_parent()->get_path();
            check_schema_update(pool_base_path);
            set_view_mode(ViewMode::POOL);
            pool_notebook = new PoolNotebook(pool_base_path, this);
            pool_box->pack_start(*pool_notebook, true, true, 0);
            pool_notebook->show();
            header->set_subtitle(pool_base_path);
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
        project = std::make_unique<Project>(Project::new_from_file(path));
        project_filename = path;
        bool modified = false;
        auto prj_pool = PoolManager::get().get_by_uuid(project->pool_uuid);
        if (prj_pool == nullptr) {
            PoolChooserDialog dia(this, "Pool " + (std::string)project->pool_uuid
                                                + " is not available. Pick another pool to open this project.");
            if (dia.run() == Gtk::RESPONSE_OK) {
                project->pool_uuid = dia.get_selected_pool();
                prj_pool = PoolManager::get().get_by_uuid(project->pool_uuid);
                modified = true;
            }
            else {
                project.reset();
                return;
            }
        }
        std::string pool_path = prj_pool->base_path;
        check_schema_update(pool_path);

        header->set_subtitle(project->title);
        view_project.label_project_directory->set_text(Glib::path_get_dirname(project_filename));
        view_project.entry_project_title->set_text(project->title);
        view_project.entry_project_title->grab_focus_without_selecting();


        view_project.pool_info_bar->hide();
        view_project.label_pool_name->set_text(prj_pool->name);
        view_project.label_pool_path->set_text(prj_pool->base_path);

        part_browser_window = PartBrowserWindow::create(this, prj_pool->base_path, app->part_favorites);
        part_browser_window->signal_place_part().connect(
                sigc::mem_fun(*this, &PoolProjectManagerAppWindow::handle_place_part));
        part_browser_window->signal_assign_part().connect(
                sigc::mem_fun(*this, &PoolProjectManagerAppWindow::handle_assign_part));
        pool_cache_window = PoolCacheWindow::create(this, project->pool_cache_directory, prj_pool->base_path, this);
        project_needs_save = modified;
        set_view_mode(ViewMode::PROJECT);
    }
}

void PoolProjectManagerAppWindow::open_pool(const std::string &pool_json, ObjectType type, const UUID &uu)
{
    open_file_view(Gio::File::create_for_path(pool_json));
    pool_notebook_go_to(type, uu);
}


void PoolProjectManagerAppWindow::handle_place_part(const UUID &uu)
{
    auto app = Glib::RefPtr<PoolProjectManagerApplication>::cast_dynamic(get_application());
    if (auto proc = find_top_schematic_process()) {
        auto pid = proc->proc->get_pid();
        allow_set_foreground_window(pid);
        app->send_json(pid, {{"op", "place-part"}, {"part", (std::string)uu}});
    }
}

void PoolProjectManagerAppWindow::handle_assign_part(const UUID &uu)
{
    auto app = Glib::RefPtr<PoolProjectManagerApplication>::cast_dynamic(get_application());
    if (auto proc = find_top_schematic_process()) {
        auto pid = proc->proc->get_pid();
        allow_set_foreground_window(pid);
        app->send_json(pid, {{"op", "assign-part"}, {"part", (std::string)uu}});
    }
}

bool PoolProjectManagerAppWindow::on_delete_event(GdkEventAny *ev)
{
    // returning false will destroy the window
    std::cout << "close" << std::endl;
    return !close_pool_or_project();
}

PoolProjectManagerProcess *PoolProjectManagerAppWindow::spawn(PoolProjectManagerProcess::Type type,
                                                              const std::vector<std::string> &args,
                                                              const std::vector<std::string> &ienv, bool read_only,
                                                              bool is_temp)
{
    auto app = Glib::RefPtr<PoolProjectManagerApplication>::cast_dynamic(get_application());
    std::string pool_base_path;
    if (pool)
        pool_base_path = pool->get_base_path();
    else if (project)
        pool_base_path = PoolManager::get().get_by_uuid(project->pool_uuid)->base_path;
    else
        throw std::runtime_error("can't locate pool");
    if (find_process(args.at(0)) == nullptr || args.at(0).size() == 0) { // need to launch imp

        std::vector<std::string> env = {"HORIZON_POOL=" + pool_base_path,
                                        "HORIZON_EP_BROADCAST=" + app->get_ep_broadcast(),
                                        "HORIZON_EP_MGR=" + sock_mgr_ep, "HORIZON_MGR_PID=" + std::to_string(getpid())};
        env.insert(env.end(), ienv.begin(), ienv.end());
        std::string filename = args.at(0);
        if (filename.size()) {
            if (!Glib::file_test(filename, Glib::FILE_TEST_IS_REGULAR)) {
                auto top = dynamic_cast<Gtk::Window *>(get_ancestor(GTK_TYPE_WINDOW));
                Gtk::MessageDialog md(*top, "File not found", false /* use_markup */, Gtk::MESSAGE_ERROR,
                                      Gtk::BUTTONS_OK);
                md.set_secondary_text("Try updating the pool");
                md.run();
                return nullptr;
            }
        }
        auto uu = UUID::random();
        auto &proc =
                processes
                        .emplace(std::piecewise_construct, std::forward_as_tuple(uu),
                                 std::forward_as_tuple(uu, type, args, env, pool, pool_parametric, read_only, is_temp))
                        .first->second;
        if (proc.win && pool_notebook) {
            proc.win->signal_goto().connect(sigc::mem_fun(pool_notebook, &PoolNotebook::go_to));
        }
        proc.signal_exited().connect([uu, this](int status, bool need_update) {
            auto real_filename = processes.at(uu).get_filename();
            processes.erase(uu);
            s_signal_process_exited.emit(real_filename, status, need_update);
            if (status != 0) {
                info_bar_label->set_text("Editor for '" + real_filename + "' exited with status "
                                         + std::to_string(status));
                info_bar->show();
#if GTK_CHECK_VERSION(3, 22, 29)
                gtk_info_bar_set_revealed(info_bar->gobj(), true);
#endif
            }
        });
        return &proc;
    }
    else { // present imp
        auto proc = find_process(args.at(0));
        if (proc->proc) {
            auto pid = proc->proc->get_pid();
            Glib::RefPtr<PoolProjectManagerApplication>::cast_dynamic(get_application())
                    ->send_json(pid, {{"op", "present"}});
        }
        else {
            proc->win->present();
        }
        return proc;
    }
}

PoolProjectManagerProcess *PoolProjectManagerAppWindow::spawn_for_project(PoolProjectManagerProcess::Type type,
                                                                          const std::vector<std::string> &args)
{
    return spawn(type, args, {"HORIZON_POOL_CACHE=" + project->pool_cache_directory});
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
        auto app = Glib::RefPtr<PoolProjectManagerApplication>::cast_dynamic(get_application());
        app->send_json(pid, {{"op", "save"}});
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
        auto app = Glib::RefPtr<PoolProjectManagerApplication>::cast_dynamic(get_application());
        app->send_json(pid, {{"op", "close"}});
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

PoolProjectManagerAppWindow::ClosePolicy PoolProjectManagerAppWindow::get_close_policy()
{
    PoolProjectManagerAppWindow::ClosePolicy r;
    if (pool_notebook) {
        auto close_prohibited = pool_notebook->get_close_prohibited();
        r.can_close = !close_prohibited;
        r.reason = "Pool is updating or dialog is open";
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

void PoolProjectManagerAppWindow::cleanup_pool_cache()
{
    if (project == nullptr)
        return;
    auto pol = get_close_policy();
    if (pol.procs_need_save.size()) {
        Gtk::MessageDialog md(*pool_cache_window, "Close or save all open files first", false, Gtk::MESSAGE_INFO,
                              Gtk::BUTTONS_OK);
        md.run();
        return;
    }
    auto app = Glib::RefPtr<PoolProjectManagerApplication>::cast_dynamic(get_application());
    std::set<std::pair<ObjectType, UUID>> items_needed;

    PoolCached pool_cached(PoolManager::get().get_by_uuid(project->pool_uuid)->base_path,
                           project->pool_cache_directory);
    auto block = Block::new_from_file(project->get_top_block().block_filename, pool_cached);
    auto sch = Schematic::new_from_file(project->get_top_block().schematic_filename, block, pool_cached);
    sch.expand();
    ViaPadstackProvider vpp(project->vias_directory, pool_cached);
    auto board = Board::new_from_file(project->board_filename, block, pool_cached, vpp);
    board.expand();

    for (const auto &it : block.components) {
        items_needed.emplace(ObjectType::ENTITY, it.second.entity->uuid);
        for (const auto &it_gate : it.second.entity->gates) {
            items_needed.emplace(ObjectType::UNIT, it_gate.second.unit->uuid);
        }
        if (it.second.part) {
            const Part *part = it.second.part;
            while (part) {
                items_needed.emplace(ObjectType::PART, part->uuid);
                items_needed.emplace(ObjectType::PACKAGE, part->package.uuid);
                for (const auto &it_pad : part->package->pads) {
                    items_needed.emplace(ObjectType::PADSTACK, it_pad.second.pool_padstack->uuid);
                }
                if (part->base)
                    part = part->base;
                else
                    part = nullptr;
            }
        }
    }
    for (const auto &it_sheet : sch.sheets) {
        for (const auto &it_sym : it_sheet.second.symbols) {
            items_needed.emplace(ObjectType::SYMBOL, it_sym.second.pool_symbol->uuid);
        }
        if (it_sheet.second.pool_frame)
            items_needed.emplace(ObjectType::FRAME, it_sheet.second.pool_frame->uuid);
    }
    for (const auto &it_pkg : board.packages) {
        // don't use pool_package because of alternate pkg
        items_needed.emplace(ObjectType::PACKAGE, it_pkg.second.package.uuid);
        for (const auto &it_pad : it_pkg.second.package.pads) {
            items_needed.emplace(ObjectType::PADSTACK, it_pad.second.pool_padstack->uuid);
        }
    }
    for (const auto &it_via : board.vias) {
        items_needed.emplace(ObjectType::PADSTACK, it_via.second.vpp_padstack->uuid);
    }
    for (const auto &it_hole : board.holes) {
        items_needed.emplace(ObjectType::PADSTACK, it_hole.second.pool_padstack->uuid);
    }

    std::map<std::pair<ObjectType, UUID>, std::string> items_cached;
    Glib::Dir dir(project->pool_cache_directory);
    for (const auto &it : dir) {
        if (endswith(it, ".json")) {
            auto itempath = Glib::build_filename(project->pool_cache_directory, it);
            json j_cache;
            {
                j_cache = load_json_from_file(itempath);
                if (j_cache.count("_imp"))
                    j_cache.erase("_imp");
            }
            std::string type_str = j_cache.at("type");
            ObjectType type = object_type_lut.lookup(type_str);

            UUID uuid(j_cache.at("uuid").get<std::string>());
            items_cached.emplace(std::piecewise_construct, std::forward_as_tuple(type, uuid),
                                 std::forward_as_tuple(itempath));
        }
    }

    std::set<std::pair<std::string, UUID>> models_needed, models_cached;
    for (const auto &it_pkg : board.packages) {
        // don't use pool_package because of alternate pkg
        UUID pool_uuid;
        pool_cached.get_package(it_pkg.second.package.uuid, &pool_uuid);
        auto model = it_pkg.second.package.get_model(it_pkg.second.model);
        if (model) {
            models_needed.emplace(model->filename, pool_uuid);
        }
    }

    auto models_path = Glib::build_filename(project->pool_cache_directory, "3d_models");
    if (Glib::file_test(models_path, Glib::FILE_TEST_IS_DIR)) {
        Glib::Dir dir_3d(models_path);
        for (const auto &it : dir_3d) {
            if (it.find("pool_") == 0 && it.size() == 41) {
                UUID pool_uuid(it.substr(5));
                auto it_pool = PoolManager::get().get_by_uuid(pool_uuid);
                if (it_pool) {
                    auto pool_cache_path = Glib::build_filename(models_path, it);
                    find_files_recursive(pool_cache_path,
                                         [&models_cached, &pool_uuid](const std::string &model_filename) {
                                             models_cached.emplace(model_filename, pool_uuid);
                                         });
                }
            }
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
            models_to_delete.insert(Glib::build_filename(project->pool_cache_directory, "3d_models",
                                                         "pool_" + static_cast<std::string>(it.second), it.first));
        }
    }

    PoolCacheCleanupDialog dia(pool_cache_window, files_to_delete, models_to_delete, &pool_cached);
    dia.run();
}

} // namespace horizon
