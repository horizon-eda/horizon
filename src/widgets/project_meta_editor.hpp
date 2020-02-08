#pragma once
#include <gtkmm.h>

namespace horizon {

class ProjectMetaEditor : public Gtk::Grid {
public:
    ProjectMetaEditor(std::map<std::string, std::string> &v);

private:
    Gtk::Entry *add_editor(const std::string &title, const std::string &key);
    std::map<std::string, std::string> &values;
    int top = 0;
    Gtk::Box *custom_box = nullptr;
};
} // namespace horizon
