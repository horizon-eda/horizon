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
#include "pool_git_box.hpp"
#include "pool-prj-mgr/pool-prj-mgr-app.hpp"
#include "pool_update_error_dialog.hpp"
#include "pool_settings_box.hpp"
#include "pool_cache_box.hpp"
#include "pool/pool_manager.hpp"
#include "part_wizard/part_wizard.hpp"
#include "widgets/part_preview.hpp"
#include <thread>
#include "nlohmann/json.hpp"
#include "widgets/where_used_box.hpp"
#include "util/win32_undef.hpp"
#include "util/fs_util.hpp"
#include "duplicate/duplicate_unit.hpp"
#include "editors/editor_window.hpp"
#include "move_window.hpp"

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
    if (cache_box && cache_box->refreshed_once) {
        if (widget_is_visible(cache_box))
            cache_box->refresh_status();
        else
            cache_box->refreshed_once = false;
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

using OT = ObjectType;
using PT = PoolProjectManagerProcess::Type;
static const std::map<OT, PT> editor_type_map = {
        {OT::UNIT, PT::UNIT},         {OT::ENTITY, PT::ENTITY},         {OT::PART, PT::PART},
        {OT::SYMBOL, PT::IMP_SYMBOL}, {OT::PADSTACK, PT::IMP_PADSTACK}, {OT::PACKAGE, PT::IMP_PACKAGE},
        {OT::FRAME, PT::IMP_FRAME},   {OT::DECAL, PT::IMP_DECAL},
};

void PoolNotebook::handle_edit_item(ObjectType ty, const UUID &uu)
{
    if (!uu)
        return;

    UUID item_pool_uuid;
    const auto path = pool.get_filename(ty, uu, &item_pool_uuid);
    const auto spawn_read_only = pool_uuid && (item_pool_uuid != pool_uuid);
    using F = PoolProjectManagerAppWindow::SpawnFlags;
    appwin->spawn(editor_type_map.at(ty), {path}, spawn_read_only ? F::READ_ONLY : F::NONE);
}

void PoolNotebook::handle_duplicate_item(ObjectType ty, const UUID &uu)
{
    if (!uu)
        return;
    auto top = dynamic_cast<Gtk::Window *>(get_ancestor(GTK_TYPE_WINDOW));

    GtkFileChooserNative *native =
            gtk_file_chooser_native_new(("Save " + object_descriptions.at(ty).name).c_str(), top->gobj(),
                                        GTK_FILE_CHOOSER_ACTION_SAVE, "_Save", "_Cancel");
    auto chooser = Glib::wrap(GTK_FILE_CHOOSER(native));
    chooser->set_do_overwrite_confirmation(true);
    auto it_filename = Glib::build_filename(pool.get_base_path(), pool.get_rel_filename(ty, uu));
    auto it_basename = Glib::path_get_basename(it_filename);
    auto it_dirname = Glib::path_get_dirname(it_filename);
    chooser->set_current_folder(it_dirname);
    chooser->set_current_name(DuplicateUnitWidget::insert_filename(it_basename, "-copy"));

    if (gtk_native_dialog_run(GTK_NATIVE_DIALOG(native)) == GTK_RESPONSE_ACCEPT) {
        std::string fn = EditorWindow::fix_filename(chooser->get_filename());
        switch (ty) {
        case ObjectType::DECAL: {
            Decal dec(*pool.get_decal(uu));
            dec.name += " (Copy)";
            dec.uuid = UUID::random();
            save_json_to_file(fn, dec.serialize());
        } break;

        case ObjectType::FRAME: {
            Frame fr(*pool.get_frame(uu));
            fr.name += " (Copy)";
            fr.uuid = UUID::random();
            save_json_to_file(fn, fr.serialize());
        } break;

        case ObjectType::PADSTACK: {
            Padstack ps(*pool.get_padstack(uu));
            ps.name += " (Copy)";
            ps.uuid = UUID::random();
            save_json_to_file(fn, ps.serialize());
        } break;

        default:
            throw std::runtime_error("Can't duplicate " + object_descriptions.at(ty).name);
        }
        pool_update(nullptr, {fn});
        appwin->spawn(editor_type_map.at(ty), {fn});
    }
}

Gtk::Button *PoolNotebook::add_merge_button(Gtk::Box *bbox, class PoolBrowser *br, std::function<void(UUID)> cb)
{
    if (!remote_repo.size())
        return nullptr;

    auto bu = Gtk::manage(new Gtk::Button("Merge"));
    bbox->pack_start(*bu, false, false, 0);
    if (cb)
        bu->signal_clicked().connect([br, cb] { cb(br->get_selected()); });
    else
        bu->signal_clicked().connect([this, br] { remote_box->merge_item(br->get_type(), br->get_selected()); });
    br->signal_selected().connect([this, bu, br] {
        const auto uu = br->get_selected();
        if (!uu) {
            bu->set_sensitive(false);
            return;
        }
        const auto from_this_pool = get_pool_uuids(br->get_type(), uu).pool == pool.get_pool_info().uuid;
        bu->set_sensitive(from_this_pool);
    });
    bu->set_sensitive(false);
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
    rb_graphical->signal_toggled().connect([rb_graphical, stack] {
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

    pool_item_edited_conn = appwin->get_app().signal_pool_items_edited().connect(
            sigc::track_obj([this](const auto &filenames) { pool_update(nullptr, filenames); }, *this));

    {
        pool_update_dispatcher.connect([this] {
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
                        appwin->set_pool_update_status_text(last_msg + " Last file: " + pool_update_last_file);
                        pool_updated(false);
                        return;
                    }
                }
                if (pool_update_last_info != last_info)
                    appwin->set_pool_update_status_text(pool_update_last_info);
                if (pool_update_n_files_last && pool_update_filenames.size() == 0) { // only for full update
                    appwin->set_pool_update_progress((float)pool_update_n_files / pool_update_n_files_last);
                }
                else {
                    appwin->set_pool_update_progress(-1);
                }
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
    construct_decals();

    {
        settings_box = PoolSettingsBox::create(pool);
        settings_box->signal_open_pool().connect(
                [this](auto pool_bp) { appwin->get_app().open_pool(Glib::build_filename(pool_bp, "pool.json")); });
        pool_uuid = pool.get_pool_info().uuid;

        settings_box->show();
        append_page(*settings_box, "Settings");
        settings_box->unreference();
    }


    if (pool.get_pool_info().is_project_pool()) {
        cache_box = PoolCacheBox::create(&appwin->get_app(), this, pool);
        cache_box->signal_goto().connect(sigc::mem_fun(*this, &PoolNotebook::go_to));
        cache_box->show();
        append_page(*cache_box, "Cache");
        cache_box->unreference();

        signal_switch_page().connect([this](Gtk::Widget *page, int page_num) {
            if (in_destruction())
                return;
            if (page == cache_box && !cache_box->refreshed_once) {
                cache_box->refresh_status();
                cache_box->refreshed_once = true;
            }
        });
    }

    if (remote_repo.size()) {
        remote_box = PoolRemoteBox::create(this);

        remote_box->show();
        append_page(*remote_box, "Remote");
        remote_box->unreference();

        signal_switch_page().connect([this](Gtk::Widget *page, int page_num) {
            if (in_destruction())
                return;
            if (page == remote_box) {
                remote_box->login_once();
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

    construct_parametric();

    for (auto br : browsers) {
        add_context_menu(br.second);
    }
}

void PoolNotebook::create_paned_state_store(Gtk::Paned *paned, const std::string &prefix)
{
    paned_state_stores.emplace_back(std::make_unique<PanedStateStore>(paned, "pool_notebook_" + prefix));
}

void PoolNotebook::add_context_menu(PoolBrowser *br)
{
    const auto ty = br->get_type();
    auto this_pool_lambda = [this, ty](const UUID &uu) {
        return get_pool_uuids(ty, uu).pool == pool.get_pool_info().uuid;
    };

    br->add_context_menu_item(
            "Delete", [this, ty](const UUID &uu) { handle_delete(ty, uu); }, this_pool_lambda);
    br->add_context_menu_item(
            "Move/rename", [this, ty](const UUID &uu) { handle_move_rename(ty, uu); }, this_pool_lambda);
    br->add_context_menu_item("Copy path", [this, ty](const UUID &uu) { handle_copy_path(ty, uu); });
    br->add_context_menu_item("Copy UUID", [](const UUID &uu) {
        auto clip = Gtk::Clipboard::get();
        clip->set_text((std::string)uu);
    });
    br->add_copy_name_context_menu_item();
    br->add_context_menu_item("Update this item", [this, ty](const UUID &uu) {
        auto filename = pool.get_filename(ty, uu);
        pool_update(nullptr, {filename});
    });
    br->add_context_menu_item(
            "Open in included pool",
            [this, ty](const UUID &uu) {
                auto x = get_pool_uuids(ty, uu);
                if (auto pool2 = PoolManager::get().get_by_uuid(x.last ? x.last : x.pool)) {
                    appwin->get_app().open_pool(Glib::build_filename(pool2->base_path, "pool.json"), ty, uu);
                }
            },
            [this, ty](const UUID &uu) -> bool {
                auto x = get_pool_uuids(ty, uu);
                return x.last || x.pool != pool.get_pool_info().uuid;
            });
    br->add_context_menu_item(
            "Move to other pool",
            [this](const UUID &uu) {
                auto win = MoveWindow::create(pool, ObjectType::PART, uu);
                auto top = dynamic_cast<Gtk::Window *>(get_ancestor(GTK_TYPE_WINDOW));
                win->set_transient_for(*top);
                win->present();
                win->signal_hide().connect([this, win] {
                    if (win->get_moved()) {
                        this->pool_update();
                    }
                    delete win;
                });
            },
            this_pool_lambda);
}

PoolNotebook::ItemPoolInfo PoolNotebook::get_pool_uuids(ObjectType ty, const UUID &uu)
{
    SQLite::Query q(pool.get_db(), "SELECT pool_uuid, last_pool_uuid FROM all_items_view WHERE type = ? AND uuid = ?");
    q.bind(1, ty);
    q.bind(2, uu);
    if (q.step()) {
        const UUID pool_uu(q.get<std::string>(0));
        const UUID last_pool_uu(q.get<std::string>(1));
        return {pool_uu, last_pool_uu};
    }
    throw std::runtime_error("item not found");
}

void PoolNotebook::handle_delete(ObjectType ty, const UUID &uu)
{
    auto top = dynamic_cast<Gtk::Window *>(get_ancestor(GTK_TYPE_WINDOW));

    std::string item_name;
    {
        SQLite::Query q(pool.db, "SELECT name FROM all_items_view WHERE type = ? and uuid = ?");
        q.bind(1, ty);
        q.bind(2, uu);
        if (q.step()) {
            item_name = q.get<std::string>(0);
        }
    }

    Gtk::MessageDialog md(*top, "Permanently delete " + object_descriptions.at(ty).name + " \"" + item_name + "\" ?",
                          false /* use_markup */, Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_NONE);
    auto box = Gtk::manage(new WhereUsedBox(pool));
    const auto cnt = box->load(ty, uu);
    ItemSet items_del = {{ty, uu}};
    if (cnt) {
        box->property_margin() = 10;
        md.get_content_area()->pack_start(*box, true, true, 0);
        md.get_content_area()->show_all();
        md.get_content_area()->set_spacing(0);
    }
    else {
        delete box;
    }


    static constexpr int RESP_DEL_THIS = 1;
    static constexpr int RESP_DEL_ALL = 2;

    md.add_button("Cancel", Gtk::RESPONSE_CANCEL);
    if (cnt == 0) {
        md.add_button("Delete", RESP_DEL_THIS)->get_style_context()->add_class("destructive-action");
    }
    else {
        md.add_button("Delete and break dependents", RESP_DEL_THIS)
                ->get_style_context()
                ->add_class("destructive-action");
        md.add_button("Delete dependents as well", RESP_DEL_ALL)->get_style_context()->add_class("destructive-action");
    }

    const auto resp = md.run();
    if (any_of(resp, {RESP_DEL_THIS, RESP_DEL_ALL})) {
        if (resp == RESP_DEL_ALL) {
            auto deps = box->get_items();
            items_del.insert(deps.begin(), deps.end());
        }
        for (const auto &[it_ty, it_uu] : items_del) {
            auto filename = pool.get_filename(it_ty, it_uu);
            if (it_ty == ObjectType::PACKAGE) {
                auto dir = Glib::path_get_dirname(filename);
                rmdir_recursive(dir);
            }
            else {
                Gio::File::create_for_path(filename)->remove();
            }
        }
        pool_update();
    }
}


void PoolNotebook::handle_move_rename(ObjectType ty, const UUID &uu)
{
    auto filename = pool.get_filename(ty, uu);
    auto top = dynamic_cast<Gtk::Window *>(get_ancestor(GTK_TYPE_WINDOW));

    GtkFileChooserNative *native =
            gtk_file_chooser_native_new("Move item", top->gobj(), GTK_FILE_CHOOSER_ACTION_SAVE, "_Save", "_Cancel");
    auto chooser = Glib::wrap(GTK_FILE_CHOOSER(native));
    chooser->set_current_name(Glib::path_get_basename(filename));
    while (1) {
        chooser->set_current_folder(Glib::path_get_dirname(filename));
        auto resp = gtk_native_dialog_run(GTK_NATIVE_DIALOG(native));
        if (resp == GTK_RESPONSE_ACCEPT) {
            const std::string new_filename = chooser->get_filename();
            try {
                std::string e;
                if (!pool.check_filename(ty, new_filename, &e)) {
                    throw std::runtime_error(e);
                }
                Gio::File::create_for_path(filename)->move(Gio::File::create_for_path(new_filename));
                pool_update();
                return;
            }
            catch (const std::exception &e) {
                Gtk::MessageDialog md(*top, "Error moving item", false /* use_markup */, Gtk::MESSAGE_ERROR,
                                      Gtk::BUTTONS_OK);
                md.set_secondary_text(e.what());
                md.run();
            }
            catch (const Gio::Error &e) {
                Gtk::MessageDialog md(*top, "Error moving item", false /* use_markup */, Gtk::MESSAGE_ERROR,
                                      Gtk::BUTTONS_OK);
                md.set_secondary_text(e.what());
                md.run();
            }
        }
        else {
            return;
        }
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
    auto br = browsers.at(type);
    br->go_to(uu);
    Gtk::Widget *last_ancestor = nullptr;
    Gtk::Widget *ancestor = br;
    while (ancestor != this) {
        last_ancestor = ancestor;
        ancestor = ancestor->get_parent();
    }
    auto page = page_num(*last_ancestor);
    assert(page != -1);
    set_current_page(page);
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
        duplicate_window = new DuplicateWindow(pool, ty, uu);
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
    return part_wizard || pool_updating || duplicate_window || kicad_symbol_import_wizard
           || import_kicad_package_window;
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
        clock_t begin = clock();
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

void PoolNotebook::pool_update(std::function<void()> cb, const std::vector<std::string> &filenames)
{
    if (closing)
        return;

    if (pool_updating)
        return;

    if (filenames.size()) {
        pool_item_edited_conn.block();
        appwin->get_app().signal_pool_items_edited().emit(filenames);
        pool_item_edited_conn.unblock();
    }
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
