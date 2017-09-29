#include "edit_pad_parameter_set.hpp"
#include "widgets/parameter_set_editor.hpp"
#include "package/pad.hpp"
#include <iostream>
#include <deque>
#include <algorithm>

namespace horizon {

	class MyParameterSetEditor: public ParameterSetEditor {
		public:
			typedef sigc::signal<void, ParameterID> type_signal_apply_all;
			type_signal_apply_all signal_apply_all() {return s_signal_apply_all;}

		private:
			type_signal_apply_all s_signal_apply_all;

			using ParameterSetEditor::ParameterSetEditor;
			Gtk::Widget *create_extra_widget(ParameterID id) override {
				auto w = Gtk::manage(new Gtk::Button());
				w->set_image_from_icon_name("object-select-symbolic", Gtk::ICON_SIZE_BUTTON);
				w->set_tooltip_text("Apply to all pads");
				w->signal_clicked().connect([this, id] {
					s_signal_apply_all.emit(id);
				});
				return w;
			}
	};

	PadParameterSetDialog::PadParameterSetDialog(Gtk::Window *parent, std::set<class Pad*> &pads) :
		Gtk::Dialog("Edit pad parameter set", *parent, Gtk::DialogFlags::DIALOG_MODAL|Gtk::DialogFlags::DIALOG_USE_HEADER_BAR)
		{
		set_default_size(400, 300);
		add_button("Cancel", Gtk::ResponseType::RESPONSE_CANCEL);
		add_button("OK", Gtk::ResponseType::RESPONSE_OK);
		set_default_response(Gtk::ResponseType::RESPONSE_OK);

		auto box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL, 0));
		auto combo = Gtk::manage(new Gtk::ComboBoxText());
		combo->set_margin_start(8);
		combo->set_margin_end(8);
		combo->set_margin_top(8);
		combo->set_margin_bottom(8);
		box->pack_start(*combo, false, false, 0);

		std::map<UUID, Pad*> padmap;
		std::deque<Pad*> pads_sorted(pads.begin(), pads.end());
		std::sort(pads_sorted.begin(), pads_sorted.end(), [](const auto a, const auto b){return a->name < b->name;});

		for(auto it: pads_sorted) {
			combo->append(static_cast<std::string>(it->uuid), it->name);
			padmap.emplace(it->uuid, it);
		}

		combo->signal_changed().connect([this, combo, padmap, box, pads] {
			UUID uu(combo->get_active_id());
			auto pad = padmap.at(uu);
			if(editor)
				delete editor;
			editor = Gtk::manage(new MyParameterSetEditor(&pad->parameter_set, false));
			editor->populate();
			editor->signal_apply_all().connect([this, pads, pad] (ParameterID id) {
				for(auto it: pads) {
					it->parameter_set[id] = pad->parameter_set.at(id);
				}
			});
			editor->signal_activate_last().connect([this] {
				response(Gtk::RESPONSE_OK);
			});
			box->pack_start(*editor, true, true, 0);
			editor->show();
		});

		combo->set_active(0);
		editor->focus_first();

		get_content_area()->pack_start(*box, true, true, 0);
		get_content_area()->set_border_width(0);


		show_all();
	}
}
