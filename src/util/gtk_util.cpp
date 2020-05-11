#include "gtk_util.hpp"
#include "widgets/spin_button_dim.hpp"
#include "str_util.hpp"
#include "common/common.hpp"

namespace horizon {
void bind_widget(Gtk::Switch *sw, bool &v)
{
    sw->set_active(v);
    sw->property_active().signal_changed().connect([sw, &v] { v = sw->get_active(); });
}
void bind_widget(Gtk::CheckButton *cb, bool &v)
{
    cb->set_active(v);
    cb->signal_toggled().connect([cb, &v] { v = cb->get_active(); });
}
void bind_widget(SpinButtonDim *sp, int64_t &v)
{
    sp->set_value(v);
    sp->signal_value_changed().connect([sp, &v] { v = sp->get_value_as_int(); });
}
void bind_widget(SpinButtonDim *sp, uint64_t &v)
{
    sp->set_value(v);
    sp->signal_value_changed().connect([sp, &v] { v = sp->get_value_as_int(); });
}

void bind_widget(Gtk::SpinButton *sp, int &v)
{
    sp->set_value(v);
    sp->signal_value_changed().connect([sp, &v] { v = sp->get_value_as_int(); });
}

void bind_widget(Gtk::Scale *sc, float &v)
{
    sc->set_value(v);
    sc->signal_value_changed().connect([sc, &v] { v = sc->get_value(); });
}

void bind_widget(Gtk::Entry *en, std::string &v, std::function<void(std::string &v)> extra_cb)
{
    en->set_text(v);
    en->signal_changed().connect([en, &v, extra_cb] {
        v = en->get_text();
        if (extra_cb) {
            extra_cb(v);
        }
    });
}

void bind_widget(Gtk::ColorButton *color_button, Color &color, std::function<void(const Color &c)> extra_cb)
{
    Gdk::RGBA r;
    r.set_rgba(color.r, color.g, color.b);
    color_button->set_rgba(r);

    color_button->property_color().signal_changed().connect([color_button, &color, extra_cb] {
        auto co = color_button->get_rgba();
        color.r = co.get_red();
        color.g = co.get_green();
        color.b = co.get_blue();
        if (extra_cb)
            extra_cb(color);
    });
}

Gtk::Label *grid_attach_label_and_widget(Gtk::Grid *gr, const std::string &label, Gtk::Widget *w, int &top)
{
    auto la = Gtk::manage(new Gtk::Label(label));
    la->get_style_context()->add_class("dim-label");
    la->set_xalign(1);
    la->show();
    gr->attach(*la, 0, top, 1, 1);
    w->show();
    gr->attach(*w, 1, top, 1, 1);
    top++;
    return la;
}

void tree_view_scroll_to_selection(Gtk::TreeView *view)
{
    auto rows = view->get_selection()->get_selected_rows();
    if (rows.size()) {
        view->scroll_to_cell(rows[0], *view->get_column(0));
    }
}

void entry_set_warning(Gtk::Entry *e, const std::string &text)
{
    if (text.size()) {
        e->set_icon_from_icon_name("dialog-warning-symbolic", Gtk::ENTRY_ICON_SECONDARY);
        e->set_icon_tooltip_text(text, Gtk::ENTRY_ICON_SECONDARY);
    }
    else {
        e->unset_icon(Gtk::ENTRY_ICON_SECONDARY);
    }
}

void header_func_separator(Gtk::ListBoxRow *row, Gtk::ListBoxRow *before)
{
    if (before && !row->get_header()) {
        auto ret = Gtk::manage(new Gtk::Separator);
        row->set_header(*ret);
    }
}

static bool needs_trim(const std::string &s)
{
    return s.size() && (isspace(s.front()) || isspace(s.back()));
}

void entry_add_sanitizer(Gtk::Entry *entry)
{
    entry->signal_icon_press().connect([entry](Gtk::EntryIconPosition pos, const GdkEventButton *ev) {
        if (pos == Gtk::ENTRY_ICON_SECONDARY) {
            std::string txt = entry->get_text();
            txt.erase(std::remove(txt.begin(), txt.end(), '\r'), txt.end());
            for (auto &it : txt) {
                if (it == '\n')
                    it = ' ';
            }
            trim(txt);
            entry->set_text(txt);
        }
    });
    entry->signal_changed().connect([entry] {
        auto txt = entry->get_text();
        bool has_line_breaks = std::count(txt.begin(), txt.end(), '\n');
        bool has_whitespace = needs_trim(txt);
        if (has_line_breaks || has_whitespace) {
            std::string warning;
            if (has_line_breaks) {
                warning += "Contains line breaks\n";
            }
            if (has_whitespace) {
                warning += "Has trailing/leading whitespace\n";
            }
            warning += "Click to fix";
            entry->set_icon_tooltip_text(warning, Gtk::ENTRY_ICON_SECONDARY);
            entry->set_icon_from_icon_name("dialog-warning-symbolic", Gtk::ENTRY_ICON_SECONDARY);
        }
        else {
            entry->unset_icon(Gtk::ENTRY_ICON_SECONDARY);
        }
    });
}

void label_set_tnum(Gtk::Label *la)
{
    auto attributes_list = pango_attr_list_new();
    auto attribute_font_features = pango_attr_font_features_new("tnum 1");
    pango_attr_list_insert(attributes_list, attribute_font_features);
    gtk_label_set_attributes(la->gobj(), attributes_list);
    pango_attr_list_unref(attributes_list);
}

void entry_set_tnum(Gtk::Entry &en)
{
    auto attributes_list = pango_attr_list_new();
    auto attribute_font_features = pango_attr_font_features_new("tnum 1");
    pango_attr_list_insert(attributes_list, attribute_font_features);
    gtk_entry_set_attributes(GTK_ENTRY(en.gobj()), attributes_list);
    pango_attr_list_unref(attributes_list);
}

void info_bar_show(Gtk::InfoBar *bar)
{
#if GTK_CHECK_VERSION(3, 24, 10)
    gtk_info_bar_set_revealed(bar->gobj(), true);
#else
    bar->show();
#endif
}

void info_bar_hide(Gtk::InfoBar *bar)
{
#if GTK_CHECK_VERSION(3, 24, 10)
    gtk_info_bar_set_revealed(bar->gobj(), false);
#else
    bar->hide();
#endif
}

Gtk::Box *make_boolean_ganged_switch(bool &v, const std::string &label_false, const std::string &label_true,
                                     std::function<void(bool v)> extra_cb)
{
    auto box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL));
    box->set_homogeneous(true);
    box->get_style_context()->add_class("linked");
    auto b1 = Gtk::manage(new Gtk::RadioButton(label_false));
    b1->set_mode(false);
    box->pack_start(*b1, true, true, 0);

    auto b2 = Gtk::manage(new Gtk::RadioButton(label_true));
    b2->set_mode(false);
    b2->join_group(*b1);
    box->pack_start(*b2, true, true, 0);

    b2->set_active(v);
    b2->signal_toggled().connect([b2, &v, extra_cb] {
        v = b2->get_active();
        if (extra_cb)
            extra_cb(v);
    });
    box->show_all();
    return box;
}

