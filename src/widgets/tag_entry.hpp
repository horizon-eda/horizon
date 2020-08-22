#pragma once
#include <gtkmm.h>
#include <map>
#include <set>
#include "util/changeable.hpp"
#include "common/common.hpp"

namespace horizon {
class TagEntry : public Gtk::Box, public Changeable {
public:
    class TagPopover;
    class TagLabel;
    friend TagPopover;
    friend TagLabel;
    TagEntry(class IPool &p, ObjectType t, bool edit_mode = false);
    std::set<std::string> get_tags() const;
    void set_tags(const std::set<std::string> &tags);
    void clear();

private:
    class IPool &pool;
    const ObjectType type;
    const bool edit_mode;
    Gtk::MenuButton *add_button = nullptr;
    Gtk::Box *box = nullptr;

    void add_tag(const std::string &t);
    void remove_tag(const std::string &t);
    std::map<std::string, class TagLabel *> label_widgets;
    void update_add_button_sensitivity();
};
} // namespace horizon
