#include "symbol_preview_window.hpp"
#include "preview_box.hpp"

namespace horizon{
	SymbolPreviewWindow::SymbolPreviewWindow(Gtk::Window *parent): Gtk::Window() {
		set_transient_for(*parent);
		set_type_hint(Gdk::WINDOW_TYPE_HINT_DIALOG);
		auto hb = Gtk::manage(new Gtk::HeaderBar());
		hb->set_show_close_button(true);
		hb->set_title("Symbol preview");

		auto fit_button = Gtk::manage(new Gtk::Button("Fit views"));
		fit_button->signal_clicked().connect([this]{
			for(auto &it: previews) {
				it.second->zoom_to_fit();
			}
		});
		hb->pack_start(*fit_button);

		set_default_size(800, 300);
		set_titlebar(*hb);
		hb->show_all();


		auto grid = Gtk::manage(new Gtk::Grid());
		grid->set_row_spacing(10);
		grid->set_column_spacing(10);
		grid->set_row_homogeneous(true);
		grid->set_column_homogeneous(true);
		grid->set_margin_start(10);
		grid->set_margin_end(10);
		grid->set_margin_top(10);
		grid->set_margin_bottom(10);
		for(int left=0; left<4; left++) {
			for(int top=0; top<2; top++) {
				std::pair<int, bool> view(left*90, top);
				auto pv = Gtk::manage(new SymbolPreviewBox(view));
				previews[view] = pv;
				grid->attach(*pv, left, top, 1,1);
				pv->show();
			}
		}
		add(*grid);
		grid->show();
	}

	void SymbolPreviewWindow::update(const class Symbol  &sym) {
		for(auto &it: previews) {
			it.second->update(sym);
		}
	}

	std::map<std::tuple<int, bool, UUID>, Placement> SymbolPreviewWindow::get_text_placements() const {
		std::map<std::tuple<int, bool, UUID>, Placement> r;
		for(const auto &it: previews) {
			auto m = it.second->get_text_placements();
			r.insert(m.begin(), m.end());
		}
		return r;
	}

	void SymbolPreviewWindow::set_text_placements(const std::map<std::tuple<int, bool, UUID>, Placement> &p) {
		for(auto &it: previews) {
			it.second->set_text_placements(p);
		}
	}
}
