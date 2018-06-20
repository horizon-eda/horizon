#include "pool-prj-mgr-app_win.hpp"
#include "pool-prj-mgr-app.hpp"
#include <iostream>
#include <thread>
#include "util/util.hpp"
#include "pool-mgr/pool_notebook.hpp"
#include "pool-update/pool-update.hpp"
#include "util/gtk_util.hpp"
#include "util/recent_util.hpp"
#include "widgets/recent_item_box.hpp"
#include "nlohmann/json.hpp"
#include "pool-mgr/editors/editor_window.hpp"
#include "prj-mgr/part_browser/part_browser_window.hpp"
#include "prj-mgr/pool_cache_window.hpp"

extern const char *gitversion;

namespace horizon {
PoolProjectManagerAppWindow::PoolProjectManagerAppWindow(BaseObjectType *cobject,
                                                         const Glib::RefPtr<Gtk::Builder> &refBuilder,
                                                         PoolProjectManagerApplication *app)
    : Gtk::ApplicationWindow(cobject), builder(refBuilder), state_store(this, "pool-mgr"),
      view_create_project(refBuilder, this), view_project(refBuilder, this), sock_mgr(app->zctx, ZMQ_REP),
      zctx(app->zctx)
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
    builder->get_widget("label_gitversion", label_gitversion);
    builder->get_widget("pool_box", pool_box);
    builder->get_widget("pool_update_status_label", pool_update_status_label);
    builder->get_widget("pool_update_status_rev", pool_update_status_rev);
    builder->get_widget("pool_update_status_close_button", pool_update_status_close_button);
    builder->get_widget("pool_update_progress", pool_update_progress);
    builder->get_widget("download_revealer", download_revealer);
    builder->get_widget("download_label", download_label);
    builder->get_widget("download_gh_repo_entry", download_gh_repo_entry);
    builder->get_widget("download_gh_username_entry", download_gh_username_entry);
    builder->get_widget("download_dest_dir_button", download_dest_dir_button);
    builder->get_widget("info_bar", info_bar);
    builder->get_widget("info_bar_label", info_bar_label);
    builder->get_widget("menu_new_pool", menu_new_pool);
    builder->get_widget("menu_new_project", menu_new_project);
    set_view_mode(ViewMode::OPEN);

    pool_update_progress->set_pulse_step(.001);

    button_open->signal_clicked().connect(sigc::mem_fun(this, &PoolProjectManagerAppWindow::handle_open));
    button_close->signal_clicked().connect(sigc::mem_fun(this, &PoolProjectManagerAppWindow::handle_close));
    button_update->signal_clicked().connect(sigc::mem_fun(this, &PoolProjectManagerAppWindow::handle_update));
    button_download->signal_clicked().connect(sigc::mem_fun(this, &PoolProjectManagerAppWindow::handle_download));
    button_do_download->signal_clicked().connect(sigc::mem_fun(this, &PoolProjectManagerAppWindow::handle_do_download));
    button_cancel->signal_clicked().connect(sigc::mem_fun(this, &PoolProjectManagerAppWindow::handle_cancel));
    menu_new_project->signal_activate().connect(sigc::mem_fun(this, &PoolProjectManagerAppWindow::handle_new_project));
    button_create->signal_clicked().connect(sigc::mem_fun(this, &PoolProjectManagerAppWindow::handle_create));
    button_save->signal_clicked().connect(sigc::mem_fun(this, &PoolProjectManagerAppWindow::handle_save));


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

    label_gitversion->set_text(gitversion);

