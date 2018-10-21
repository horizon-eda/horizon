#include "canvas/canvas.hpp"
#include "pool_notebook.hpp"
#include "widgets/pool_browser_unit.hpp"
#include "widgets/pool_browser_symbol.hpp"
#include "widgets/pool_browser_entity.hpp"
#include "widgets/pool_browser_padstack.hpp"
#include "widgets/pool_browser_package.hpp"
#include "widgets/pool_browser_part.hpp"
#include "widgets/pool_browser_frame.hpp"
#include "dialogs/pool_browser_dialog.hpp"
#include "util/util.hpp"
#include "pool-update/pool-update.hpp"
#include "editors/editor_window.hpp"
#include "pool-prj-mgr/pool-prj-mgr-app_win.hpp"
#include "duplicate/duplicate_window.hpp"
#include "duplicate/duplicate_unit.hpp"
#include "duplicate/duplicate_part.hpp"
#include "common/object_descr.hpp"
#include "pool_remote_box.hpp"
#include "pool_merge_dialog.hpp"
#include <git2.h>
#include "util/autofree_ptr.hpp"
#include "pool-prj-mgr/pool-prj-mgr-app.hpp"
#include "widgets/part_preview.hpp"
#include "widgets/entity_preview.hpp"
#include "widgets/unit_preview.hpp"
#include "widgets/symbol_preview.hpp"
#include "widgets/preview_canvas.hpp"
#include "pool_update_error_dialog.hpp"
#include "pool_settings_box.hpp"
#include "pool/pool_manager.hpp"
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
    pool.clear();
    for (auto &br : browsers) {
        br.second->search();
    }
    auto procs = appwin->get_processes();
    for (auto &it : procs) {
        it.second->reload();
    }
    if (success && pool_update_done_cb) {
        pool_update_done_cb();
        pool_update_done_cb = nullptr;
    }
}

