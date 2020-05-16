#include "canvas/canvas.hpp"
#include "pool_notebook.hpp"
#include "widgets/pool_browser.hpp"
#include "dialogs/pool_browser_dialog.hpp"
#include "widgets/pool_browser_parametric.hpp"
#include "util/util.hpp"
#include "pool-update/pool-update.hpp"
#include "pool-prj-mgr/pool-prj-mgr-app_win.hpp"
#include "duplicate/duplicate_window.hpp"
#include "common/object_descr.hpp"
#include "pool_remote_box.hpp"
#include "pool_merge_dialog.hpp"
#include "pool_git_box.hpp"
#include <git2.h>
#include "util/autofree_ptr.hpp"
#include "pool-prj-mgr/pool-prj-mgr-app.hpp"
#include "pool_update_error_dialog.hpp"
#include "pool_settings_box.hpp"
#include "pool/pool_manager.hpp"
#include "pool/pool_parametric.hpp"
#include "part_wizard/part_wizard.hpp"
#include "widgets/part_preview.hpp"
#include <thread>
#include "nlohmann/json.hpp"

#ifdef G_OS_WIN32
#undef ERROR
#endif

namespace horizon {

void PoolNotebook::pool_updated(bool success)
{
    pool_updating = false;
    if (pool_update_error_queue.size()) {
        auto top = dynamic_cast<Gtk::Window *>(get_ancestor(GTK_TYPE_WINDOW));
        PoolUpdateErrorDialog dia(top, pool_update_error_queue);
        dia.run();
    }
    appwin->set_pool_updating(false, success);
    reload();
    if (success && pool_update_done_cb) {
        pool_update_done_cb();
        pool_update_done_cb = nullptr;
    }
    if (success)
        Glib::RefPtr<PoolProjectManagerApplication>::cast_dynamic(appwin->get_application())
                ->signal_pool_updated()
                .emit(pool_uuid);
}

void PoolNotebook::reload()
{
    pool.clear();
    for (auto &br : browsers) {
        if (widget_is_visible(br.second))
            br.second->search();
        else
            br.second->clear_search_once();
    }
    for (auto &br : browsers_parametric) {
        if (widget_is_visible(br.second))
            br.second->search();
        else
            br.second->clear_search_once();
    }
    auto procs = appwin->get_processes();
    for (auto &it : procs) {
        it.second->reload();
    }
    if (settings_box)
        settings_box->pool_updated();
    if (part_wizard)
        part_wizard->reload();
    if (git_box && git_box->refreshed_once) {
        if (widget_is_visible(git_box))
            git_box->refresh();
        else
            git_box->refreshed_once = false;
    }
}

bool PoolNotebook::widget_is_visible(Gtk::Widget *widget)
{
    auto page = get_nth_page(get_current_page());
    return widget == page || widget->is_ancestor(*page);
}

PoolNotebook::~PoolNotebook()
{
    appwin->pool = nullptr;
    appwin->pool_parametric = nullptr;
}

Gtk::Button *PoolNotebook::add_action_button(const std::string &label, Gtk::Box *bbox, sigc::slot0<void> cb)
{
    auto bu = Gtk::manage(new Gtk::Button(label));
    bbox->pack_start(*bu, false, false, 0);
    bu->signal_clicked().connect(cb);
    return bu;
}

Gtk::Button *PoolNotebook::add_action_button(const std::string &label, Gtk::Box *bbox, class PoolBrowser *br,
                                             sigc::slot1<void, UUID> cb)
{
    auto bu = Gtk::manage(new Gtk::Button(label));
    bbox->pack_start(*bu, false, false, 0);
    bu->signal_clicked().connect([br, cb] { cb(br->get_selected()); });
    br->signal_selected().connect([bu, br] { bu->set_sensitive(br->get_selected()); });
    bu->set_sensitive(br->get_selected());
    return bu;
}

void PoolNotebook::add_preview_stack_switcher(Gtk::Box *obox, Gtk::Stack *stack)
{
    auto bbox = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL));
    bbox->get_style_context()->add_class("linked");
    auto rb_graphical = Gtk::manage(new Gtk::RadioButton("Preview"));
    rb_graphical->set_mode(false);
    auto rb_text = Gtk::manage(new Gtk::RadioButton("Info"));
    rb_text->set_mode(false);
    rb_text->join_group(*rb_graphical);
    rb_graphical->set_active(true);
    rb_graphical->signal_toggled().connect([this, rb_graphical, stack] {
        if (rb_graphical->get_active()) {
            stack->set_visible_child("preview");
        }
        else {
            stack->set_visible_child("info");
        }
    });
    bbox->pack_start(*rb_graphical, true, true, 0);
    bbox->pack_start(*rb_text, true, true, 0);
    bbox->set_margin_start(4);
    bbox->show_all();
    obox->pack_end(*bbox, false, false, 0);
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

