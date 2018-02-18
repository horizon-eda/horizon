#pragma once
#include <gtkmm.h>
#include "action.hpp"

namespace horizon {
class KeySequenceDialog : public Gtk::Dialog {
public:
    KeySequenceDialog(Gtk::Window *parent);
    void add_sequence(const std::vector<KeySequence2> &seqs, const std::string &label);
    void add_sequence(const std::string &seq, const std::string &label);
    void clear();

private:
    Gtk::ListBox *lb;
    Glib::RefPtr<Gtk::SizeGroup> sg;
};
} // namespace horizon