PoolNotebook::~PoolNotebook()
{
    appwin->pool = nullptr;
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
    : Gtk::Notebook(), base_path(bp), pool(bp), appwin(aw)
{
    appwin->pool = &pool;
    {
        int user_version = pool.db.get_user_version();
        int required_version = pool.get_required_schema_version();
        if (user_version < required_version) {
            Gtk::MessageDialog md("Schema update required", false /* use_markup */, Gtk::MESSAGE_ERROR,
                                  Gtk::BUTTONS_OK);
            md.run();
            horizon::pool_update(base_path);
        }
    }
    appwin->signal_process_exited().connect([this](std::string filename, int status, bool modified) {
        if (modified)
            pool_update();
    });

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
                    pool_update_n_files_last = pool_update_n_files;
                }
                else if (last_status == PoolUpdateStatus::FILE) {
                    pool_update_last_file = last_filename;
                    pool_update_n_files++;
                    if (pool_update_n_files_last) {
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

    {
        auto br = Gtk::manage(new PoolBrowserUnit(&pool));
        br->set_show_path(true);
        br->signal_activated().connect([this, br] { handle_edit_unit(br->get_selected()); });

        br->show();
        browsers.emplace(ObjectType::UNIT, br);

        auto box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL, 0));
        auto bbox = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 8));
        bbox->set_margin_bottom(8);
        bbox->set_margin_top(8);
        bbox->set_margin_start(8);
        bbox->set_margin_end(8);

        add_action_button("Create", bbox, sigc::mem_fun(this, &PoolNotebook::handle_create_unit));
        add_action_button("Edit", bbox, br, sigc::mem_fun(this, &PoolNotebook::handle_edit_unit));
        add_action_button("Duplicate", bbox, br, sigc::mem_fun(this, &PoolNotebook::handle_duplicate_unit));
        add_action_button("Create Symbol", bbox, br, sigc::mem_fun(this, &PoolNotebook::handle_create_symbol_for_unit));
        add_action_button("Create Entity", bbox, br, sigc::mem_fun(this, &PoolNotebook::handle_create_entity_for_unit));
        if (remote_repo.size())
            add_action_button("Merge", bbox, br,
                              [this](const UUID &uu) { remote_box->merge_item(ObjectType::UNIT, uu); });

        bbox->show_all();

        box->pack_start(*bbox, false, false, 0);

        auto sep = Gtk::manage(new Gtk::Separator(Gtk::ORIENTATION_HORIZONTAL));
        sep->show();
        box->pack_start(*sep, false, false, 0);
        box->pack_start(*br, true, true, 0);

        auto paned = Gtk::manage(new Gtk::Paned(Gtk::ORIENTATION_HORIZONTAL));
        paned->add1(*box);
        paned->child_property_shrink(*box) = false;

        auto preview = Gtk::manage(new UnitPreview(pool));
        preview->signal_goto().connect(sigc::mem_fun(this, &PoolNotebook::go_to));
        br->signal_selected().connect([this, br, preview] {
            auto sel = br->get_selected();
            if (!sel) {
                preview->load(nullptr);
                return;
            }
            auto unit = pool.get_unit(sel);
            preview->load(unit);
        });
        paned->add2(*preview);
        paned->show_all();

        append_page(*paned, "Units");
    }
    {
        auto br = Gtk::manage(new PoolBrowserSymbol(&pool));
        br->set_show_path(true);
        browsers.emplace(ObjectType::SYMBOL, br);
        br->show();
        auto paned = Gtk::manage(new Gtk::Paned(Gtk::ORIENTATION_HORIZONTAL));

        auto box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL, 0));
        auto bbox = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 8));
        bbox->set_margin_bottom(8);
        bbox->set_margin_top(8);
        bbox->set_margin_start(8);
        bbox->set_margin_end(8);

        add_action_button("Create", bbox, sigc::mem_fun(this, &PoolNotebook::handle_create_symbol));
        add_action_button("Edit", bbox, br, sigc::mem_fun(this, &PoolNotebook::handle_edit_symbol));
        add_action_button("Duplicate", bbox, br, sigc::mem_fun(this, &PoolNotebook::handle_duplicate_symbol));
        if (remote_repo.size())
            add_action_button("Merge", bbox, br,
                              [this](const UUID &uu) { remote_box->merge_item(ObjectType::SYMBOL, uu); });
        bbox->show_all();

        box->pack_start(*bbox, false, false, 0);

        auto sep = Gtk::manage(new Gtk::Separator(Gtk::ORIENTATION_HORIZONTAL));
        sep->show();
        box->pack_start(*sep, false, false, 0);
        box->pack_start(*br, true, true, 0);
        box->show();

        paned->add1(*box);
        paned->child_property_shrink(*box) = false;


        auto preview = Gtk::manage(new SymbolPreview(pool));
        br->signal_selected().connect([this, br, preview] {
            auto sel = br->get_selected();
            preview->load(sel);
        });
        paned->add2(*preview);
        paned->show_all();

        br->signal_activated().connect([this, br] { handle_edit_symbol(br->get_selected()); });

        append_page(*paned, "Symbols");
    }


    {
        auto br = Gtk::manage(new PoolBrowserEntity(&pool));
        br->set_show_path(true);
        br->signal_activated().connect([this, br] { handle_edit_entity(br->get_selected()); });

        br->show();
        browsers.emplace(ObjectType::ENTITY, br);

        auto box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL, 0));
        auto bbox = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 8));
        bbox->set_margin_bottom(8);
        bbox->set_margin_top(8);
        bbox->set_margin_start(8);
        bbox->set_margin_end(8);

        add_action_button("Create", bbox, sigc::mem_fun(this, &PoolNotebook::handle_create_entity));
        add_action_button("Edit", bbox, br, sigc::mem_fun(this, &PoolNotebook::handle_edit_entity));
        add_action_button("Duplicate", bbox, br, sigc::mem_fun(this, &PoolNotebook::handle_duplicate_entity));
        if (remote_repo.size())
            add_action_button("Merge", bbox, br,
                              [this](const UUID &uu) { remote_box->merge_item(ObjectType::ENTITY, uu); });

        bbox->show_all();

        box->pack_start(*bbox, false, false, 0);

        auto sep = Gtk::manage(new Gtk::Separator(Gtk::ORIENTATION_HORIZONTAL));
        sep->show();
        box->pack_start(*sep, false, false, 0);
        box->pack_start(*br, true, true, 0);

        auto paned = Gtk::manage(new Gtk::Paned(Gtk::ORIENTATION_HORIZONTAL));
        paned->add1(*box);
        paned->child_property_shrink(*box) = false;

        auto preview = Gtk::manage(new EntityPreview(pool));
        preview->signal_goto().connect(sigc::mem_fun(this, &PoolNotebook::go_to));
        br->signal_selected().connect([this, br, preview] {
            auto sel = br->get_selected();
            if (!sel) {
                preview->load(nullptr);
                return;
            }
            auto part = pool.get_entity(sel);
            preview->load(part);
        });

        paned->add2(*preview);
        paned->show_all();

        paned->show_all();

        append_page(*paned, "Entities");
    }

    {
        auto br = Gtk::manage(new PoolBrowserPadstack(&pool));
        br->set_show_path(true);
        br->set_include_padstack_type(Padstack::Type::VIA, true);
        br->set_include_padstack_type(Padstack::Type::MECHANICAL, true);
        br->set_include_padstack_type(Padstack::Type::HOLE, true);
        br->signal_activated().connect([this, br] { handle_edit_padstack(br->get_selected()); });

        br->show();
        browsers.emplace(ObjectType::PADSTACK, br);

        auto paned = Gtk::manage(new Gtk::Paned(Gtk::ORIENTATION_HORIZONTAL));

        auto box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL, 0));
        auto bbox = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 8));
        bbox->set_margin_bottom(8);
        bbox->set_margin_top(8);
        bbox->set_margin_start(8);
        bbox->set_margin_end(8);

        add_action_button("Create", bbox, sigc::mem_fun(this, &PoolNotebook::handle_create_padstack));
        add_action_button("Edit", bbox, br, sigc::mem_fun(this, &PoolNotebook::handle_edit_padstack));
        add_action_button("Duplicate", bbox, br, sigc::mem_fun(this, &PoolNotebook::handle_duplicate_padstack));
        if (remote_repo.size())
            add_action_button("Merge", bbox, br,
                              [this](const UUID &uu) { remote_box->merge_item(ObjectType::PADSTACK, uu); });

        bbox->show_all();

        box->pack_start(*bbox, false, false, 0);

        auto sep = Gtk::manage(new Gtk::Separator(Gtk::ORIENTATION_HORIZONTAL));
        sep->show();
        box->pack_start(*sep, false, false, 0);
        box->pack_start(*br, true, true, 0);
        box->show();

        paned->add1(*box);
        paned->child_property_shrink(*box) = false;


        auto canvas = Gtk::manage(new PreviewCanvas(pool, true));
        paned->add2(*canvas);
        paned->show_all();

        br->signal_selected().connect([this, br, canvas] {
            auto sel = br->get_selected();
            if (!sel) {
                canvas->clear();
            }
            else {
                canvas->load(ObjectType::PADSTACK, sel);
            }
        });

        append_page(*paned, "Padstacks");
    }

    {
        auto br = Gtk::manage(new PoolBrowserPackage(&pool));
        br->set_show_path(true);
        br->signal_activated().connect([this, br] { handle_edit_package(br->get_selected()); });

        br->show();
        browsers.emplace(ObjectType::PACKAGE, br);

        auto paned = Gtk::manage(new Gtk::Paned(Gtk::ORIENTATION_HORIZONTAL));


        auto box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL, 0));
        auto bbox = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 8));
        bbox->set_margin_bottom(8);
        bbox->set_margin_top(8);
        bbox->set_margin_start(8);
        bbox->set_margin_end(8);

        add_action_button("Create", bbox, sigc::mem_fun(this, &PoolNotebook::handle_create_package));
        add_action_button("Edit", bbox, br, sigc::mem_fun(this, &PoolNotebook::handle_edit_package));
        add_action_button("Duplicate", bbox, br, sigc::mem_fun(this, &PoolNotebook::handle_duplicate_package));
        add_action_button("Create Padstack", bbox, br,
                          sigc::mem_fun(this, &PoolNotebook::handle_create_padstack_for_package));
        if (remote_repo.size()) {
            add_action_button("Merge", bbox, br,
                              [this](const UUID &uu) { remote_box->merge_item(ObjectType::PACKAGE, uu); });
            add_action_button("Merge 3D", bbox, br, [this](const UUID &uu) {
                auto pkg = pool.get_package(uu);
                for (const auto &it : pkg->models) {
                    remote_box->merge_3d_model(it.second.filename);
                }
            });
        }
        add_action_button("Part Wizard...", bbox, br, sigc::mem_fun(this, &PoolNotebook::handle_part_wizard))
                ->get_style_context()
                ->add_class("suggested-action");

        bbox->show_all();

        box->pack_start(*bbox, false, false, 0);

        auto sep = Gtk::manage(new Gtk::Separator(Gtk::ORIENTATION_HORIZONTAL));
        sep->show();
        box->pack_start(*sep, false, false, 0);
        box->pack_start(*br, true, true, 0);
        box->show();

        paned->add1(*box);
        paned->child_property_shrink(*box) = false;

        auto canvas = Gtk::manage(new PreviewCanvas(pool, true));
        paned->add2(*canvas);
        paned->show_all();

        br->signal_selected().connect([this, br, canvas] {
            auto sel = br->get_selected();
            if (!sel) {
                canvas->clear();
                return;
            }
            canvas->load(ObjectType::PACKAGE, sel);
        });
        append_page(*paned, "Packages");
    }

    {
        auto br = Gtk::manage(new PoolBrowserPart(&pool));
        br->set_show_path(true);
        br->signal_activated().connect([this, br] { handle_edit_part(br->get_selected()); });

        br->show();
        browsers.emplace(ObjectType::PART, br);

        auto paned = Gtk::manage(new Gtk::Paned(Gtk::ORIENTATION_HORIZONTAL));

        auto box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL, 0));
        auto bbox = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 8));
        bbox->set_margin_bottom(8);
        bbox->set_margin_top(8);
        bbox->set_margin_start(8);
        bbox->set_margin_end(8);

        add_action_button("Create", bbox, sigc::mem_fun(this, &PoolNotebook::handle_create_part));
        add_action_button("Edit", bbox, br, sigc::mem_fun(this, &PoolNotebook::handle_edit_part));
        add_action_button("Duplicate", bbox, br, sigc::mem_fun(this, &PoolNotebook::handle_duplicate_part));
        add_action_button("Create Part from Part", bbox, br,
                          sigc::mem_fun(this, &PoolNotebook::handle_create_part_from_part));
        if (remote_repo.size())
            add_action_button("Merge", bbox, br,
                              [this](const UUID &uu) { remote_box->merge_item(ObjectType::PART, uu); });


        bbox->show_all();

        box->pack_start(*bbox, false, false, 0);

        auto sep = Gtk::manage(new Gtk::Separator(Gtk::ORIENTATION_HORIZONTAL));
        sep->show();
        box->pack_start(*sep, false, false, 0);
        box->pack_start(*br, true, true, 0);
        box->show();

        paned->add1(*box);
        paned->child_property_shrink(*box) = false;

        auto preview = Gtk::manage(new PartPreview(pool));
        preview->signal_goto().connect(sigc::mem_fun(this, &PoolNotebook::go_to));
        paned->add2(*preview);
        paned->show_all();

        br->signal_selected().connect([this, br, preview] {
            auto sel = br->get_selected();
            if (!sel) {
                preview->load(nullptr);
                return;
            }
            auto part = pool.get_part(sel);
            preview->load(part);
        });

        append_page(*paned, "Parts");
    }


    {
        auto br = Gtk::manage(new PoolBrowserFrame(&pool));
        br->set_show_path(true);
        browsers.emplace(ObjectType::FRAME, br);
        br->show();
        auto paned = Gtk::manage(new Gtk::Paned(Gtk::ORIENTATION_HORIZONTAL));

        auto box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL, 0));
        auto bbox = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 8));
        bbox->set_margin_bottom(8);
        bbox->set_margin_top(8);
        bbox->set_margin_start(8);
        bbox->set_margin_end(8);

        add_action_button("Create", bbox, sigc::mem_fun(this, &PoolNotebook::handle_create_frame));
        add_action_button("Edit", bbox, br, sigc::mem_fun(this, &PoolNotebook::handle_edit_frame));
        add_action_button("Duplicate", bbox, br, sigc::mem_fun(this, &PoolNotebook::handle_duplicate_frame));
        /*if (remote_repo.size())
            add_action_button("Merge", bbox, br,
                              [this](const UUID &uu) { remote_box->merge_item(ObjectType::SYMBOL, uu); });*/
        bbox->show_all();

        box->pack_start(*bbox, false, false, 0);

        auto sep = Gtk::manage(new Gtk::Separator(Gtk::ORIENTATION_HORIZONTAL));
        sep->show();
        box->pack_start(*sep, false, false, 0);
        box->pack_start(*br, true, true, 0);
        box->show();

        paned->add1(*box);
        paned->child_property_shrink(*box) = false;


        auto canvas = Gtk::manage(new PreviewCanvas(pool, false));
        paned->add2(*canvas);
        paned->show_all();

        br->signal_selected().connect([this, br, canvas] {
            auto sel = br->get_selected();
            if (!sel) {
                canvas->clear();
            }
            else {
                canvas->load(ObjectType::FRAME, sel);
            }
        });

        br->signal_activated().connect([this, br] { handle_edit_frame(br->get_selected()); });

        append_page(*paned, "Frames");
    }

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

    for (auto br : browsers) {
        add_context_menu(br.second);
    }
}

