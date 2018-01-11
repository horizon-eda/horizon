#include "pool_notebook.hpp"
#include "editor_window.hpp"
#include "duplicate/duplicate_unit.hpp"
#include "util/util.hpp"
#include "dialogs/pool_browser_dialog.hpp"
#include "widgets/pool_browser_unit.hpp"

namespace horizon {
	void PoolNotebook::handle_edit_symbol(const UUID &uu) {
		if(!uu)
			return;
		auto path = pool.get_filename(ObjectType::SYMBOL, uu);
		spawn(PoolManagerProcess::Type::IMP_SYMBOL, {path});
	}

	void PoolNotebook::handle_create_symbol() {
		auto top = dynamic_cast<Gtk::Window*>(get_ancestor(GTK_TYPE_WINDOW));
		UUID unit_uuid;

		{
			PoolBrowserDialog dia(top, ObjectType::UNIT, &pool);
			if(dia.run() == Gtk::RESPONSE_OK) {
				unit_uuid = dia.get_browser()->get_selected();
			}
		}
		if(unit_uuid) {
			GtkFileChooserNative *native = gtk_file_chooser_native_new ("Save Symbol",
				top->gobj(),
				GTK_FILE_CHOOSER_ACTION_SAVE,
				"_Save",
				"_Cancel");
			auto chooser = Glib::wrap(GTK_FILE_CHOOSER(native));
			chooser->set_do_overwrite_confirmation(true);
			auto unit_filename = pool.get_filename(ObjectType::UNIT, unit_uuid);
			auto basename = Gio::File::create_for_path(unit_filename)->get_basename();
			chooser->set_current_folder(Glib::build_filename(base_path, "symbols"));
			chooser->set_current_name(basename);

			if(gtk_native_dialog_run (GTK_NATIVE_DIALOG (native))==GTK_RESPONSE_ACCEPT) {
				std::string fn = EditorWindow::fix_filename(chooser->get_filename());
				Symbol sym(horizon::UUID::random());
				auto unit = pool.get_unit(unit_uuid);
				sym.name = unit->name;
				sym.unit = unit;
				save_json_to_file(fn, sym.serialize());
				spawn(PoolManagerProcess::Type::IMP_SYMBOL, {fn});
			}
		}
	}

	void PoolNotebook::handle_duplicate_symbol(const UUID &uu) {
		if(!uu)
			return;
		auto top = dynamic_cast<Gtk::Window*>(get_ancestor(GTK_TYPE_WINDOW));

		GtkFileChooserNative *native = gtk_file_chooser_native_new ("Save Symbol",
			top->gobj(),
			GTK_FILE_CHOOSER_ACTION_SAVE,
			"_Save",
			"_Cancel");
		auto chooser = Glib::wrap(GTK_FILE_CHOOSER(native));
		chooser->set_do_overwrite_confirmation(true);
		auto sym_filename = pool.get_filename(ObjectType::SYMBOL, uu);
		auto sym_basename = Glib::path_get_basename(sym_filename);
		auto sym_dirname = Glib::path_get_dirname(sym_filename);
		chooser->set_current_folder(sym_dirname);
		chooser->set_current_name(DuplicateUnitWidget::insert_filename(sym_basename, "-copy"));

		if(gtk_native_dialog_run (GTK_NATIVE_DIALOG (native))==GTK_RESPONSE_ACCEPT) {
			std::string fn = EditorWindow::fix_filename(chooser->get_filename());
			Symbol sym(*pool.get_symbol(uu));
			sym.name += " (Copy)";
			sym.uuid = UUID::random();
			save_json_to_file(fn, sym.serialize());
			spawn(PoolManagerProcess::Type::IMP_SYMBOL, {fn});
		}
	}
}

