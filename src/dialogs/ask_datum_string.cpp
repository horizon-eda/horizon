#include "ask_datum_string.hpp"
#include "util/gtk_util.hpp"
#include <iostream>

namespace horizon {


AskDatumStringDialog::AskDatumStringDialog(Gtk::Window *parent, const std::string &label, TextEditor::Lines mode)
    : Gtk::Dialog("Enter Datum", *parent, Gtk::DialogFlags::DIALOG_MODAL | Gtk::DialogFlags::DIALOG_USE_HEADER_BAR)
{
    add_button("Cancel", Gtk::ResponseType::RESPONSE_CANCEL);
    add_button("OK", Gtk::ResponseType::RESPONSE_OK);

    set_default_response(Gtk::ResponseType::RESPONSE_OK);


    auto box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL, 4));
    box->property_margin() = 6;
    {
        auto la = Gtk::manage(new Gtk::Label(label));
        la->set_halign(Gtk::ALIGN_START);
        box->pack_start(*la, false, false, 0);
    }

    editor = Gtk::manage(new TextEditor(mode));
    editor->signal_activate().connect([this] { response(Gtk::RESPONSE_OK); });
    box->pack_start(*editor, true, true, 0);
    get_content_area()->set_border_width(0);
    get_content_area()->pack_start(*box, true, true, 0);
    show_all();
}

void AskDatumStringDialog::set_text(const std::string &text)
{
    editor->set_text(text, TextEditor::Select::YES);
}
std::string AskDatumStringDialog::get_text() const
{
    return editor->get_text();
}

} // namespace horizon
