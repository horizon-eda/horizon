#include "preview_box.hpp"
#include "canvas/canvas_gl.hpp"
#include "pool/symbol.hpp"
#include <sstream>

namespace horizon {
	SymbolPreviewBox::SymbolPreviewBox(const std::pair<int, bool> &v): Gtk::Box(Gtk::ORIENTATION_VERTICAL, 5), view(v), symbol(UUID::random()) {
		auto tbox = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 5));

		auto la = Gtk::manage(new Gtk::Label());
		std::stringstream ss;
		ss << "<b>";
		ss << view.first << "°";
		if(view.second) {
			ss << " mirrored";
		}
		ss << "</b>";
		la->set_markup(ss.str());
		la->set_xalign(0);
		tbox->pack_start(*la, true, true, 0);

		set_button = Gtk::manage(new Gtk::Button("Set"));
		tbox->pack_start(*set_button, false, false, 0);
		set_button->signal_clicked().connect([this] {
			if(text_placements.size()) {
				set_button->set_label("Set");
				clear_placements();
			}
			else {
				set_button->set_label("Clear");
				set_placements();
			}

		});

		pack_start(*tbox, false, false, 0);
		tbox->show_all();

		canvas = Gtk::manage(new CanvasGL());
		canvas->set_selection_allowed(false);
		pack_start(*canvas, true, true, 0);
		canvas->show();
	}

	void SymbolPreviewBox::update(const Symbol &sym) {
		for(const auto &la: sym.get_layers()) {
			canvas->set_layer_display(la.first, LayerDisplay(true, LayerDisplay::Mode::FILL, la.second.color));
		}
		Placement p;
		p.set_angle_deg(view.first);
		p.mirror = view.second;
		symbol = sym;
		symbol.text_placements.clear();
		for(const auto &it: text_placements) {
			std::tuple<int, bool, UUID> key(view.first, view.second, it.first);
			symbol.text_placements[key] = it.second;
		}
		symbol.apply_placement(p);
		canvas->update(symbol, p);
	}

	void SymbolPreviewBox::zoom_to_fit() {
		auto bb = symbol.get_bbox();
		int64_t pad = 1_mm;
		bb.first.x -= pad;
		bb.first.y -= pad;

		bb.second.x += pad;
		bb.second.y += pad;
		canvas->zoom_to_bbox(bb.first, bb.second);
	}

	void SymbolPreviewBox::set_placements() {
		text_placements.clear();
		for(const auto &it: symbol.texts) {
			text_placements[it.second.uuid] = it.second.placement;
		}
	}

	void SymbolPreviewBox::clear_placements() {
		text_placements.clear();
	}

	std::map<std::tuple<int, bool, UUID>, Placement> SymbolPreviewBox::get_text_placements() const {
		std::map<std::tuple<int, bool, UUID>, Placement> r;
		for(const auto &it: text_placements) {
			std::tuple<int, bool, UUID> key(view.first, view.second, it.first);
			r[key] = it.second;
		}
		return r;
	}

	void SymbolPreviewBox::set_text_placements(const std::map<std::tuple<int, bool, UUID>, Placement> &p) {
		text_placements.clear();
		for(const auto &it: p) {
			int angle;
			bool mirror;
			UUID uu;
			std::tie(angle, mirror, uu) = it.first;
			if(view.first == angle && view.second == mirror) {
				text_placements[uu] = it.second;
				set_button->set_label("Clear");
			}
		}
	}
}
