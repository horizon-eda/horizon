#include "parameter_window.hpp"
#include "widgets/parameter_set_editor.hpp"

namespace horizon {
ParameterWindow::ParameterWindow(Gtk::Window *p, std::string *ppc, ParameterSet *ps, ParameterSetEditor *editor)
    : Gtk::Window()
{
    set_transient_for(*p);
    set_type_hint(Gdk::WINDOW_TYPE_HINT_DIALOG);
    hb = Gtk::manage(new Gtk::HeaderBar());
    hb->set_show_close_button(true);
    hb->set_title("Parameters");

    apply_button = Gtk::manage(new Gtk::Button("Apply"));
    apply_button->get_style_context()->add_class("suggested-action");
    apply_button->signal_clicked().connect([this] { signal_apply().emit(); });
    hb->pack_start(*apply_button);

    set_default_size(800, 300);
    set_titlebar(*hb);
    hb->show_all();

    auto box2 = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL, 0)); // for info bar
    auto box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 0));

    auto tbox = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL, 0));
    extra_button_box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 10));
    extra_button_box->set_margin_start(10);
    extra_button_box->set_margin_end(10);
    extra_button_box->set_margin_top(10);
    extra_button_box->set_margin_bottom(10);

    tbox->pack_start(*extra_button_box, false, false, 0);
    {
        auto sep = Gtk::manage(new Gtk::Separator(Gtk::ORIENTATION_HORIZONTAL));
        tbox->pack_start(*sep, false, false, 0);
    }
    auto sc = Gtk::manage(new Gtk::ScrolledWindow());
    view = Gtk::manage(new Gsv::View());
    buffer = view->get_source_buffer();
    view->set_top_margin(5);
    view->set_bottom_margin(5);
    view->set_left_margin(5);
    view->set_right_margin(5);
    buffer->begin_not_undoable_action();
    buffer->set_text(*ppc);
    buffer->end_not_undoable_action();
    view->set_monospace(true);
    buffer->signal_changed().connect([ppc, this] {
        *ppc = buffer->get_text();
        s_signal_changed.emit();
    });
    sc->add(*view);
    tbox->pack_start(*sc, true, true, 0);
    box->pack_start(*tbox, true, true, 0);

    auto sep = Gtk::manage(new Gtk::Separator());
    box->pack_start(*sep, false, false, 0);

    if (editor)
        parameter_set_editor = Gtk::manage(editor);
    else
        parameter_set_editor = Gtk::manage(new ParameterSetEditor(ps, false));

    parameter_set_editor->signal_create_extra_widget().connect([this](ParameterID id) {
        auto w = Gtk::manage(new Gtk::Button);
        w->set_image_from_icon_name("insert-text-symbolic", Gtk::ICON_SIZE_BUTTON);
        w->set_tooltip_text("Insert into parameter program");
        w->signal_clicked().connect([this, id] { insert_parameter(id); });
        return w;
    });

    parameter_set_editor->populate();
    parameter_set_editor->signal_changed().connect([this] { s_signal_changed.emit(); });
    box->pack_start(*parameter_set_editor, false, false, 0);

    box2->pack_start(*box, true, true, 0);

    bar = Gtk::manage(new Gtk::InfoBar());
    bar_label = Gtk::manage(new Gtk::Label("fixme"));
    bar_label->set_xalign(0);
    dynamic_cast<Gtk::Box *>(bar->get_content_area())->pack_start(*bar_label, true, true, 0);
    box2->pack_start(*bar, false, false, 0);

    box2->show_all();
    add(*box2);
    bar->hide();
    extra_button_box->hide();
}

void ParameterWindow::set_subtitle(const std::string &t)
{
    hb->set_subtitle(t);
}

void ParameterWindow::set_can_apply(bool v)
{
    apply_button->set_sensitive(v);
}

void ParameterWindow::add_button(Gtk::Widget *button)
{
    extra_button_box->pack_start(*button, false, false, 0);
    extra_button_box->show_all();
}

void ParameterWindow::insert_text(const std::string &text)
{
    buffer->insert_at_cursor(text);
}

void ParameterWindow::insert_parameter(ParameterID id)
{
    insert_text("get-parameter [ " + parameter_id_to_string(id) + " ]\n");
}

void ParameterWindow::set_error_message(const std::string &s)
{
    if (s.size()) {
        bar->show();
        bar_label->set_markup(s);
        bar->set_size_request(0, 0);
        bar->set_size_request(-1, -1);
    }
    else {
        bar->hide();
    }
}
} // namespace horizon
