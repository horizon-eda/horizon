#include "check_settings_dialog.hpp"
#include "checks/check.hpp"
#include "checks/single_pin_net.hpp"
#include <iostream>
#include <deque>
#include <algorithm>
#include <assert.h>

namespace horizon {


	class CheckSettingsEditor {
		public:
			virtual void load(CheckSettings *set) = 0;
			virtual void save(CheckSettings *set) = 0;

			virtual ~CheckSettingsEditor() {}
	};


	class CheckSettingsSinglePinNet: public CheckSettingsEditor, public Gtk::CheckButton {
		public:
		CheckSettingsSinglePinNet(): Gtk::CheckButton("Include unnamed nets") {
			set_margin_bottom(20);
			set_margin_top(20);
			set_margin_end(20);
			set_margin_start(20);
			set_halign(Gtk::ALIGN_START);
		}

		void load(CheckSettings *s) override {
			auto settings = dynamic_cast<CheckSinglePinNet::Settings*>(s);
			assert(settings);
			set_active(settings->include_unnamed);
		}
		void save(CheckSettings *s) override {
			auto settings = dynamic_cast<CheckSinglePinNet::Settings*>(s);
			assert(settings);
			settings->include_unnamed = get_active();
		}
	};

	CheckSettingsDialog::CheckSettingsDialog(Gtk::Window *parent, CheckBase *c) :
		Gtk::Dialog("Check settings", *parent, Gtk::DialogFlags::DIALOG_MODAL|Gtk::DialogFlags::DIALOG_USE_HEADER_BAR),
		check(c)
		{
		add_button("Cancel", Gtk::ResponseType::RESPONSE_CANCEL);
		auto ok_button = add_button("OK", Gtk::ResponseType::RESPONSE_OK);
		ok_button->signal_clicked().connect(sigc::mem_fun(this, &CheckSettingsDialog::ok_clicked));
		set_default_response(Gtk::ResponseType::RESPONSE_OK);
		//set_default_size(400, 300);
		if(dynamic_cast<CheckSinglePinNet*>(c)) {
			editor = Gtk::manage(new CheckSettingsSinglePinNet);
		}

		if(editor) {
			editor_w = dynamic_cast<Gtk::Widget*>(editor);
			assert(editor_w);
			get_content_area()->set_border_width(0);
			get_content_area()->pack_start(*editor_w, false, false, 0);
			editor->load(c->get_settings());
		}

		show_all();
	}

	void CheckSettingsDialog::ok_clicked() {
		editor->save(check->get_settings());
	}
}
