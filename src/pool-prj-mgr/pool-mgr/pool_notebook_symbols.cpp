#include "pool_notebook.hpp"
#include "duplicate/duplicate_unit.hpp"
#include "util/util.hpp"
#include "dialogs/pool_browser_dialog.hpp"
#include "widgets/pool_browser_symbol.hpp"
#include "widgets/symbol_preview.hpp"
#include <nlohmann/json.hpp>
#include "pool-prj-mgr/pool-prj-mgr-app_win.hpp"
#include "util/gtk_util.hpp"

namespace horizon {
void PoolNotebook::handle_edit_symbol(const UUID &uu)
{
    edit_item(ObjectType::SYMBOL, uu);
}

void PoolNotebook::handle_create_symbol()
{
    auto top = dynamic_cast<Gtk::Window *>(get_ancestor(GTK_TYPE_WINDOW));
    UUID unit_uuid;

    {
        PoolBrowserDialog dia(top, ObjectType::UNIT, pool);
        dia.get_browser().sort_by_mtime();
        if (dia.run() == Gtk::RESPONSE_OK) {
            unit_uuid = dia.get_browser().get_selected();
        }
    }
    if (unit_uuid) {
        handle_create_symbol_for_unit(unit_uuid);
    }
}

void PoolNotebook::handle_duplicate_symbol(const UUID &uu)
{
    if (!uu)
        return;
    auto top = dynamic_cast<Gtk::Window *>(get_ancestor(GTK_TYPE_WINDOW));

    GtkFileChooserNative *native =
            gtk_file_chooser_native_new("Save Symbol", top->gobj(), GTK_FILE_CHOOSER_ACTION_SAVE, "_Save", "_Cancel");
    auto chooser = Glib::wrap(GTK_FILE_CHOOSER(native));
    chooser->set_do_overwrite_confirmation(true);
    auto sym_filename = pool.get_filename(ObjectType::SYMBOL, uu);
    auto sym_basename = Glib::path_get_basename(sym_filename);
    auto sym_dirname = Glib::path_get_dirname(sym_filename);
    chooser->set_current_folder(sym_dirname);
    chooser->set_current_name(DuplicateUnitWidget::insert_filename(sym_basename, "-copy"));

    std::string filename;
    auto success = run_native_filechooser_with_retry(chooser, "Error saving symbol", [this, chooser, &filename, &uu] {
        filename = append_dot_json(chooser->get_filename());
        pool.check_filename_throw(ObjectType::SYMBOL, filename);
        Symbol sym(*pool.get_symbol(uu));
        sym.name += " (Copy)";
        sym.uuid = UUID::random();
        save_json_to_file(filename, sym.serialize());
    });
    if (success) {
        pool_update({filename});
        appwin.spawn(PoolProjectManagerProcess::Type::IMP_SYMBOL, {filename});
    }
}

void PoolNotebook::construct_symbols()
{
    auto br = Gtk::manage(new PoolBrowserSymbol(pool, UUID(), "pool_notebook"));
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

    add_action_button("Create", bbox, sigc::mem_fun(*this, &PoolNotebook::handle_create_symbol));
    add_action_button("Edit", bbox, br, sigc::mem_fun(*this, &PoolNotebook::handle_edit_symbol));
    add_action_button("Duplicate", bbox, br, sigc::mem_fun(*this, &PoolNotebook::handle_duplicate_symbol));
    add_merge_button(bbox, br);
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
    preview->signal_goto().connect(sigc::mem_fun(*this, &PoolNotebook::go_to));
    br->signal_selected().connect([br, preview] {
        auto sel = br->get_selected();
        preview->load(sel);
    });
    paned->add2(*preview);
    paned->show_all();

    br->signal_activated().connect([this, br] { handle_edit_symbol(br->get_selected()); });

    append_page(*paned, "Symbols");
    install_search_once(paned, br);
    create_paned_state_store(paned, "symbols");
}
} // namespace horizon
