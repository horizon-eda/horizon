#include "entity_preview.hpp"
#include "pool/entity.hpp"
#include "pool/pool.hpp"
#include "pool/part.hpp"
#include "canvas/canvas.hpp"
#include "util/util.hpp"
#include "util/sqlite.hpp"
#include "common/object_descr.hpp"
#include "preview_canvas.hpp"

namespace horizon {
EntityPreview::EntityPreview(class Pool &p, bool show_goto) : Gtk::Box(Gtk::ORIENTATION_VERTICAL, 0), pool(p)
{

    auto symbol_sel_box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 4));
    symbol_sel_box->property_margin() = 8;
    {
        auto la = Gtk::manage(new Gtk::Label("Gate"));
        la->get_style_context()->add_class("dim-label");
        symbol_sel_box->pack_start(*la, false, false, 0);
    }
    combo_gate = Gtk::manage(new GenericComboBox<UUID>);
    combo_gate->get_cr_text().property_ellipsize() = Pango::ELLIPSIZE_END;
    combo_gate->get_cr_text().property_width_chars() = 10;
    combo_gate->signal_changed().connect(sigc::mem_fun(*this, &EntityPreview::handle_gate_sel));
    symbol_sel_box->pack_start(*combo_gate, true, true, 0);

    if (show_goto) {
        goto_unit_button = create_goto_button(
                ObjectType::UNIT, [this] { return entity->gates.at(combo_gate->get_active_key()).unit->uuid; });
        symbol_sel_box->pack_start(*goto_unit_button, false, false, 0);
    }

    {
        auto la = Gtk::manage(new Gtk::Label("Symbol"));
        la->get_style_context()->add_class("dim-label");
        symbol_sel_box->pack_start(*la, false, false, 0);
        la->set_margin_start(4);
    }
    combo_symbol = Gtk::manage(new GenericComboBox<UUID>);
    combo_symbol->get_cr_text().property_ellipsize() = Pango::ELLIPSIZE_END;
    combo_symbol->get_cr_text().property_width_chars() = 10;
    combo_symbol->signal_changed().connect(sigc::mem_fun(*this, &EntityPreview::handle_symbol_sel));
    symbol_sel_box->pack_start(*combo_symbol, true, true, 0);

    if (show_goto) {
        goto_symbol_button = create_goto_button(ObjectType::SYMBOL, [this] { return combo_symbol->get_active_key(); });
        symbol_sel_box->pack_start(*goto_symbol_button, false, false, 0);
    }

    pack_start(*symbol_sel_box, false, false, 0);

    {
        auto sep = Gtk::manage(new Gtk::Separator(Gtk::ORIENTATION_HORIZONTAL));
        sep->show();
        pack_start(*sep, false, false, 0);
    }

    canvas_symbol = Gtk::manage(new PreviewCanvas(pool, false));
    canvas_symbol->show();
    pack_start(*canvas_symbol, true, true, 0);


    clear();
}

void EntityPreview::load(const Entity *e)
{
    load(e, nullptr);
}

void EntityPreview::load(const Part *p)
{
    load(p->entity, p);
}

void EntityPreview::clear()
{
    load(nullptr, nullptr);
}

void EntityPreview::load(const Entity *e, const Part *p)
{
    entity = e;
    part = p;
    combo_gate->remove_all();

    for (auto it : goto_buttons) {
        it->set_sensitive(entity);
    }
    if (!entity) {
        canvas_symbol->clear();
        return;
    }

    std::vector<const Gate *> gates;
    gates.reserve(entity->gates.size());
    for (const auto &it : entity->gates) {
        gates.push_back(&it.second);
    }
    std::sort(gates.begin(), gates.end(),
              [](const auto *a, const auto *b) { return strcmp_natural(a->suffix, b->suffix) < 0; });
    if (goto_unit_button)
        goto_unit_button->set_sensitive(gates.size());

    for (const auto it : gates) {
        if (it->suffix.size())
            combo_gate->append(it->uuid, it->suffix + ": " + it->unit->name);
        else
            combo_gate->append(it->uuid, it->unit->name);
    }
    combo_gate->set_active(0);
}

void EntityPreview::handle_gate_sel()
{
    combo_symbol->remove_all();
    if (!entity)
        return;
    if (combo_gate->get_active_row_number() == -1)
        return;

    const Gate *gate = &entity->gates.at(combo_gate->get_active_key());
    auto unit = gate->unit;

    bool have_symbols = false;
    SQLite::Query q(pool.db, "SELECT uuid, name FROM symbols WHERE unit=? ORDER BY name");
    q.bind(1, unit->uuid);
    while (q.step()) {
        UUID uu = q.get<std::string>(0);
        std::string name = q.get<std::string>(1);
        combo_symbol->append(uu, name);
        have_symbols = true;
    }
    if (goto_symbol_button)
        goto_symbol_button->set_sensitive(have_symbols);

    combo_symbol->set_active(0);
}

void EntityPreview::handle_symbol_sel()
{
    canvas_symbol->clear();
    if (!entity)
        return;
    if (combo_symbol->get_active_row_number() == -1)
        return;

    canvas_symbol->load_symbol(combo_symbol->get_active_key(), Placement(), true, part ? part->uuid : UUID(),
                               combo_gate->get_active_key());
}
} // namespace horizon
