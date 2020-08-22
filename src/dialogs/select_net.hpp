#pragma once
#include <gtkmm.h>
#include <array>
#include <set>
#include "util/uuid.hpp"
namespace horizon {


class SelectNetDialog : public Gtk::Dialog {
public:
    SelectNetDialog(Gtk::Window *parent, const class Block &b, const std::string &ti);
    bool valid = false;
    UUID net;
    class NetSelector *net_selector;

private:
    void ok_clicked();
    void net_selected(const UUID &uu);
};
} // namespace horizon
