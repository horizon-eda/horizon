#include "editor_window.hpp"
#include "unit_editor.hpp"
#include "entity_editor.hpp"
#include "part_editor.hpp"
#include "unit.hpp"
#include "entity.hpp"
#include "part.hpp"
#include "util.hpp"
#include "pool.hpp"

namespace horizon {

	EditorWindowStore::EditorWindowStore(const std::string &fn): filename(fn) {}

	void EditorWindowStore::save() {
		save_as(filename);
	}

	class UnitStore: public EditorWindowStore {
		public:
			UnitStore(const std::string &fn): EditorWindowStore(fn), unit(filename.size()?Unit::new_from_file(filename):Unit(UUID::random())) {

			}
			void save_as(const std::string &fn) {
				save_json_to_file(fn, unit.serialize());
				filename = fn;
			}
			Unit unit;

	};

	class EntityStore: public EditorWindowStore {
		public:
			EntityStore(const std::string &fn, class Pool *pool): EditorWindowStore(fn), entity(filename.size()?Entity::new_from_file(filename, *pool):Entity(UUID::random())) {

			}
			void save_as(const std::string &fn) {
				save_json_to_file(fn, entity.serialize());
				filename = fn;
			}
			Entity entity;

	};

	class PartStore: public EditorWindowStore {
		public:
			PartStore(const std::string &fn, class Pool *pool): EditorWindowStore(fn), part(filename.size()?Part::new_from_file(filename, *pool):Part(UUID::random())) {

			}
			void save_as(const std::string &fn) {
				save_json_to_file(fn, part.serialize());
				filename = fn;
			}
			Part part;

	};

	EditorWindow::EditorWindow(ObjectType type, const std::string &filename, Pool *p): Gtk::Window(), pool(p) {
		set_type_hint(Gdk::WINDOW_TYPE_HINT_DIALOG);
		auto hb = Gtk::manage(new Gtk::HeaderBar());
		set_titlebar(*hb);

		std::string save_label;
		if(filename.size()) {
			save_label = "Save";
		}
		else {
			save_label = "Save as..";
		}
		auto save_buton = Gtk::manage(new Gtk::Button(save_label));
		hb->pack_start(*save_buton);

		hb->show_all();
		hb->set_show_close_button(true);



		Gtk::Widget *editor = nullptr;
        switch(type) {
        	case ObjectType::UNIT: {
        		auto st = new UnitStore(filename);
        		store.reset(st);
        		editor = UnitEditor::create(&st->unit);
        		hb->set_title("Unit Editor");
        	} break;
        	case ObjectType::ENTITY: {
        		auto st = new EntityStore(filename, pool);
        		store.reset(st);
        		auto ed =  EntityEditor::create(&st->entity, pool);
        		editor = ed;
        		iface = ed;
        		hb->set_title("Entity Editor");
        	} break;
        	case ObjectType::PART: {
        		auto st = new PartStore(filename, pool);
        		store.reset(st);
        		auto ed =  PartEditor::create(&st->part, pool);
        		editor = ed;
        		iface = ed;
        		hb->set_title("Part Editor");
        	} break;
        	default:;
        }
        editor->show();
        add(*editor);
        editor->unreference();
        set_default_size(-1, 600);

        save_buton->signal_clicked().connect([this, save_buton]{
        	if(iface)
        		iface->save();

        	if(store->filename.size()) {
        		store->save();
        	}
        	else {
        		Gtk::FileChooserDialog fc(*this, "Save as", Gtk::FILE_CHOOSER_ACTION_SAVE);
				fc.set_do_overwrite_confirmation(true);
				fc.set_current_name("something.json");
				fc.add_button("_Cancel", Gtk::RESPONSE_CANCEL);
				fc.add_button("_Save", Gtk::RESPONSE_ACCEPT);
				if(fc.run()==Gtk::RESPONSE_ACCEPT) {
					std::string fn = fc.get_filename();
					store->save_as(fn);
					save_buton->set_label("Save");
				}
        	}

        });
	}

	void EditorWindow::reload() {
		if(iface) {
			iface->reload();
		}
	}
}
