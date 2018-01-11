#include "pool_notebook.hpp"
#include "editor_window.hpp"
#include "duplicate/duplicate_unit.hpp"
#include "util/util.hpp"

namespace horizon {
	void PoolNotebook::handle_edit_padstack(const UUID &uu) {
		if(!uu)
			return;
		auto path = pool.get_filename(ObjectType::PADSTACK, uu);
		spawn(PoolManagerProcess::Type::IMP_PADSTACK, {path});
	}

	void PoolNotebook::handle_create_padstack() {
		auto top = dynamic_cast<Gtk::Window*>(get_ancestor(GTK_TYPE_WINDOW));

		GtkFileChooserNative *native = gtk_file_chooser_native_new ("Save Padstack",
			top->gobj(),
			GTK_FILE_CHOOSER_ACTION_SAVE,
			"_Save",
			"_Cancel");
		auto chooser = Glib::wrap(GTK_FILE_CHOOSER(native));
		chooser->set_do_overwrite_confirmation(true);
		chooser->set_current_name("padstack.json");
		chooser->set_current_folder(Glib::build_filename(base_path, "padstacks"));

		if(gtk_native_dialog_run (GTK_NATIVE_DIALOG (native))==GTK_RESPONSE_ACCEPT) {
			std::string fn = EditorWindow::fix_filename(chooser->get_filename());
			Padstack ps(horizon::UUID::random());
			save_json_to_file(fn, ps.serialize());
			spawn(PoolManagerProcess::Type::IMP_PADSTACK, {fn});
		}
	}

	void PoolNotebook::handle_duplicate_padstack(const UUID &uu) {
		if(!uu)
			return;
		auto top = dynamic_cast<Gtk::Window*>(get_ancestor(GTK_TYPE_WINDOW));

		GtkFileChooserNative *native = gtk_file_chooser_native_new ("Save Padstack",
			top->gobj(),
			GTK_FILE_CHOOSER_ACTION_SAVE,
			"_Save",
			"_Cancel");
		auto chooser = Glib::wrap(GTK_FILE_CHOOSER(native));
		chooser->set_do_overwrite_confirmation(true);
		auto ps_filename = pool.get_filename(ObjectType::PADSTACK, uu);
		auto ps_basename = Glib::path_get_basename(ps_filename);
		auto ps_dirname = Glib::path_get_dirname(ps_filename);
		chooser->set_current_folder(ps_dirname);
		chooser->set_current_name(DuplicateUnitWidget::insert_filename(ps_basename, "-copy"));

		if(gtk_native_dialog_run (GTK_NATIVE_DIALOG (native))==GTK_RESPONSE_ACCEPT) {
			std::string fn = EditorWindow::fix_filename(chooser->get_filename());
			Padstack ps(*pool.get_padstack(uu));
			ps.name += " (Copy)";
			ps.uuid = UUID::random();
			save_json_to_file(fn, ps.serialize());
			spawn(PoolManagerProcess::Type::IMP_PADSTACK, {fn});
		}

	}
}
