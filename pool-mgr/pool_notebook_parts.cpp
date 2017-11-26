#include "pool_notebook.hpp"
#include "editor_window.hpp"
#include "duplicate/duplicate_unit.hpp"
#include "util.hpp"
#include "create_part_dialog.hpp"

namespace horizon {
	void PoolNotebook::handle_edit_part(const UUID &uu) {
		if(!uu)
			return;
		auto path = pool.get_filename(ObjectType::PART, uu);
		spawn(PoolManagerProcess::Type::PART, {path});
	}

	void PoolNotebook::handle_create_part() {
		auto top = dynamic_cast<Gtk::Window*>(get_ancestor(GTK_TYPE_WINDOW));
		auto entity_uuid = UUID();
		auto package_uuid = UUID();
		{
			CreatePartDialog dia(top, &pool);
			if(dia.run() == Gtk::RESPONSE_OK) {
				entity_uuid = dia.get_entity();
				package_uuid = dia.get_package();
			}
		}
		if(!(entity_uuid && package_uuid))
			return;

		auto entity = pool.get_entity(entity_uuid);
		GtkFileChooserNative *native = gtk_file_chooser_native_new ("Save Part",
			top->gobj(),
			GTK_FILE_CHOOSER_ACTION_SAVE,
			"_Save",
			"_Cancel");
		auto chooser = Glib::wrap(GTK_FILE_CHOOSER(native));
		chooser->set_do_overwrite_confirmation(true);
		chooser->set_current_name(entity->name+".json");
		chooser->set_current_folder(Glib::build_filename(base_path, "parts"));

		if(gtk_native_dialog_run (GTK_NATIVE_DIALOG (native))==GTK_RESPONSE_ACCEPT) {
			std::string fn = EditorWindow::fix_filename(chooser->get_filename());
			Part part(horizon::UUID::random());
			auto package = pool.get_package(package_uuid);
			part.attributes[Part::Attribute::MPN] = {false, entity->name};
			part.attributes[Part::Attribute::MANUFACTURER] = {false, entity->manufacturer};
			part.package = package;
			part.entity = entity;
			save_json_to_file(fn, part.serialize());
			spawn(PoolManagerProcess::Type::PART, {fn});
		}
	}

	void PoolNotebook::handle_create_part_from_part(const UUID &uu) {
		if(!uu)
			return;
		auto base_part = pool.get_part(uu);
		auto top = dynamic_cast<Gtk::Window*>(get_ancestor(GTK_TYPE_WINDOW));

		GtkFileChooserNative *native = gtk_file_chooser_native_new ("Save Part",
			top->gobj(),
			GTK_FILE_CHOOSER_ACTION_SAVE,
			"_Save",
			"_Cancel");
		auto chooser = Glib::wrap(GTK_FILE_CHOOSER(native));
		chooser->set_do_overwrite_confirmation(true);
		chooser->set_current_name(base_part->get_MPN()+".json");
		chooser->set_current_folder(Glib::path_get_dirname(pool.get_filename(ObjectType::PART, uu)));

		if(gtk_native_dialog_run (GTK_NATIVE_DIALOG (native))==GTK_RESPONSE_ACCEPT) {
			std::string fn = EditorWindow::fix_filename(chooser->get_filename());
			Part part(horizon::UUID::random());
			part.base = base_part;
			part.attributes[Part::Attribute::MPN] = {true, base_part->get_MPN()};
			part.attributes[Part::Attribute::MANUFACTURER] = {true, base_part->get_manufacturer()};
			part.attributes[Part::Attribute::VALUE] = {true, base_part->get_value()};
			save_json_to_file(fn, part.serialize());
			spawn(PoolManagerProcess::Type::PART, {fn});
		}
	}

	void PoolNotebook::handle_duplicate_part(const UUID &uu) {
		if(!uu)
			return;
		show_duplicate_window(ObjectType::PART, uu);
	}
}
