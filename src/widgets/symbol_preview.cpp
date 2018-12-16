#include "symbol_preview.hpp"
#include "pool/symbol.hpp"
#include "pool/pool.hpp"
#include "canvas/canvas.hpp"
#include "util/util.hpp"
#include "util/sqlite.hpp"
#include "common/object_descr.hpp"
#include "preview_canvas.hpp"

namespace horizon {
SymbolPreview::SymbolPreview(class Pool &p) : Gtk::Box(Gtk::ORIENTATION_VERTICAL, 0), pool(p)
{

    canvas_symbol = Gtk::manage(new PreviewCanvas(pool, false));

    auto top_box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 4));
    top_box->property_margin() = 8;
    {
        auto box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 0));
        box->set_homogeneous(true);
        box->get_style_context()->add_class("linked");
        rb_normal = Gtk::manage(new Gtk::RadioButton("Normal"));
        rb_normal->set_mode(false);
        rb_normal->signal_toggled().connect([this] { update(); });
        box->pack_start(*rb_normal, false, true, 0);

        rb_mirrored = Gtk::manage(new Gtk::RadioButton("Mirrored"));
        rb_mirrored->set_mode(false);
        rb_mirrored->join_group(*rb_normal);
        rb_mirrored->signal_toggled().connect([this] { update(); });
        box->pack_start(*rb_mirrored, false, true, 0);

        top_box->pack_start(*box, false, false, 0);
    }

    {
        auto box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 0));
        box->set_homogeneous(true);
        box->get_style_context()->add_class("linked");
        for (int i = 0; i < 4; i++) {
            auto rb = Gtk::manage(new Gtk::RadioButton(std::to_string(i * 90) + "°"));
            rb_angles[i] = rb;
            rb->set_mode(false);
            box->pack_start(*rb, false, true, 0);
            if (i > 0) {
                rb->join_group(*rb_angles[0]);
            }
            rb->signal_toggled().connect([this] { update(); });
        }
        box->set_margin_start(4);
        top_box->pack_start(*box, false, false, 0);
    }

    {
        auto la = Gtk::manage(new Gtk::Label("Unit"));
        la->set_margin_start(4);
        la->get_style_context()->add_class("dim-label");
        top_box->pack_start(*la, false, false, 0);

        unit_label = Gtk::manage(new Gtk::Label());
        unit_label->set_use_markup(true);
        unit_label->set_track_visited_links(false);
        unit_label->signal_activate_link().connect(
                [this](const std::string &url) {
                    s_signal_goto.emit(ObjectType::UNIT, UUID(url));
                    return true;
                },
                false);
        top_box->pack_start(*unit_label, false, false, 0);
    }


    pack_start(*top_box, false, false, 0);

    {
        auto sep = Gtk::manage(new Gtk::Separator(Gtk::ORIENTATION_HORIZONTAL));
        sep->show();
        pack_start(*sep, false, false, 0);
    }


    canvas_symbol->show();
    pack_start(*canvas_symbol, true, true, 0);


    load(UUID());
}

void SymbolPreview::load(const UUID &uu)
{
    symbol = uu;
    if (uu) {
        auto sym = pool.get_symbol(uu);
        unit_label->set_markup("<a href=\"" + (std::string)sym->unit->uuid + "\">"
                               + Glib::Markup::escape_text(sym->unit->name) + "</a>");
    }
    else {
        unit_label->set_text("");
    }
    update(true);
}

void SymbolPreview::update(bool fit)
{
    canvas_symbol->clear();
    if (!symbol)
        return;
    Placement tr;
    tr.mirror = rb_mirrored->get_active();
    int angle = 0;
    for (auto &rb : rb_angles) {
        if (rb->get_active())
            break;
        angle += 16384;
    }
    tr.set_angle(angle);

    canvas_symbol->load(ObjectType::SYMBOL, symbol, tr, fit);
}
} // namespace horizon
