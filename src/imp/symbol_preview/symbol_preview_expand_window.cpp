#include "symbol_preview_expand_window.hpp"
#include "preview_box.hpp"
#include "util/gtk_util.hpp"
#include "canvas/canvas_gl.hpp"
#include "pool/symbol.hpp"

namespace horizon {
SymbolPreviewExpandWindow::SymbolPreviewExpandWindow(Gtk::Window *parent, const Symbol &sy)
    : Gtk::Window(), sym(sy), state_store(this, "imp-symbol-preview-expand")
{
    set_transient_for(*parent);
    set_type_hint(Gdk::WINDOW_TYPE_HINT_DIALOG);
    auto hb = Gtk::manage(new Gtk::HeaderBar());
    hb->set_show_close_button(true);
    hb->set_title("Expanded symbol preview");

    auto fit_button = Gtk::manage(new Gtk::Button("Fit view"));
    fit_button->signal_clicked().connect(sigc::mem_fun(*this, &SymbolPreviewExpandWindow::zoom_to_fit));
    hb->pack_start(*fit_button);


    if (!state_store.get_default_set())
        set_default_size(300, 300);

    set_titlebar(*hb);
    hb->show_all();

    auto overlay = Gtk::manage(new Gtk::Overlay);

    box2 = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 5));
    box2->set_halign(Gtk::ALIGN_START);
    box2->set_valign(Gtk::ALIGN_START);
    box2->get_style_context()->add_class("imp-symbol-expanded-preview");
    box2->get_style_context()->add_class("background");
    overlay->add_overlay(*box2);
    {
        auto la = Gtk::manage(new Gtk::Label("Expand"));
        la->get_style_context()->add_class("dim-label");
        box2->pack_start(*la, false, false, 0);

        sp_expand = Gtk::manage(new Gtk::SpinButton);
        box2->pack_start(*sp_expand, false, false, 0);
        sp_expand->set_range(0, 10);
        sp_expand->set_value(3);
        sp_expand->set_increments(1, 1);
        sp_expand->signal_value_changed().connect(sigc::mem_fun(*this, &SymbolPreviewExpandWindow::update));
    }


    canvas = Gtk::manage(new CanvasGL());
    canvas->set_selection_allowed(false);
    overlay->add(*canvas);

    add(*overlay);
    overlay->show_all();
}

void SymbolPreviewExpandWindow::update()
{
    Symbol sym_expanded = sym;
    sym_expanded.can_expand = true;
    sym_expanded.apply_expand(sym, sp_expand->get_value_as_int());
    sym_expanded.expand(Symbol::PinDisplayMode::PRIMARY);
    canvas->update(sym_expanded, Placement(), Canvas::SymbolMode::SHEET);
}

void SymbolPreviewExpandWindow::set_canvas_appearance(const Appearance &a)
{
    canvas->set_appearance(a);
}

void SymbolPreviewExpandWindow::zoom_to_fit()
{

    int64_t pad = 1_mm;
    float header_height = box2->get_allocated_height();
    for (int i = 0; i < 3; i++) { // do multiple times to account for changing scale
        auto bb = canvas->get_bbox();
        auto [scale, offset] = canvas->get_scale_and_offset();
        bb.first.x -= pad;
        bb.first.y -= pad;

        bb.second.x += pad;
        bb.second.y += pad + header_height / scale;
        canvas->zoom_to_bbox(bb.first, bb.second);
    }
}

} // namespace horizon
