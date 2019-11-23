#include "autosave_recovery_dialog.hpp"
#include "util/gtk_util.hpp"

namespace horizon {
AutosaveRecoveryDialog::AutosaveRecoveryDialog(Gtk::Window *parent)
    : Gtk::MessageDialog(*parent, "Autosave recovery", false, Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_NONE)
{
    add_button("Ok", Gtk::RESPONSE_OK);

    auto box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL, 10));

    auto rb = Gtk::manage(
            new Gtk::RadioButton("Use autosaved file.\nThis will replace the original file with autosaved one.\n"
                                 "The original file will get renamed beforehand."));
    auto rb2 = Gtk::manage(new Gtk::RadioButton(
            "Keep autosaved file and use last saved version.\nYou will get the same prompt the next time."));
    auto rb3 = Gtk::manage(new Gtk::RadioButton("Delete autosaved file.\nPermanently delete the autosaved file."));
    rb2->join_group(*rb);
    rb3->join_group(*rb);
    std::map<Result, Gtk::RadioButton *> buttons = {
            {Result::USE, rb},
            {Result::KEEP, rb2},
            {Result::DELETE, rb3},
    };
    bind_widget<Result>(buttons, result);

    box->pack_start(*rb, false, false, 0);
    box->pack_start(*rb2, false, false, 0);
    box->pack_start(*rb3, false, false, 0);
    box->property_margin() = 10;

    get_content_area()->pack_start(*box, true, true, 0);
    get_content_area()->show_all();
    get_content_area()->set_spacing(0);
}
} // namespace horizon
