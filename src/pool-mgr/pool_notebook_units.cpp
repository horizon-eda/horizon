#include "pool_notebook.hpp"
#include "editor_window.hpp"
#include "util/util.hpp"
#include "nlohmann/json.hpp"

namespace horizon {
void PoolNotebook::handle_create_unit()
{
    spawn(PoolManagerProcess::Type::UNIT, {""});
}

void PoolNotebook::handle_edit_unit(const UUID &uu)
{
    if (!uu)
        return;
    auto path = pool.get_filename(ObjectType::UNIT, uu);
    spawn(PoolManagerProcess::Type::UNIT, {path});
}

void PoolNotebook::handle_create_symbol_for_unit(const UUID &uu)
{
    if (!uu)
        return;
    auto top = dynamic_cast<Gtk::Window *>(get_ancestor(GTK_TYPE_WINDOW));

    GtkFileChooserNative *native =
            gtk_file_chooser_native_new("Save Symbol", top->gobj(), GTK_FILE_CHOOSER_ACTION_SAVE, "_Save", "_Cancel");
    auto chooser = Glib::wrap(GTK_FILE_CHOOSER(native));
    chooser->set_do_overwrite_confirmation(true);
    auto unit_filename = pool.get_filename(ObjectType::UNIT, uu);
    auto basename = Gio::File::create_for_path(unit_filename)->get_basename();
    chooser->set_current_folder(Glib::build_filename(base_path, "symbols"));
    chooser->set_current_name(basename);

    if (gtk_native_dialog_run(GTK_NATIVE_DIALOG(native)) == GTK_RESPONSE_ACCEPT) {
        std::string fn = EditorWindow::fix_filename(chooser->get_filename());
        Symbol sym(horizon::UUID::random());
        auto unit = pool.get_unit(uu);
        sym.name = unit->name;
        sym.unit = unit;
        save_json_to_file(fn, sym.serialize());
        spawn(PoolManagerProcess::Type::IMP_SYMBOL, {fn});
    }
}

void PoolNotebook::handle_create_entity_for_unit(const UUID &uu)
{
    if (!uu)
        return;
    auto top = dynamic_cast<Gtk::Window *>(get_ancestor(GTK_TYPE_WINDOW));

    GtkFileChooserNative *native =
            gtk_file_chooser_native_new("Save Entity", top->gobj(), GTK_FILE_CHOOSER_ACTION_SAVE, "_Save", "_Cancel");
    auto chooser = Glib::wrap(GTK_FILE_CHOOSER(native));
    chooser->set_do_overwrite_confirmation(true);
    auto unit_filename = pool.get_filename(ObjectType::UNIT, uu);
    auto basename = Gio::File::create_for_path(unit_filename)->get_basename();
    chooser->set_current_folder(Glib::build_filename(base_path, "entities"));
    chooser->set_current_name(basename);

    if (gtk_native_dialog_run(GTK_NATIVE_DIALOG(native)) == GTK_RESPONSE_ACCEPT) {
        std::string fn = EditorWindow::fix_filename(chooser->get_filename());
        Entity entity(horizon::UUID::random());
        auto unit = pool.get_unit(uu);
        entity.name = unit->name;
        auto uu2 = UUID::random();
        auto gate = &entity.gates.emplace(uu2, uu2).first->second;
        gate->unit = unit;
        gate->name = "Main";

        save_json_to_file(fn, entity.serialize());
        spawn(PoolManagerProcess::Type::ENTITY, {fn});
    }
}

void PoolNotebook::handle_duplicate_unit(const UUID &uu)
{
    if (!uu)
        return;
    show_duplicate_window(ObjectType::UNIT, uu);
}
} // namespace horizon
