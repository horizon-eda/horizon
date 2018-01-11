#include "part_preview.hpp"
#include "pool/part.hpp"
#include "canvas/canvas.hpp"
#include "util/util.hpp"
#include "util/sqlite.hpp"
#include "common/object_descr.hpp"
#include "preview_canvas.hpp"
#include "entity_preview.hpp"

namespace horizon {
	PartPreview::PartPreview(class Pool &p, bool show_goto): Gtk::Box(Gtk::ORIENTATION_VERTICAL, 0), pool(p) {
		auto infogrid = Gtk::manage(new Gtk::Grid());
		infogrid->property_margin() = 8;
		infogrid->set_row_spacing(4);
		infogrid->set_column_spacing(4);

		{
			auto la = Gtk::manage(new Gtk::Label("MPN"));
			la->get_style_context()->add_class("dim-label");
			la->set_halign(Gtk::ALIGN_END);
			infogrid->attach(*la, 0, 0, 1, 1);
		}
		label_MPN = Gtk::manage(new Gtk::Label("1337"));
		label_MPN->set_hexpand(true);
		label_MPN->set_halign(Gtk::ALIGN_START);
		label_MPN->set_selectable(true);
		infogrid->attach(*label_MPN, 1, 0, 1, 1);

		{
			auto la = Gtk::manage(new Gtk::Label("Manufacturer"));
			la->get_style_context()->add_class("dim-label");
			la->set_halign(Gtk::ALIGN_END);
			infogrid->attach(*la, 2, 0, 1, 1);
		}
		label_manufacturer = Gtk::manage(new Gtk::Label("1337"));
		label_manufacturer->set_hexpand(true);
		label_manufacturer->set_halign(Gtk::ALIGN_START);
		label_manufacturer->set_selectable(true);
		infogrid->attach(*label_manufacturer, 3, 0, 1, 1);

		{
			auto la = Gtk::manage(new Gtk::Label("Value"));
			la->get_style_context()->add_class("dim-label");
			la->set_halign(Gtk::ALIGN_END);
			infogrid->attach(*la, 4, 0, 1, 1);
		}
		label_value = Gtk::manage(new Gtk::Label("1337"));
		label_value->set_hexpand(true);
		label_value->set_halign(Gtk::ALIGN_START);
		label_value->set_selectable(true);
		infogrid->attach(*label_value, 5, 0, 1, 1);

		{
			auto la = Gtk::manage(new Gtk::Label("Description"));
			la->get_style_context()->add_class("dim-label");
			la->set_halign(Gtk::ALIGN_END);
			infogrid->attach(*la, 0, 1, 1, 1);
		}
		label_description = Gtk::manage(new Gtk::Label("1337"));
		label_description->set_hexpand(true);
		label_description->set_halign(Gtk::ALIGN_START);
		label_description->set_selectable(true);
		infogrid->attach(*label_description, 1, 1, 1, 1);

		{
			auto la = Gtk::manage(new Gtk::Label("Datasheet"));
			la->get_style_context()->add_class("dim-label");
			la->set_halign(Gtk::ALIGN_END);
			infogrid->attach(*la, 2, 1, 1, 1);
		}
		label_datasheet = Gtk::manage(new Gtk::Label("1337"));
		label_datasheet->set_hexpand(true);
		label_datasheet->set_ellipsize(Pango::ELLIPSIZE_MIDDLE);
		label_datasheet->set_halign(Gtk::ALIGN_START);
		infogrid->attach(*label_datasheet, 3, 1, 1, 1);

		{
			auto la = Gtk::manage(new Gtk::Label("Entity"));
			la->get_style_context()->add_class("dim-label");
			la->set_halign(Gtk::ALIGN_END);
			infogrid->attach(*la, 4, 1, 1, 1);
		}
		label_entity = Gtk::manage(new Gtk::Label("1337"));
		label_entity->set_hexpand(true);
		label_entity->set_halign(Gtk::ALIGN_START);
		label_entity->set_use_markup(true);
		label_entity->set_track_visited_links(false);
		label_entity->signal_activate_link().connect([this] (const std::string &url) {
			std::cout << "link" << url << std::endl;
			s_signal_goto.emit(ObjectType::ENTITY, UUID(url));
			return true;
		}, false);
		infogrid->attach(*label_entity, 5, 1, 1, 1);


		infogrid->show_all();
		pack_start(*infogrid, false, false, 0);

		{
			auto sep = Gtk::manage(new Gtk::Separator(Gtk::ORIENTATION_HORIZONTAL));
			sep->show();
			pack_start(*sep, false, false, 0);
		}

		auto paned = Gtk::manage(new Gtk::Paned(Gtk::ORIENTATION_HORIZONTAL));


		entity_preview = Gtk::manage(new EntityPreview(pool, show_goto));
		entity_preview->signal_goto().connect([this](ObjectType type, const UUID &uu){s_signal_goto.emit(type, uu);});

		paned->add1(*entity_preview);
		paned->child_property_shrink(*entity_preview) = false;

		auto package_box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL, 0));

		auto package_sel_box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 4));
		package_sel_box->property_margin() = 8;
		{
			auto la = Gtk::manage(new Gtk::Label("Package"));
			la->get_style_context()->add_class("dim-label");
			package_sel_box->pack_start(*la, false, false, 0);
		}
		combo_package = Gtk::manage(new Gtk::ComboBoxText);
		combo_package->signal_changed().connect(sigc::mem_fun(this, &PartPreview::handle_package_sel));
		package_sel_box->pack_start(*combo_package, true, true, 0);

		if(show_goto) {
			auto bu = create_goto_button(ObjectType::PACKAGE, [this] {
				return UUID(combo_package->get_active_id());
			});
			package_sel_box->pack_start(*bu, false, false, 0);
		}


		package_box->pack_start(*package_sel_box, false, false, 0);

		{
			auto sep = Gtk::manage(new Gtk::Separator(Gtk::ORIENTATION_HORIZONTAL));
			sep->show();
			package_box->pack_start(*sep, false, false, 0);
		}

		canvas_package = Gtk::manage(new PreviewCanvas(pool));
		package_box->pack_start(*canvas_package, true, true, 0);

		paned->add2(*package_box);
		paned->child_property_shrink(*package_box) = false;
		pack_start(*paned, true, true, 0);
		paned->show_all();

		load(nullptr);

	}

	void PartPreview::load(const Part *p) {
		part = p;
		combo_package->remove_all();

		for(auto it: goto_buttons) {
			it->set_sensitive(part);
		}
		if(!part) {
			label_MPN->set_text("");
			label_value->set_text("");
			label_manufacturer->set_text("");
			label_description->set_text("");
			label_datasheet->set_text("");
			label_entity->set_text("");
			canvas_package->clear();
			entity_preview->load(nullptr);
			return;
		}

		label_MPN->set_text(part->get_MPN());
		label_value->set_text(part->get_value());
		label_manufacturer->set_text(part->get_manufacturer());
		label_description->set_text(part->get_description());
		auto datasheet = part->get_datasheet();
		if(datasheet.size()) {
			label_datasheet->set_markup("<a href=\"" + Glib::Markup::escape_text(datasheet) + "\">" + Glib::Markup::escape_text(datasheet) + "</a>");
			label_datasheet->set_tooltip_text(datasheet);
		}
		else {
			label_datasheet->set_text("");
		}
		label_entity->set_markup("<a href=\"" + (std::string)part->entity->uuid + "\">" + Glib::Markup::escape_text(part->entity->name) + "</a>");

		std::vector<const Gate*> gates;
		gates.reserve(part->entity->gates.size());
		for(const auto &it: part->entity->gates) {
			gates.push_back(&it.second);
		}
		std::sort(gates.begin(), gates.end(), [](const auto *a, const auto *b){return strcmp_natural(a->suffix, b->suffix)<0;});

		entity_preview->load(part->entity);

		const class Package *pkg = part->package;
		combo_package->append((std::string)pkg->uuid, pkg->name + " (Primary)");

		SQLite::Query q(pool.db, "SELECT uuid, name FROM packages WHERE alternate_for=? ORDER BY name");
		q.bind(1, pkg->uuid);
		while(q.step()) {
			UUID uu = q.get<std::string>(0);
			std::string name = q.get<std::string>(1);
			combo_package->append((std::string)uu, name + " (Alternate)");
		}

		combo_package->set_active(0);
	}

	void PartPreview::handle_package_sel() {
		canvas_package->clear();
		if(!part)
			return;
		if(combo_package->get_active_row_number() == -1)
			return;

		canvas_package->load(ObjectType::PACKAGE, UUID(combo_package->get_active_id()));
	}
}
