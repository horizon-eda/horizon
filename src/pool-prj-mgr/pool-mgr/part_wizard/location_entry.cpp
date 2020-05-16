#include "location_entry.hpp"
#include "util/util.hpp"

namespace horizon {

std::string LocationEntry::get_rel_filename(const std::string &s) const
{
    return Gio::File::create_for_path(relative_to)->get_relative_path(Gio::File::create_for_path(s));
}


void LocationEntry::set_filename(const std::string &s)
{
    if (relative_to.size()) {
        std::string rel = get_rel_filename(s);
        if (rel.size())
            entry->set_text(rel);
    }
    else
        entry->set_text(s);
}

std::string LocationEntry::get_filename()
{
    if (relative_to.size())
        return Glib::build_filename(relative_to, entry->get_text());
    else
        return entry->get_text();
}

LocationEntry::LocationEntry(const std::string &rel) : Gtk::Box(), relative_to(rel)
{
    get_style_context()->add_class("linked");
    entry = Gtk::manage(new Gtk::Entry());
    pack_start(*entry, true, true, 0);
    entry->show();

    entry->signal_changed().connect([this] { s_signal_changed.emit(); });

    auto button = Gtk::manage(new Gtk::Button("Browse..."));
    pack_start(*button, false, false, 0);
    button->signal_clicked().connect(sigc::mem_fun(*this, &LocationEntry::handle_button));
    button->show();
}

void LocationEntry::set_warning(const std::string &text)
{
    if (text.size()) {
        entry->set_icon_from_icon_name("dialog-warning-symbolic", Gtk::ENTRY_ICON_SECONDARY);
        entry->set_icon_tooltip_text(text, Gtk::ENTRY_ICON_SECONDARY);
    }
    else {
        entry->unset_icon(Gtk::ENTRY_ICON_SECONDARY);
    }
}

void LocationEntry::handle_button()
{
    auto top = dynamic_cast<Gtk::Window *>(get_ancestor(GTK_TYPE_WINDOW));

    GtkFileChooserNative *native =
            gtk_file_chooser_native_new("Save", top->gobj(), GTK_FILE_CHOOSER_ACTION_SAVE, "Set", "_Cancel");
    auto chooser = Glib::wrap(GTK_FILE_CHOOSER(native));
    chooser->set_do_overwrite_confirmation(true);
    chooser->set_filename(get_filename());
    chooser->set_current_name(Glib::path_get_basename(get_filename()));

    if (gtk_native_dialog_run(GTK_NATIVE_DIALOG(native)) == GTK_RESPONSE_ACCEPT) {
        auto filename = chooser->get_filename();
        if (relative_to.size()) {
            auto rel = get_rel_filename(filename);
            if (rel.size()) {
                entry->set_text(rel);
            }
            else {
                Gtk::MessageDialog md(*dynamic_cast<Gtk::Window *>(get_ancestor(GTK_TYPE_WINDOW)),
                                      "Incorrect file location", false /* use_markup */, Gtk::MESSAGE_ERROR,
                                      Gtk::BUTTONS_OK);
                md.set_secondary_text("File has to be in a (sub)directory of " + relative_to);
                md.run();
            }
        }
        else {
            entry->set_text(filename);
        }
    }
}

bool LocationEntry::check_ends_json(bool *v)
{
    bool r;
    std::string t = get_filename();
    if (!endswith(t, ".json")) {
        set_warning("Filename has to end in .json");
        r = false;
        if (v)
            *v = false;
    }
    else {
        set_warning("");
        r = true;
        if (v)
            *v = true;
    }
    return r;
}

} // namespace horizon
