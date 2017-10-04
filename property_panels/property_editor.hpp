#pragma once
#include <gtkmm.h>
#include "core/core.hpp"
#include "object_descr.hpp"

namespace horizon {
	class PropertyEditor: public Gtk::Box {
		public:
			PropertyEditor(const UUID &uu, ObjectType t, ObjectProperty::ID prop, Core *c, class PropertyEditors *p);
			void construct();
			virtual void reload() {}

			virtual ~PropertyEditor() {}

			class PropertyEditors *parent;
			const ObjectProperty::ID property_id;
		protected:
			Core *core;
			const UUID uuid;
			const ObjectType type;
			bool mute = false;

			const ObjectProperty &property;
			Gtk::Button *apply_all_button;

			virtual Gtk::Widget *create_editor();
			virtual void set_from_other(PropertyEditor *other) {}

		private :
			void handle_apply_all();
	};

	class PropertyEditorBool : public PropertyEditor {
		using PropertyEditor::PropertyEditor;
		public:
			virtual void reload();
		protected:
			virtual Gtk::Widget *create_editor();
		private :
			Gtk::Switch *sw;
			void changed();
			virtual void set_from_other(PropertyEditor *other);
	};
	class PropertyEditorStringRO : public PropertyEditor {
		using PropertyEditor::PropertyEditor;
		public:
			virtual void reload();
		protected:
			virtual Gtk::Widget *create_editor();
		private :
			Gtk::Label *la;
	};

	class PropertyEditorLength : public PropertyEditor {
		using PropertyEditor::PropertyEditor;
		public:
			void set_range(int64_t min, int64_t max);
		protected:
			virtual Gtk::Widget *create_editor();
		private :
			Gtk::SpinButton *sp;
			bool sp_output();
			int sp_input(double *v);
			void changed();
			virtual void set_from_other(PropertyEditor *other);
			virtual void reload();
			std::pair<int64_t, int64_t> range = {0, 1e9};
	};

	class PropertyEditorString: public PropertyEditor {
		using PropertyEditor::PropertyEditor;
		public:
			virtual void reload();

		protected:
			virtual Gtk::Widget *create_editor();
		private :
			Gtk::Entry *en;
			void changed();
			void activate();
			bool focus_out_event(GdkEventFocus *e);
			virtual void set_from_other(PropertyEditor *other);
			bool modified = false;
	};

	class PropertyEditorLayer: public PropertyEditor {
		using PropertyEditor::PropertyEditor;
		public:
			virtual void reload();
			bool copper_only = false;

		protected:
			virtual Gtk::Widget *create_editor();

		private :
			Gtk::ComboBoxText *combo;
			void changed();
			virtual void set_from_other(PropertyEditor *other);
	};

	class PropertyEditorNetClass: public PropertyEditor {
		using PropertyEditor::PropertyEditor;
		public:
			virtual void reload();

		protected:
			virtual Gtk::Widget *create_editor();

		private :
			Gtk::ComboBoxText *combo;
			void changed();
			virtual void set_from_other(PropertyEditor *other);
	};

	class PropertyEditorEnum: public PropertyEditor {
		using PropertyEditor::PropertyEditor;
		public:
			virtual void reload();

		protected:
			virtual Gtk::Widget *create_editor();

		private :
			Gtk::ComboBoxText *combo;
			void changed();
			virtual void set_from_other(PropertyEditor *other);
	};


}
