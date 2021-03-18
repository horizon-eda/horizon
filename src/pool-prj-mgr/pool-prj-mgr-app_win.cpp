#include "pool-prj-mgr-app_win.hpp"
#include "pool-prj-mgr-app.hpp"
#include "preferences/preferences_window.hpp"
#include "pools_window/pools_window.hpp"
#include <iostream>
#include "util/util.hpp"
#include "pool/pool_manager.hpp"
#include "pool/project_pool.hpp"
#include "pool-mgr/pool_notebook.hpp"
#include "forced_pool_update_dialog.hpp"
#include "util/gtk_util.hpp"
#include "util/recent_util.hpp"
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
#include <filesystem>

#ifdef G_OS_WIN32
#include <winsock2.h>
#endif
#include "util/win32_undef.hpp"

namespace horizon {
namespace fs = std::filesystem;
PoolProjectManagerAppWindow::PoolProjectManagerAppWindow(BaseObjectType *cobject,
                                                         const Glib::RefPtr<Gtk::Builder> &refBuilder,
                                                         PoolProjectManagerApplication *aapp)
    : Gtk::ApplicationWindow(cobject), builder(refBuilder), state_store(this, "pool-mgr"),
      view_create_project(refBuilder, this), view_project(refBuilder, this), view_create_pool(refBuilder, this),
      app(aapp), sock_mgr(app->zctx, ZMQ_REP), zctx(app->zctx)
{
    builder->get_widget("stack", stack);
    builder->get_widget("button_open", button_open);
    builder->get_widget("button_close", button_close);
    builder->get_widget("button_update", button_update);
    builder->get_widget("spinner_update", spinner_update);
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
                auto pools_window = app->show_pools_window();
                pools_window->add_pool(Glib::build_filename(pool->get_base_path(), "pool.json"));
            }
        }
        info_bar_hide(info_bar_pool_not_added);
    });
    info_bar_hide(info_bar_pool_not_added);

    info_bar_pool_doc->signal_response().connect([this](int resp) {
        if (resp == Gtk::RESPONSE_OK) {
            info_bar_hide(info_bar_pool_doc);
            app->pool_doc_info_bar_dismissed = true;
        }
    });
    info_bar_hide(info_bar_pool_doc);

    show_output_button->signal_clicked().connect([this] {
        output_window->present();
        info_bar_hide(info_bar);
    });

    view_create_project.signal_valid_change().connect([this](bool v) { button_create->set_sensitive(v); });
    view_create_pool.signal_valid_change().connect([this](bool v) { button_create->set_sensitive(v); });

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
                                              if (zmq_helper::recv(sock_mgr, msg)) {
                                                  char *data = (char *)msg.data();
                                                  json jrx = json::parse(data);
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
                    auto w = WelcomeWindow::create(this);
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
    guint32 timestamp = j.value("time", 0);
    if (op == "part-placed") {
        UUID part = j.at("part").get<std::string>();
        part_browser_window->placed_part(part);
    }
    else if (op == "show-browser") {
        part_browser_window->present(timestamp);
        part_browser_window->focus_search();
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
            tx["time"] = timestamp;
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
            app->send_json(pid, tx);
        }
        else {
            view_project.open_top_schematic();
        }
    }
    else if (op == "needs-save") {
        int pid = j.at("pid");
        bool needs_save = j.at("needs_save");
        std::cout << "needs save " << pid << " " << needs_save << std::endl;
        pids_need_save[pid] = needs_save;
        if (project && !needs_save)
            view_project.update_meta();
        if (!needs_save) {
            s_signal_process_saved.emit(j.at("filename"));
            if (auto proc = find_top_schematic_process()) {
                if (proc->proc->get_pid() == pid) { // schematic got saved
                    if (auto proc_board = find_board_process()) {
                        auto board_pid = proc_board->proc->get_pid();
                        json tx;
                        tx["op"] = "reload-netlist-hint";
                        app->send_json(board_pid, tx);
                    }
                }
            }
        }
    }
    else if (op == "ready") {
        int pid = j.at("pid");
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
            spawn(ptype, {path}, {},
                  (item_pool_uuid != Pool::tmp_pool_uuid) && this_pool_uuid && (item_pool_uuid != this_pool_uuid));
        }
        catch (const std::exception &e) {
            Gtk::MessageDialog md(*this, "Can't open editor", false /* use_markup */, Gtk::MESSAGE_ERROR,
                                  Gtk::BUTTONS_OK);
            md.set_secondary_text(e.what());
            md.run();
        }
    }
    else if (op == "preferences") {
        auto win = app->show_preferences_window(timestamp);
        if (j.count("page"))
            win->show_page(j.at("page"));
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
            app->open_pool(Glib::build_filename(pool_path, "pool.json"), type, uu, timestamp);
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
        app->send_json(0, {{"op", "save"}});
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
    if (pool_notebook) {
        pool_notebook->pool_update();
    }
}

