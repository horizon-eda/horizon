#include "symbol_preview_window.hpp"
#include "preview_box.hpp"
#include "util/gtk_util.hpp"

namespace horizon {
SymbolPreviewWindow::SymbolPreviewWindow(Gtk::Window *parent) : Gtk::Window(), state_store(this, "imp-symbol-preview")
{
    set_transient_for(*parent);
    set_type_hint(Gdk::WINDOW_TYPE_HINT_DIALOG);
    auto hb = Gtk::manage(new Gtk::HeaderBar());
    hb->set_show_close_button(true);
    hb->set_title("Symbol preview");

    auto fit_button = Gtk::manage(new Gtk::Button("Fit views"));
    fit_button->signal_clicked().connect([this] {
        for (auto &it : previews) {
            it.second->zoom_to_fit();
        }
    });
    hb->pack_start(*fit_button);

    if (!state_store.get_default_set())
        set_default_size(800, 300);

    set_titlebar(*hb);
    hb->show_all();

    auto box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL, 10));

    bar = Gtk::manage(new Gtk::InfoBar());
    bar->set_message_type(Gtk::MESSAGE_WARNING);
    {
        auto la = Gtk::manage(
                new Gtk::Label("Don't apply orientation-specific texts to this symbol as it has more than 4 pins. Only "
                               "use orientation-specific placements for small symbols that are "
                               "expected to be used in multiple orientations."));
        la->show();
        dynamic_cast<Gtk::Box &>(*bar->get_content_area()).pack_start(*la, false, false, 0);
    }
    box->pack_start(*bar, false, false, 0);

    {
        auto la = Gtk::manage(new Gtk::Label());
        la->set_markup(
                "<i>A symbol can have orientation-specific text placements. First click \"Clear\" for all "
                "orientations. Then for each orientation position the texts as desired in the main window and click "
                "\"Set\" in this window.</i>");
        la->set_line_wrap_mode(Pango::WRAP_WORD);
        la->set_line_wrap(true);
        la->set_margin_start(10);
        la->set_margin_end(10);
        box->pack_start(*la, false, false);
    }


    auto grid = Gtk::manage(new Gtk::Grid());
    grid->set_row_spacing(10);
    grid->set_column_spacing(10);
    grid->set_margin_start(10);
    grid->set_margin_end(10);
    grid->set_margin_bottom(10);
    grid->set_row_homogeneous(true);
    grid->set_column_homogeneous(true);

    for (int left = 0; left < 4; left++) {
        for (int top = 0; top < 2; top++) {
            std::pair<int, bool> view(left * 90, top);
            auto pv = Gtk::manage(new SymbolPreviewBox(view));
            pv->signal_changed().connect([this] { s_signal_changed.emit(); });
            pv->signal_load().connect([this](auto a, auto b) { s_signal_load.emit(a, b); });
            previews[view] = pv;
            grid->attach(*pv, left, top, 1, 1);
            pv->show();
        }
    }

    box->pack_start(*grid, true, true, 0);

    add(*box);
    box->show_all();
}

void SymbolPreviewWindow::update(const class Symbol &sym)
{
    if (sym.unit->pins.size() > 4)
        info_bar_show(bar);
    else
        info_bar_hide(bar);
    for (auto &it : previews) {
        it.second->update(sym);
    }
}

std::map<std::tuple<int, bool, UUID>, Placement> SymbolPreviewWindow::get_text_placements() const
{
    std::map<std::tuple<int, bool, UUID>, Placement> r;
    for (const auto &it : previews) {
        auto m = it.second->get_text_placements();
        r.insert(m.begin(), m.end());
    }
    return r;
}

void SymbolPreviewWindow::set_text_placements(const std::map<std::tuple<int, bool, UUID>, Placement> &p)
{
    for (auto &it : previews) {
        it.second->set_text_placements(p);
    }
}

void SymbolPreviewWindow::set_canvas_appearance(const Appearance &a)
{
    for (auto &it : previews) {
        it.second->set_canvas_appearance(a);
    }
}

void SymbolPreviewWindow::set_can_load(bool v)
{
    for (auto &it : previews) {
        it.second->set_can_load(v);
    }
}
} // namespace horizon
