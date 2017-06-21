#include "chooser_buttons.hpp"
#include "dialogs/dialogs.hpp"
#include "board/via_padstack_provider.hpp"
#include "dialogs/pool_browser_padstack.hpp"
#include "padstack.hpp"

namespace horizon {
	PadstackButton::PadstackButton(Pool &p, const UUID &pkg_uuid):Glib::ObjectBase (typeid(PadstackButton)),  Gtk::Button("fixme"), p_property_selected_uuid(*this, "selected-uuid"), pool(p), package_uuid(pkg_uuid) {
				update_label();
				property_selected_uuid().signal_changed().connect([this]{update_label();});
			}

	void PadstackButton::on_clicked() {
		Gtk::Button::on_clicked();
		auto top = dynamic_cast<Gtk::Window*>(get_ancestor(GTK_TYPE_WINDOW));
		PoolBrowserDialogPadstack pb(top, &pool, package_uuid);
		pb.run();
		if(pb.selection_valid) {
			p_property_selected_uuid = pb.selected_uuid;
			update_label();
		}
	}

	void PadstackButton::update_label() {
		UUID uu = p_property_selected_uuid;
		if(uu) {
			set_label(pool.get_padstack(uu)->name);
		}
		else {
			set_label("no padstack");
		}
	}

	ViaPadstackButton::ViaPadstackButton(ViaPadstackProvider &vpp):Glib::ObjectBase (typeid(ViaPadstackButton)),  Gtk::Button("fixme"), p_property_selected_uuid(*this, "selected-uuid"), via_padstack_provider(vpp) {
				update_label();
				property_selected_uuid().signal_changed().connect([this]{update_label();});
			}

	void ViaPadstackButton::on_clicked() {
		Gtk::Button::on_clicked();
		auto top = dynamic_cast<Gtk::Window*>(get_ancestor(GTK_TYPE_WINDOW));
		Dialogs dias;
		dias.set_parent(top);
		auto r = dias.select_via_padstack(&via_padstack_provider);
		if(r.first) {
			p_property_selected_uuid = r.second;
			update_label();
		}
	}

	void ViaPadstackButton::update_label() {
		UUID uu = p_property_selected_uuid;
		if(uu) {
			set_label(via_padstack_provider.get_padstack(uu)->name);
		}
		else {
			set_label("no padstack");
		}
	}

}
