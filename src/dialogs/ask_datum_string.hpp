#pragma once
#include <gtkmm.h>
#include <array>
#include <set>
namespace horizon {


class AskDatumStringDialog : public Gtk::Dialog {
public:
    AskDatumStringDialog(Gtk::Window *parent, const std::string &label, bool multiline = false);
    void set_text(const std::string &text);
    std::string get_text();

private:
    Gtk::Entry *entry = nullptr;
    Gtk::TextView *view = nullptr;
    Gtk::Stack *stack = nullptr;
};
} // namespace horizon
