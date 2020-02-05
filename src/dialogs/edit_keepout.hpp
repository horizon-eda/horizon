#pragma once
#include <gtkmm.h>
#include <array>
#include <set>
#include "common/common.hpp"
#include "util/uuid.hpp"
namespace horizon {
class EditKeepoutDialog : public Gtk::Dialog {
public:
    EditKeepoutDialog(Gtk::Window *parent, class Keepout *k, class IDocument *c, bool add_mode);

private:
    class Keepout *keepout = nullptr;
};
} // namespace horizon
