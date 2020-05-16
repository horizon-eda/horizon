#include "pool_notebook.hpp"
#include "editors/editor_window.hpp"
#include "duplicate/duplicate_unit.hpp"
#include "util/util.hpp"
#include "pool-prj-mgr/pool-prj-mgr-app_win.hpp"
#include "widgets/pool_browser_entity.hpp"
#include "widgets/entity_preview.hpp"
#include "widgets/entity_info_box.hpp"
#include "pool_remote_box.hpp"
#include "pool-prj-mgr/pool-mgr/kicad_symbol_import_wizard/kicad_symbol_import_wizard.hpp"

namespace horizon {
void PoolNotebook::handle_edit_entity(const UUID &uu)
{
    if (!uu)
        return;
    UUID item_pool_uuid;
    auto path = pool.get_filename(ObjectType::ENTITY, uu, &item_pool_uuid);
    appwin->spawn(PoolProjectManagerProcess::Type::ENTITY, {path}, {}, pool_uuid && (item_pool_uuid != pool_uuid));
}

void PoolNotebook::handle_create_entity()
{
    appwin->spawn(PoolProjectManagerProcess::Type::ENTITY, {""});
}

void PoolNotebook::handle_duplicate_entity(const UUID &uu)
{
    if (!uu)
        return;
    show_duplicate_window(ObjectType::ENTITY, uu);
}

void PoolNotebook::handle_kicad_symbol_import_wizard()
{
    if (!kicad_symbol_import_wizard) {
        auto package_uuid = browsers.at(ObjectType::PACKAGE)->get_selected();
        GtkFileChooserNative *native = gtk_file_chooser_native_new("Import KiCad library", GTK_WINDOW(appwin->gobj()),
                                                                   GTK_FILE_CHOOSER_ACTION_OPEN, "_Open", "_Cancel");
        auto chooser = Glib::wrap(GTK_FILE_CHOOSER(native));
        auto filter = Gtk::FileFilter::create();
        filter->set_name("KiCad library");
        filter->add_pattern("*.lib");
        chooser->add_filter(filter);

        if (gtk_native_dialog_run(GTK_NATIVE_DIALOG(native)) == GTK_RESPONSE_ACCEPT) {
            kicad_symbol_import_wizard =
                    KiCadSymbolImportWizard::create(package_uuid, pool, appwin, chooser->get_filename());
            kicad_symbol_import_wizard->present();
            kicad_symbol_import_wizard->signal_hide().connect([this] {
                auto files_saved = kicad_symbol_import_wizard->get_files_saved();
                if (files_saved.size()) {
                    pool_update(nullptr, files_saved);
                }
                delete kicad_symbol_import_wizard;
                kicad_symbol_import_wizard = nullptr;
            });
        }
    }
    else {
        kicad_symbol_import_wizard->present();
    }
}

void PoolNotebook::construct_entities()
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

    add_action_button("Create", bbox, sigc::mem_fun(*this, &PoolNotebook::handle_create_entity));
    add_action_button("Edit", bbox, br, sigc::mem_fun(*this, &PoolNotebook::handle_edit_entity));
    add_action_button("Duplicate", bbox, br, sigc::mem_fun(*this, &PoolNotebook::handle_duplicate_entity));
    if (remote_repo.size())
        add_action_button("Merge", bbox, br,
                          [this](const UUID &uu) { remote_box->merge_item(ObjectType::ENTITY, uu); });
    add_action_button("Import KiCad symbol", bbox,
                      sigc::mem_fun(*this, &PoolNotebook::handle_kicad_symbol_import_wizard))
            ->get_style_context()
            ->add_class("suggested-action");

    auto stack = Gtk::manage(new Gtk::Stack);
    add_preview_stack_switcher(bbox, stack);

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
    preview->signal_goto().connect(sigc::mem_fun(*this, &PoolNotebook::go_to));

    auto info_box = EntityInfoBox::create(pool);
    info_box->signal_goto().connect(sigc::mem_fun(*this, &PoolNotebook::go_to));

    br->signal_selected().connect([this, br, preview, info_box] {
        auto sel = br->get_selected();
        if (!sel) {
            preview->clear();
            return;
        }
        auto entity = pool.get_entity(sel);
        preview->load(entity);
        info_box->load(entity);
    });

    stack->add(*preview, "preview");
    stack->add(*info_box, "info");
    info_box->unreference();

    paned->add2(*stack);
    paned->show_all();

    paned->show_all();

    stack->set_visible_child(*preview);

    append_page(*paned, "Entities");
    install_search_once(paned, br);
}

} // namespace horizon
