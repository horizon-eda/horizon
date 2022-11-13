#pragma once
#include <gtkmm.h>
#include "common/common.hpp"
#include "editor_base.hpp"
#include "util/sort_helper.hpp"
#include "pool/unit.hpp"
#include "rules/rules.hpp"

namespace horizon {

class UnitEditor : public Gtk::Box, public PoolEditorBase {
    friend class PinEditor;

public:
    UnitEditor(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x, const std::string &filename,
               class IPool &p);
    static UnitEditor *create(const std::string &filename, class IPool &p);
    void select(const ItemSet &items) override;

    void save_as(const std::string &fn) override;
    std::string get_name() const override;
    const UUID &get_uuid() const override;
    RulesCheckResult run_checks() const override;
    const FileVersion &get_version() const override;
    unsigned int get_required_version() const override;
    ObjectType get_type() const override;

    virtual ~UnitEditor(){};

private:
    Unit unit;
    Gtk::Entry *name_entry = nullptr;
    Gtk::Entry *manufacturer_entry = nullptr;
    Gtk::ListBox *pins_listbox = nullptr;
    Gtk::Button *add_button = nullptr;
    Gtk::Button *delete_button = nullptr;
    Gtk::CheckButton *cross_probing_cb = nullptr;

    Glib::RefPtr<Gtk::SizeGroup> sg_direction;
    Glib::RefPtr<Gtk::SizeGroup> sg_name;
    Glib::RefPtr<Gtk::SizeGroup> sg_names;

    void handle_add();
    void handle_delete();
    void sort();
    void handle_activate(class PinEditor *ed);

    SortHelper sort_helper;
};
} // namespace horizon