void PoolNotebook::add_context_menu(PoolBrowser *br)
{
    ObjectType ty = br->get_type();
    br->add_context_menu_item("Delete", [this, ty](const UUID &uu) { handle_delete(ty, uu); });
    br->add_context_menu_item("Copy path", [this, ty](const UUID &uu) { handle_copy_path(ty, uu); });
}

void rmdir_recursive(const std::string &dirname)
{
    Glib::Dir dir(dirname);
    std::list<std::string> entries(dir.begin(), dir.end());
    for (const auto &it : entries) {
        auto filename = Glib::build_filename(dirname, it);
        if (Glib::file_test(filename, Glib::FILE_TEST_IS_DIR)) {
            rmdir_recursive(filename);
        }
        else {
            Gio::File::create_for_path(filename)->remove();
        }
    }
    Gio::File::create_for_path(dirname)->remove();
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
            {ObjectType::UNIT, 0},     {ObjectType::SYMBOL, 1},  {ObjectType::ENTITY, 2},
            {ObjectType::PADSTACK, 3}, {ObjectType::PACKAGE, 4}, {ObjectType::PART, 5},
    };
    set_current_page(pages.at(type));
}

void PoolNotebook::show_duplicate_window(ObjectType ty, const UUID &uu)
{
    if (!uu)
        return;
    if (!duplicate_window) {
        duplicate_window = new DuplicateWindow(&pool, ty, uu);
        duplicate_window->present();
        duplicate_window->signal_hide().connect([this] {
            if (duplicate_window->get_duplicated()) {
                pool_update();
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
    return part_wizard || pool_updating || duplicate_window;
}

void PoolNotebook::prepare_close()
{
    if (remote_box)
        remote_box->prs_refreshed_once = true;
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
        horizon::pool_update(pool.get_base_path(), [this](PoolUpdateStatus st, std::string filename, std::string msg) {
            {
                std::lock_guard<std::mutex> guard(pool_update_status_queue_mutex);
                pool_update_status_queue.emplace_back(st, filename, msg);
            }
            pool_update_dispatcher.emit();
        });
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

void PoolNotebook::pool_update(std::function<void()> cb)
{
    if (closing)
        return;

    if (pool_updating)
        return;

    appwin->set_pool_updating(true, true);
    pool_update_n_files = 0;
    pool_updating = true;
    pool_update_done_cb = cb;
    pool_update_status_queue.clear();
    pool_update_error_queue.clear();
    std::thread thr(&PoolNotebook::pool_update_thread, this);
    thr.detach();
}
} // namespace horizon