void PoolProjectManagerAppWindow::handle_new_project()
{
    if (!check_pools())
        return;
    view_create_project.clear();
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

static std::optional<std::string> peek_name(const std::string &path)
{
    std::string name;
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
            auto block_filename = prj.get_top_block().block_filename;
            auto meta = Block::peek_project_meta(block_filename);
            if (meta.count("project_title"))
                name = meta.at("project_title");
            if (!name.size())
                name = load_json_from_file(path).at("title");
        }
    }
    catch (...) {
        name = "error opening!";
    }
    return name;
}

void PoolProjectManagerAppWindow::update_recent_items()
{
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
        if (const auto name = peek_name(path)) {
            auto box = Gtk::manage(new RecentItemBox(*name, it.first, it.second));
            if (endswith(path, "pool.json"))
                recent_pools_listbox->append(*box);
            else
                recent_projects_listbox->append(*box);
            box->show();
            box->signal_remove().connect([this, box] {
                app->recent_items.erase(box->path);
                Glib::signal_idle().connect_once([this] { update_recent_items(); });
            });
        }
    }
}

void PoolProjectManagerAppWindow::prepare_close()
{
    if (pool_notebook)
        pool_notebook->prepare_close();
}

bool PoolProjectManagerAppWindow::close_pool_or_project()
{
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
        app->recent_items[project_filename] = Glib::DateTime::create_now_local();
        project.reset();
        cleanup();
        set_view_mode(ViewMode::OPEN);
    }
    else {
        set_view_mode(ViewMode::OPEN);
    }
    output_window->clear_all();
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
        break;

    case ViewMode::POOL:
        stack->set_visible_child("pool");
        button_close->show();
        button_update->show();
        hamburger_menu_button->show();
        header->set_title("Pool manager");
        if (!app->pool_doc_info_bar_dismissed)
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

PoolProjectManagerAppWindow *PoolProjectManagerAppWindow::create(PoolProjectManagerApplication *app)
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
        ForcedPoolUpdateDialog dia(base_path, this);
        while (dia.run() != 1) {
        }
    }
    return true;
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