PoolNotebook::PoolNotebook(const std::string &bp, class PoolProjectManagerAppWindow *aw)
    : Gtk::Notebook(), base_path(bp), pool(bp), pool_parametric(bp), appwin(aw)
{
    appwin->pool = &pool;
    appwin->pool_parametric = &pool_parametric;

    appwin->signal_process_saved().connect(sigc::track_obj(
            [this](std::string filename) {
                auto in_pool = Gio::File::create_for_path(base_path)
                                       ->get_relative_path(Gio::File::create_for_path(filename))
                                       .size();
                if (in_pool)
                    pool_update(nullptr, {filename});
                else
                    reload();
            },
            *this));

    {

        pool_update_dispatcher.connect([this] {
            if (in_pool_update_handler)
                return;
            SetReset rst(in_pool_update_handler);
            std::lock_guard<std::mutex> guard(pool_update_status_queue_mutex);
            while (pool_update_status_queue.size()) {
                std::string last_filename;
                std::string last_msg;
                PoolUpdateStatus last_status;

                std::tie(last_status, last_filename, last_msg) = pool_update_status_queue.front();

                appwin->set_pool_update_status_text(last_filename);
                if (last_status == PoolUpdateStatus::DONE) {
                    pool_updated(true);
                    if (pool_update_filenames.size() == 0) // only for full update
                        pool_update_n_files_last = pool_update_n_files;
                }
                else if (last_status == PoolUpdateStatus::FILE) {
                    pool_update_last_file = last_filename;
                    pool_update_n_files++;
                    if (pool_update_n_files_last && pool_update_filenames.size() == 0) { // only for full update
                        appwin->set_pool_update_progress((float)pool_update_n_files / pool_update_n_files_last);
                    }
                    else {
                        appwin->set_pool_update_progress(-1);
                    }
                }
                else if (last_status == PoolUpdateStatus::FILE_ERROR) {
                    pool_update_error_queue.emplace_back(last_status, last_filename, last_msg);
                }
                else if (last_status == PoolUpdateStatus::ERROR) {
                    appwin->set_pool_update_status_text(last_msg + " Last file: " + pool_update_last_file);
                    pool_updated(false);
                }
                pool_update_status_queue.pop_front();
            }
        });
    }
    remote_repo = Glib::build_filename(base_path, ".remote");
    if (!Glib::file_test(remote_repo, Glib::FILE_TEST_IS_DIR)) {
        remote_repo = "";
    }

    construct_units();
    construct_symbols();
    construct_entities();
    construct_padstacks();
    construct_packages();
    construct_parts();
    construct_frames();

    {
        if (PoolManager::get().get_pools().count(pool.get_base_path())) {
            settings_box = PoolSettingsBox::create(this, pool.get_base_path());
            pool_uuid = PoolManager::get().get_pools().at(pool.get_base_path()).uuid;

            settings_box->show();
            append_page(*settings_box, "Settings");
            settings_box->unreference();
        }
    }

    if (remote_repo.size()) {
        remote_box = PoolRemoteBox::create(this);

        remote_box->show();
        append_page(*remote_box, "Remote");
        remote_box->unreference();

        signal_switch_page().connect([this](Gtk::Widget *page, int page_num) {
            if (page == remote_box && !remote_box->prs_refreshed_once) {
                remote_box->handle_refresh_prs();
                remote_box->prs_refreshed_once = true;
            }
        });
    }

    if (Glib::file_test(Glib::build_filename(base_path, ".git"), Glib::FILE_TEST_IS_DIR)) {
        git_box = PoolGitBox::create(this);
        git_box->show();
        append_page(*git_box, "Git");
        git_box->unreference();

        signal_switch_page().connect([this](Gtk::Widget *page, int page_num) {
            if (page == git_box && !git_box->refreshed_once) {
                git_box->refresh();
                git_box->refreshed_once = true;
            }
        });
    }

    for (const auto &it_tab : pool_parametric.get_tables()) {
        auto br = Gtk::manage(new PoolBrowserParametric(&pool, &pool_parametric, it_tab.first));
        br->show();
        add_context_menu(br);
        br->signal_activated().connect([this, br] { go_to(ObjectType::PART, br->get_selected()); });
        auto paned = Gtk::manage(new Gtk::Paned(Gtk::ORIENTATION_HORIZONTAL));
        paned->add1(*br);
        paned->child_property_shrink(*br) = false;

        auto preview = Gtk::manage(new PartPreview(pool));
        preview->signal_goto().connect(sigc::mem_fun(*this, &PoolNotebook::go_to));
        br->signal_selected().connect([this, br, preview] {
            auto sel = br->get_selected();
            if (!sel) {
                preview->load(nullptr);
                return;
            }
            auto part = pool.get_part(sel);
            preview->load(part);
        });
        paned->add2(*preview);
        paned->show_all();

        append_page(*paned, "Param: " + it_tab.second.display_name);
        install_search_once(paned, br);
        browsers_parametric.emplace(it_tab.first, br);
    }


    for (auto br : browsers) {
        add_context_menu(br.second);
    }
}

