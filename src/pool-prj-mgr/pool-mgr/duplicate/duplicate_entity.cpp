#include "duplicate_entity.hpp"
#include "duplicate_unit.hpp"
#include "duplicate_window.hpp"
#include "pool/pool.hpp"
#include "pool/part.hpp"
#include "pool/unit.hpp"
#include "pool/symbol.hpp"
#include "util/gtk_util.hpp"
#include "util/util.hpp"
#include "../part_wizard/location_entry.hpp"
#include "nlohmann/json.hpp"

namespace horizon {
DuplicateEntityWidget::DuplicateEntityWidget(Pool *p, const UUID &entity_uuid, Gtk::Box *ubox, bool optional)
    : Gtk::Box(Gtk::ORIENTATION_VERTICAL, 10), pool(p), entity(pool->get_entity(entity_uuid))
{
    auto la = Gtk::manage(new Gtk::Label);
    la->set_markup("<b>Entity: " + entity->name + "</b>");
    la->set_xalign(0);
    la->show();
    if (!optional) {
        pack_start(*la, false, false, 0);
    }
    else {
        auto explain_label = Gtk::manage(new Gtk::Label);
        explain_label->get_style_context()->add_class("dim-label");
        explain_label->set_xalign(0);
        explain_label->set_text("The new part will reference the new entity");
        explain_label->show();
        explain_label->set_margin_left(20);


        auto cb = Gtk::manage(new Gtk::CheckButton);
        cb->add(*la);
        cb->show();
        cb->set_active(true);
        cb->signal_toggled().connect([this, cb, explain_label] {
            grid->set_visible(cb->get_active());
            if (cb->get_active()) {
                explain_label->set_text("The new part will reference the new entity");
            }
            else {
                explain_label->set_text("The new part will reference the existing entity");
            }
            for (auto ch : unit_box->get_children()) {
                if (auto c = dynamic_cast<DuplicateUnitWidget *>(ch)) {
                    c->set_visible(cb->get_active());
                }
            }
        });
        pack_start(*cb, false, false, 0);
        pack_start(*explain_label, false, false, 0);
    }

    grid = Gtk::manage(new Gtk::Grid);
    grid->set_row_spacing(10);
    grid->set_column_spacing(10);
    if (optional)
        grid->set_margin_left(20);

    int top = 0;

    name_entry = Gtk::manage(new Gtk::Entry);
    name_entry->set_text(entity->name + " (Copy)");
    name_entry->set_hexpand(true);
    grid_attach_label_and_widget(grid, "Name", name_entry, top);

    location_entry = Gtk::manage(new LocationEntry(pool->get_base_path()));
    location_entry->set_filename(
            DuplicateUnitWidget::insert_filename(pool->get_filename(ObjectType::ENTITY, entity->uuid), "-copy"));
    grid_attach_label_and_widget(grid, "Filename", location_entry, top);

    grid->show_all();
    grid->set_margin_bottom(10);

    pack_start(*grid, false, false, 0);


    unit_box = ubox;

    std::set<UUID> units;
    for (const auto &it : entity->gates) {
        units.insert(it.second.unit->uuid);
    }

    for (const auto &uu : units) {
        auto wu = Gtk::manage(new DuplicateUnitWidget(pool, uu, true));
        unit_box->pack_start(*wu, false, false, 0);
        wu->show();
        wu->set_margin_start(10);
        wu->set_margin_bottom(10);
    }
}

UUID DuplicateEntityWidget::get_uuid() const
{
    return entity->uuid;
}

UUID DuplicateEntityWidget::duplicate(std::vector<std::string> *filenames)
{
    if (grid->get_visible()) {
        Entity new_entity(*entity);
        new_entity.uuid = UUID::random();
        new_entity.name = name_entry->get_text();
        auto new_entity_json = new_entity.serialize();

        std::map<UUID, UUID> unit_map;
        for (auto ch : unit_box->get_children()) {
            if (auto c = dynamic_cast<DuplicateUnitWidget *>(ch)) {
                auto old_unit_uuid = c->get_uuid();
                auto new_unit_uuid = c->duplicate(filenames);
                unit_map[old_unit_uuid] = new_unit_uuid;
            }
        }
        auto &o = new_entity_json.at("gates");
        for (auto it = o.begin(); it != o.end(); ++it) {
            auto &v = it.value();
            UUID unit_uu(v.at("unit").get<std::string>());
            v["unit"] = (std::string)unit_map.at(unit_uu);
        }
        auto entity_filename = location_entry->get_filename();
        if (filenames)
            filenames->push_back(entity_filename);
        save_json_to_file(entity_filename, new_entity_json);
        return new_entity.uuid;
    }
    else {
        return entity->uuid;
    }
}
} // namespace horizon
