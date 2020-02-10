#pragma once
#include <gtkmm.h>
#include "util/changeable.hpp"

namespace horizon {

class ProjectMetaEditor : public Gtk::Grid, public Changeable {
public:
    ProjectMetaEditor(std::map<std::string, std::string> &v);
    void clear();
    void preset();

private:
    Gtk::Entry *add_editor(const std::string &title, const std::string &descr, const std::string &key);
    class CustomFieldEditor *add_custom_editor(const std::string &key);
    std::map<std::string, Gtk::Entry *> entries;
    std::set<class CustomFieldEditor *> custom_editors;
    std::map<std::string, std::string> &values;
    int top = 0;
    Gtk::Box *custom_box = nullptr;
};
} // namespace horizon
