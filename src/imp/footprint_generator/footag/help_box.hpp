#pragma once
#include <gtkmm.h>
#include <map>

namespace horizon {
class FootagHelpBox : public Gtk::ScrolledWindow {
public:
    FootagHelpBox(void);

    void showit(const std::string &str);

private:
    Gtk::Label *label = nullptr;
};

} // namespace horizon