void PoolProjectManagerAppWindow::open_file_view(const Glib::RefPtr<Gio::File> &file)
{
    auto path = file->get_path();
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
            if (!file->query_exists()) {
                throw std::runtime_error("pool.json not found");
            }
            auto pool_base_path = file->get_parent()->get_path();
            check_schema_update(pool_base_path);
            set_view_mode(ViewMode::POOL);
            pool_notebook = new PoolNotebook(pool_base_path, this);
            if (!PoolManager::get().get_pools().count(pool_base_path) && !pool->get_pool_info().is_project_pool()) {
                info_bar_show(info_bar_pool_not_added);
            }
            pool_box->pack_start(*pool_notebook, true, true, 0);
            pool_notebook->show();
            header->set_subtitle(pool_base_path);

            if (pool->get_pool_info().is_project_pool()) {
                button_close->hide();
                auto project_path = Glib::path_get_dirname(pool_base_path);
                for (auto win : app->get_windows()) {
                    if (auto w = dynamic_cast<PoolProjectManagerAppWindow *>(win)) {
                        if (w->get_view_mode() == PoolProjectManagerAppWindow::ViewMode::PROJECT) {
                            const auto win_path = Glib::path_get_dirname(w->get_filename());
                            if (win_path == project_path) {
                                set_title(w->get_project_title() + " - Pool");
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
        check_schema_update(project->pool_directory);

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
        }

        view_project.label_project_directory->set_text(Glib::path_get_dirname(project_filename));

        view_project.reset_pool_cache_status();

        part_browser_window = PartBrowserWindow::create(this, project->pool_directory, app->part_favorites);
        part_browser_window->signal_place_part().connect(
                sigc::mem_fun(*this, &PoolProjectManagerAppWindow::handle_place_part));
        part_browser_window->signal_assign_part().connect(
                sigc::mem_fun(*this, &PoolProjectManagerAppWindow::handle_assign_part));

        set_view_mode(ViewMode::PROJECT);

        if (view_project.update_meta() == false) {
            Gtk::MessageDialog md(*this, "Metadata update required", false /* use_markup */, Gtk::MESSAGE_ERROR,
                                  Gtk::BUTTONS_OK);
            md.set_secondary_text(
                    "Open and save the schematic for the project title to appear in the project manager.");
            md.run();
        }
    }
}

const std::string &PoolProjectManagerAppWindow::get_project_title() const
{
    if (!project)
        throw std::runtime_error("not a project");
    return project_title;
}

void PoolProjectManagerAppWindow::open_pool(const std::string &pool_json, ObjectType type, const UUID &uu)
{
    open_file_view(Gio::File::create_for_path(pool_json));
    if (type != ObjectType::INVALID)
        pool_notebook_go_to(type, uu);
}


void PoolProjectManagerAppWindow::handle_place_part(const UUID &uu)
{
    if (auto proc = find_top_schematic_process()) {
        auto pid = proc->proc->get_pid();
        allow_set_foreground_window(pid);
        app->send_json(pid, {{"op", "place-part"}, {"part", (std::string)uu}, {"time", gtk_get_current_event_time()}});
    }
}

void PoolProjectManagerAppWindow::handle_assign_part(const UUID &uu)
{
    if (auto proc = find_top_schematic_process()) {
        auto pid = proc->proc->get_pid();
        allow_set_foreground_window(pid);
        app->send_json(pid, {{"op", "assign-part"}, {"part", (std::string)uu}, {"time", gtk_get_current_event_time()}});
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
    std::string pool_base_path;
    if (pool)
        pool_base_path = pool->get_base_path();
    else if (project)
        pool_base_path = project->pool_directory;
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

        if (!check_autosave(type, args))
            return nullptr;

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
                show_output_button->set_visible(app->get_preferences().capture_output);
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

        return &proc;
    }
    else { // present imp
        auto proc = find_process(args.at(0));
        if (proc->proc) {
            auto pid = proc->proc->get_pid();
            allow_set_foreground_window(pid);
            app->send_json(pid, {{"op", "present"}, {"time", gtk_get_current_event_time()}});
        }
        else {
            proc->win->present();
        }
        return proc;
    }
}

bool PoolProjectManagerAppWindow::check_autosave(PoolProjectManagerProcess::Type type,
                                                 const std::vector<std::string> &filenames)
{
    if (filenames.size() == 0)
        return true;

    std::vector<std::string> my_filenames = filenames;
    if (type == PoolProjectManagerProcess::Type::IMP_SCHEMATIC) {
        my_filenames.resize(2);
    }
    else {
        my_filenames.resize(1);
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
    auto block = Block::new_from_file(project->get_top_block().block_filename, project_pool);
    auto sch = Schematic::new_from_file(project->get_top_block().schematic_filename, block, project_pool);
    sch.expand();
    ViaPadstackProvider vpp(project->vias_directory, project_pool);
    auto board = Board::new_from_file(project->board_filename, block, project_pool, vpp);
    board.expand();

    {
        auto items = block.get_pool_items_used();
        items_needed.insert(items.begin(), items.end());
    }
    {
        auto items = sch.get_pool_items_used();
        items_needed.insert(items.begin(), items.end());
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

} // namespace horizon