void PoolNotebook::add_context_menu(PoolBrowser *br)
{
    ObjectType ty = br->get_type();
    br->add_context_menu_item("Delete", [this, ty](const UUID &uu) { handle_delete(ty, uu); });
    br->add_context_menu_item("Copy path", [this, ty](const UUID &uu) { handle_copy_path(ty, uu); });
    br->add_context_menu_item("Copy UUID", [this, ty](const UUID &uu) {
        auto clip = Gtk::Clipboard::get();
        clip->set_text((std::string)uu);
    });
    br->add_copy_name_context_menu_item();
    br->add_context_menu_item("Update this item", [this, ty](const UUID &uu) {
        auto filename = pool.get_filename(ty, uu);
        pool_update(nullptr, {filename});
    });
}

void PoolNotebook::handle_delete(ObjectType ty, const UUID &uu)
{
    auto top = dynamic_cast<Gtk::Window *>(get_ancestor(GTK_TYPE_WINDOW));
    Gtk::MessageDialog md(*top, "Permanently delete " + object_descriptions.at(ty).name + "?", false /* use_markup */,
                          Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_NONE);
    md.add_button("Cancel", Gtk::RESPONSE_CANCEL);
    md.add_button("Delete", Gtk::RESPONSE_OK)->get_style_context()->add_class("destructive-action");
    if (md.run() == Gtk::RESPONSE_OK) {
        auto filename = pool.get_filename(ty, uu);
        if (ty == ObjectType::PACKAGE) {
            auto dir = Glib::path_get_dirname(filename);
            rmdir_recursive(dir);
        }
        else {
            Gio::File::create_for_path(filename)->remove();
        }
        pool_update();
    }
}

void PoolNotebook::handle_copy_path(ObjectType ty, const UUID &uu)
{
    auto filename = pool.get_filename(ty, uu);
    auto clip = Gtk::Clipboard::get();
    clip->set_text(filename);
}

void PoolNotebook::go_to(ObjectType type, const UUID &uu)
{
    browsers.at(type)->go_to(uu);
    static const std::map<ObjectType, int> pages = {
            {ObjectType::UNIT, 0},    {ObjectType::SYMBOL, 1}, {ObjectType::ENTITY, 2}, {ObjectType::PADSTACK, 3},
            {ObjectType::PACKAGE, 4}, {ObjectType::PART, 5},   {ObjectType::FRAME, 6},
    };
    set_current_page(pages.at(type));
}

const UUID &PoolNotebook::get_pool_uuid() const
{
    return pool_uuid;
}

void PoolNotebook::show_duplicate_window(ObjectType ty, const UUID &uu)
{
    if (!uu)
        return;
    if (!duplicate_window) {
        duplicate_window = new DuplicateWindow(&pool, ty, uu);
        duplicate_window->present();
        duplicate_window->signal_hide().connect([this] {
            auto files_duplicated = duplicate_window->get_filenames();
            if (files_duplicated.size()) {
                pool_update(nullptr, files_duplicated);
            }
            delete duplicate_window;
            duplicate_window = nullptr;
        });
    }
    else {
        duplicate_window->present();
    }
}

bool PoolNotebook::get_close_prohibited() const
{
    return part_wizard || pool_updating || duplicate_window || kicad_symbol_import_wizard;
}

void PoolNotebook::prepare_close()
{
    if (remote_box)
        remote_box->prs_refreshed_once = true;
    if (git_box)
        git_box->refreshed_once = true;
    closing = true;
}

bool PoolNotebook::get_needs_save() const
{
    if (settings_box)
        return settings_box->get_needs_save();
    else
        return false;
}

void PoolNotebook::save()
{
    if (settings_box)
        settings_box->save();
}

void PoolNotebook::pool_update_thread()
{
    std::cout << "hello from thread" << std::endl;

    try {
        horizon::pool_update(
                pool.get_base_path(),
                [this](PoolUpdateStatus st, std::string filename, std::string msg) {
                    {
                        std::lock_guard<std::mutex> guard(pool_update_status_queue_mutex);
                        pool_update_status_queue.emplace_back(st, filename, msg);
                    }
                    pool_update_dispatcher.emit();
                },
                true, pool_update_filenames);
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

void PoolNotebook::pool_update(std::function<void()> cb, const std::vector<std::string> &filenames)
{
    if (closing)
        return;

    if (pool_updating)
        return;

    appwin->set_pool_updating(true, true);
    pool_update_n_files = 0;
    pool_updating = true;
    pool_update_done_cb = cb;
    pool_update_filenames = filenames;
    pool_update_status_queue.clear();
    pool_update_error_queue.clear();
    std::thread thr(&PoolNotebook::pool_update_thread, this);
    thr.detach();
}

void PoolNotebook::install_search_once(Gtk::Widget *widget, PoolBrowser *browser)
{
    signal_switch_page().connect(sigc::track_obj(
            [this, widget, browser](Gtk::Widget *page, guint page_num) {
                if (in_destruction())
                    return;
                if (page == widget)
                    browser->search_once();
            },
            *widget, *browser));
}

} // namespace horizon