    download_dispatcher.connect([this] {
        std::lock_guard<std::mutex> lock(download_mutex);
        download_revealer->set_reveal_child(downloading || download_error);
        download_label->set_text(download_status);
        if (!downloading) {
            button_cancel->set_sensitive(true);
            button_do_download->set_sensitive(true);
        }
        if (!downloading && !download_error) {
            open_file_view(Gio::File::create_for_path(
                    Glib::build_filename(download_dest_dir_button->get_filename(), "pool.json")));
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

    sock_mgr_conn = Glib::signal_io().connect(
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

bool PoolProjectManagerAppWindow::check_pools()
{
    auto mapp = Glib::RefPtr<PoolProjectManagerApplication>::cast_dynamic(get_application());
    if (mapp->pools.size() == 0) {
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
    if (op == "part-placed") {
        UUID part = j.at("part").get<std::string>();
        part_browser_window->placed_part(part);
    }
    else if (op == "show-browser") {
        part_browser_window->present();
    }
    else if (op == "schematic-select") {

        if (processes.count(project->board_filename)) {
            auto pid = processes.at(project->board_filename).proc->get_pid();
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
        auto top_block = project->get_top_block();
        if (processes.count(top_block.schematic_filename)) {
            auto pid = processes.at(top_block.schematic_filename).proc->get_pid();
            json tx;
            tx["op"] = "highlight";
            tx["objects"] = j.at("selection");
            app->send_json(pid, tx);
        }
    }
    else if (op == "to-board") {
        if (processes.count(project->board_filename)) {
            auto pid = processes.at(project->board_filename).proc->get_pid();
            json tx;
            tx["op"] = "place";
            tx["components"] = j.at("selection");
            app->send_json(pid, tx);
        }
    }
    else if (op == "show-in-browser") {
        part_browser_window->go_to_part(UUID(j.at("part").get<std::string>()));
        part_browser_window->present();
    }
    else if (op == "has-board") {
        return processes.count(project->board_filename) > 0;
    }
    else if (op == "get-board-pid") {
        if (processes.count(project->board_filename))
            return processes.at(project->board_filename).proc->get_pid();
        else
            return -1;
    }
    else if (op == "reload-netlist") {
        if (processes.count(project->board_filename)) {
            auto pid = processes.at(project->board_filename).proc->get_pid();
            json tx;
            tx["op"] = "reload-netlist";
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
    sock_mgr_conn.disconnect();
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

void PoolProjectManagerAppWindow::handle_save()
{
    if (project) {
        project->title = view_project.entry_project_title->get_text();
        save_json_to_file(project_filename, project->serialize());
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

void PoolProjectManagerAppWindow::handle_create()
{
    auto r = view_create_project.create();
    if (r.first) {
        view_create_project.clear();
        open_file_view(Gio::File::create_for_path(r.second));
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
            std::ifstream ifs(path);
            if (ifs.is_open()) {
                json k;
                ifs >> k;
                if (endswith(path, "pool.json"))
                    name = k.at("name");
                else
                    name = k.at("title");
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

bool PoolProjectManagerAppWindow::close_pool_or_project()
{
    auto app = Glib::RefPtr<PoolProjectManagerApplication>::cast_dynamic(get_application());
    if (pool_notebook) {
        if (!pool_notebook->can_close()) {
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

    switch (mode) {
    case ViewMode::OPEN:
        stack->set_visible_child("open");
        button_open->show();
        button_download->show();
        button_new->show();
        header->set_title("Horizon EDA");
        update_recent_items();
        break;

    case ViewMode::POOL:
        stack->set_visible_child("pool");
        button_close->show();
        button_update->show();
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
        break;

    case ViewMode::PROJECT:
        stack->set_visible_child("project");
        button_close->show();
        button_save->show();
        header->set_title("Project manager");
        break;

    case ViewMode::CREATE_PROJECT:
        stack->set_visible_child("create_project");
        header->set_show_close_button(false);
        button_cancel->show();
        button_create->show();
        view_create_project.populate_pool_combo(get_application());
        header->set_title("Horizon EDA");
        break;
    }
}

PoolProjectManagerAppWindow *PoolProjectManagerAppWindow::create(PoolProjectManagerApplication *app)
{
    // Load the Builder file and instantiate its widgets.
    std::vector<Glib::ustring> widgets = {"app_window",   "sg_dest",         "sg_repo",         "menu1",
                                          "sg_base_path", "sg_project_name", "sg_project_title"};
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
    pool_update(base_path, [this](PoolUpdateStatus st, std::string filename, std::string msg) {
        {
            std::lock_guard<std::mutex> guard(pool_update_status_queue_mutex);
            pool_update_status_queue.emplace_back(st, filename, msg);
        }
        dispatcher.emit();
    });
}

bool PoolProjectManagerAppWindow::check_schema_update(const std::string &base_path)
{
    bool update_required = false;
    try {
        Pool my_pool(base_path);
        int user_version = my_pool.db.get_user_version();
        int required_version = my_pool.get_required_schema_version();
        update_required = user_version < required_version;
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
        if (!app->pools.count(project->pool_uuid)) {
            throw std::runtime_error("pool not found");
        }
        std::string pool_path = app->pools.at(project->pool_uuid).path;
        check_schema_update(pool_path);

        header->set_subtitle(project->title);
        view_project.entry_project_title->set_text(project->title);


        view_project.label_pool_name->set_text(app->pools.at(project->pool_uuid).name);

        part_browser_window =
                PartBrowserWindow::create(this, app->pools.at(project->pool_uuid).path, app->part_favorites);
        part_browser_window->signal_place_part().connect(
                sigc::mem_fun(this, &PoolProjectManagerAppWindow::handle_place_part));
        part_browser_window->signal_assign_part().connect(
                sigc::mem_fun(this, &PoolProjectManagerAppWindow::handle_assign_part));
        pool_cache_window =
                PoolCacheWindow::create(this, project->pool_cache_directory, app->pools.at(project->pool_uuid).path);
        set_view_mode(ViewMode::PROJECT);
    }
}

void PoolProjectManagerAppWindow::handle_place_part(const UUID &uu)
{
    auto app = Glib::RefPtr<PoolProjectManagerApplication>::cast_dynamic(get_application());
    if (processes.count(project->get_top_block().schematic_filename)) {
        auto pid = processes.at(project->get_top_block().schematic_filename).proc->get_pid();
        allow_set_foreground_window(pid);
        app->send_json(pid, {{"op", "place-part"}, {"part", (std::string)uu}});
    }
}

void PoolProjectManagerAppWindow::handle_assign_part(const UUID &uu)
{
    auto app = Glib::RefPtr<PoolProjectManagerApplication>::cast_dynamic(get_application());
    if (processes.count(project->get_top_block().schematic_filename)) {
        auto pid = processes.at(project->get_top_block().schematic_filename).proc->get_pid();
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
                                                              const std::vector<std::string> &ienv)
{
    auto app = Glib::RefPtr<PoolProjectManagerApplication>::cast_dynamic(get_application());
    std::string pool_base_path;
    if (pool)
        pool_base_path = pool->get_base_path();
    else if (project)
        pool_base_path = app->pools.at(project->pool_uuid).path;
    else
        throw std::runtime_error("can't locate pool");
    if (processes.count(args.at(0)) == 0) { // need to launch imp

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
        auto &proc = processes
                             .emplace(std::piecewise_construct, std::forward_as_tuple(filename),
                                      std::forward_as_tuple(type, args, env, pool))
                             .first->second;
        proc.signal_exited().connect([filename, this](int status, bool need_update) {
            processes.erase(filename);
            s_signal_process_exited.emit(filename, status, need_update);
            if (status != 0) {
                info_bar_label->set_text("Editor for '" + filename + "' exited with status " + std::to_string(status));
                info_bar->show();
#if GTK_CHECK_VERSION(3, 22, 29)
                gtk_info_bar_set_revealed(info_bar->gobj(), true);
#endif
            }
        });
        return &proc;
    }
    else { // present imp
        auto &proc = processes.at(args.at(0));
        if (proc.proc) {
            auto pid = proc.proc->get_pid();
            Glib::RefPtr<PoolProjectManagerApplication>::cast_dynamic(get_application())
                    ->send_json(pid, {{"op", "present"}});
        }
        else {
            proc.win->present();
        }
        return &proc;
    }
}

PoolProjectManagerProcess *PoolProjectManagerAppWindow::spawn_for_project(PoolProjectManagerProcess::Type type,
                                                                          const std::vector<std::string> &args)
{
    return spawn(type, args, {"HORIZON_POOL_CACHE=" + project->pool_cache_directory});
}

std::map<std::string, PoolProjectManagerProcess *> PoolProjectManagerAppWindow::get_processes()
{
    std::map<std::string, PoolProjectManagerProcess *> r;
    for (auto &it : processes) {
        r.emplace(it.first, &it.second);
    }
    return r;
}


} // namespace horizon
