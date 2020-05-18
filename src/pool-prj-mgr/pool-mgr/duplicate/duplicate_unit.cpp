#include "duplicate_unit.hpp"
#include "duplicate_window.hpp"
#include "pool/pool.hpp"
#include "pool/part.hpp"
#include "pool/unit.hpp"
#include "pool/symbol.hpp"
#include "util/gtk_util.hpp"
#include "../part_wizard/location_entry.hpp"
#include "util/util.hpp"
#include "nlohmann/json.hpp"

namespace horizon {

class DuplicateSymbolWidget : public Gtk::Box {
    friend class DuplicateUnitWidget;

public:
    DuplicateSymbolWidget(Pool *p, const UUID &sym_uuid)
        : Gtk::Box(Gtk::ORIENTATION_VERTICAL, 10), pool(p), sym(pool->get_symbol(sym_uuid))
    {
        auto explain_label = Gtk::manage(new Gtk::Label);
        explain_label->get_style_context()->add_class("dim-label");
        explain_label->set_xalign(0);
        explain_label->set_text("This symbol will be created for the new unit");
        explain_label->show();
        explain_label->set_margin_left(20);

        auto cb = Gtk::manage(new Gtk::CheckButton);
        auto la = Gtk::manage(new Gtk::Label);
        la->set_markup("<b>Symbol: " + sym->name + "</b>");
        cb->add(*la);
        cb->show_all();
        cb->set_active(true);
        cb->signal_toggled().connect([this, cb, explain_label] {
            if (cb->get_active()) {
                explain_label->set_text("This symbol will be created for the new unit");
            }
            else {
                explain_label->set_text("This symbol won't be created for the new unit");
            }
            grid->set_visible(cb->get_active());
        });

        pack_start(*cb, false, false, 0);
        pack_start(*explain_label, false, false, 0);

        grid = Gtk::manage(new Gtk::Grid);
        grid->set_row_spacing(10);
        grid->set_column_spacing(10);
        grid->set_margin_left(20);
        int top = 0;

        name_entry = Gtk::manage(new Gtk::Entry);
        name_entry->set_text(sym->name + " (Copy)");
        name_entry->set_hexpand(true);
        grid_attach_label_and_widget(grid, "Name", name_entry, top);

        location_entry = Gtk::manage(new LocationEntry(pool->get_base_path()));
        location_entry->set_filename(
                DuplicateUnitWidget::insert_filename(pool->get_filename(ObjectType::SYMBOL, sym->uuid), "-copy"));
        grid_attach_label_and_widget(grid, "Filename", location_entry, top);

        grid->show_all();


        pack_start(*grid, true, true, 0);
    }

    std::string duplicate(const UUID &new_unit_uuid)
    {
        if (grid->get_visible()) {
            Symbol new_sym(*sym);
            new_sym.uuid = UUID::random();
            new_sym.name = name_entry->get_text();
            auto new_sym_json = new_sym.serialize();
            new_sym_json["unit"] = (std::string)new_unit_uuid;
            auto filename = location_entry->get_filename();
            save_json_to_file(filename, new_sym_json);
            return filename;
        }
        else {
            return "";
        }
    }

private:
    Pool *pool;
    const Symbol *sym;
    Gtk::Entry *name_entry = nullptr;
    class LocationEntry *location_entry = nullptr;
    Gtk::Grid *grid = nullptr;
};


DuplicateUnitWidget::DuplicateUnitWidget(Pool *p, const UUID &unit_uuid, bool optional)
    : Gtk::Box(Gtk::ORIENTATION_VERTICAL, 10), pool(p), unit(pool->get_unit(unit_uuid))
{
    auto la = Gtk::manage(new Gtk::Label);
    la->set_markup("<b>Unit: " + unit->name + "</b>");
    la->set_xalign(0);
    la->show();
    if (!optional) {
        pack_start(*la, false, false, 0);
    }
    else {
        auto explain_label = Gtk::manage(new Gtk::Label);
        explain_label->get_style_context()->add_class("dim-label");
        explain_label->set_xalign(0);
        explain_label->set_text("The new entity's gates will reference the new unit");
        explain_label->show();
        explain_label->set_margin_left(20);

        auto cb = Gtk::manage(new Gtk::CheckButton);
        cb->add(*la);
        cb->show();
        cb->set_active(true);
        cb->signal_toggled().connect([this, cb, explain_label] {
            grid->set_visible(cb->get_active());
            if (cb->get_active()) {
                explain_label->set_text("The new entity's gates will reference the new unit");
            }
            else {
                explain_label->set_text(
                        "The new entity's gates will reference the existing "
                        "unit");
            }
            for (auto ch : get_children()) {
                if (auto c = dynamic_cast<DuplicateSymbolWidget *>(ch)) {
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
    name_entry->set_text(unit->name + " (Copy)");
    name_entry->set_hexpand(true);
    grid_attach_label_and_widget(grid, "Name", name_entry, top);

    location_entry = Gtk::manage(new LocationEntry(pool->get_base_path()));
    location_entry->set_filename(insert_filename(pool->get_filename(ObjectType::UNIT, unit->uuid), "-copy"));
    grid_attach_label_and_widget(grid, "Filename", location_entry, top);

    grid->show_all();
    grid->set_margin_bottom(10);
    pack_start(*grid, false, false, 0);

    SQLite::Query q(pool->db, "SELECT uuid FROM symbols WHERE unit=?");
    q.bind(1, unit->uuid);
    while (q.step()) {
        auto ws = Gtk::manage(new DuplicateSymbolWidget(pool, UUID(q.get<std::string>(0))));
        pack_start(*ws, false, false, 0);
        ws->show();
        ws->set_margin_start(10);
        ws->set_margin_bottom(10);
    }
}

UUID DuplicateUnitWidget::duplicate(std::vector<std::string> *filenames)
{
    if (grid->get_visible()) {
        Unit new_unit(*unit);
        new_unit.uuid = UUID::random();
        new_unit.name = name_entry->get_text();
        save_json_to_file(location_entry->get_filename(), new_unit.serialize());
        if (filenames)
            filenames->push_back(location_entry->get_filename());

        for (auto ch : get_children()) {
            if (auto c = dynamic_cast<DuplicateSymbolWidget *>(ch)) {
                auto symbol_filename = c->duplicate(new_unit.uuid);
                if (filenames && symbol_filename.size())
                    filenames->push_back(symbol_filename);
            }
        }
        return new_unit.uuid;
    }
    else {
        return unit->uuid;
    }
}

UUID DuplicateUnitWidget::get_uuid() const
{
    return unit->uuid;
}

std::string DuplicateUnitWidget::insert_filename(const std::string &fn, const std::string &ins)
{
    if (endswith(fn, ".json")) {
        std::string s(fn);
        s.resize(s.size() - 5);
        return s + ins + ".json";
    }
    else {
        return fn;
    }
}
} // namespace horizon