void make_label_small(Gtk::Label *la)
{
    auto attributes_list = pango_attr_list_new();
    auto attribute_scale = pango_attr_scale_new(.833);
    pango_attr_list_insert(attributes_list, attribute_scale);
    gtk_label_set_attributes(la->gobj(), attributes_list);
    pango_attr_list_unref(attributes_list);
}

std::string make_link_markup(const std::string &href, const std::string &label)
{
    return "<a href=\"" + Glib::Markup::escape_text(href) + "\" title=\""
           + Glib::Markup::escape_text(Glib::Markup::escape_text(href)) + "\">" + Glib::Markup::escape_text(label)
           + "</a>";
}

Gtk::TreeViewColumn *tree_view_append_column_ellipsis(Gtk::TreeView *view, const std::string &name,
                                                      const Gtk::TreeModelColumnBase &column,
                                                      Pango::EllipsizeMode ellipsize)
{
    auto cr = Gtk::manage(new Gtk::CellRendererText());
    auto tvc = Gtk::manage(new Gtk::TreeViewColumn(name, *cr));
    tvc->add_attribute(cr->property_text(), column);
    cr->property_ellipsize() = ellipsize;
    view->append_column(*tvc);
    return tvc;
}

void install_esc_to_close(Gtk::Window &win)
{
    win.signal_key_press_event().connect([&win](GdkEventKey *ev) {
        if (ev->keyval == GDK_KEY_Escape) {
            win.close();
            return true;
        }
        else {
            return false;
        }
    });
}

void widget_set_insensitive_tooltip(Gtk::Widget &w, const std::string &txt)
{
    if (txt.size()) {
        w.set_tooltip_text(txt);
        w.set_has_tooltip(true);
        w.set_sensitive(false);
    }
    else {
        w.set_has_tooltip(false);
        w.set_sensitive(true);
    }
}

/*
 * adapted from glade:
 * https://gitlab.gnome.org/GNOME/glade/blob/master/gladeui/glade-utils.c#L2115
 */

/* Use this to disable scroll events on property editors,
 * we dont want them handling scroll because they are inside
 * a scrolled window and interrupt workflow causing unexpected
 * results when scrolled.
 */
static gint abort_scroll_events(GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
    GtkWidget *parent = gtk_widget_get_parent(widget);

    /* Removing the events from the mask doesnt work for
     * stubborn combo boxes which call gtk_widget_add_events()
     * in it's gtk_combo_box_init() - so handle the event and propagate
     * it up the tree so the scrollwindow still handles the scroll event.
     */
    gtk_propagate_event(parent, event);

    return TRUE;
}

static void util_remove_scroll_events(GtkWidget *widget)
{
    gint events = gtk_widget_get_events(widget);

    events &= ~(GDK_SCROLL_MASK | GDK_SMOOTH_SCROLL_MASK);
    gtk_widget_set_events(widget, events);

    g_signal_connect(G_OBJECT(widget), "scroll-event", G_CALLBACK(abort_scroll_events), NULL);
}


void widget_remove_scroll_events(Gtk::Widget &widget)
{
    util_remove_scroll_events(widget.gobj());
}

} // namespace horizon
