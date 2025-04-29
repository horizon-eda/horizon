#pragma once
#include <gtkmm.h>
#include "common/common.hpp"
#include "editor_base.hpp"
#include "util/sort_helper.hpp"
#include "pool/entity.hpp"

namespace horizon {

class EntityEditor : public Gtk::Box, public PoolEditorBase {
    friend class GateEditor;

public:
    EntityEditor(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x, const std::string &filename,
                 class IPool &p);
    static EntityEditor *create(const std::string &filename, class IPool &p);
    void reload() override;

    void save_as(const std::string &fn) override;
    std::string get_name() const override;
    const UUID &get_uuid() const override;
    RulesCheckResult run_checks() const override;
    const FileVersion &get_version() const override;
    ObjectType get_type() const override;

    virtual ~EntityEditor() {};

private:
    class Entity entity;

    void load();

    Gtk::Entry *name_entry = nullptr;
    Gtk::Entry *manufacturer_entry = nullptr;
    Gtk::Entry *prefix_entry = nullptr;
    class TagEntry *tag_entry = nullptr;

    Gtk::ListBox *gates_listbox = nullptr;
    Gtk::ToolButton *add_button = nullptr;
    Gtk::ToolButton *delete_button = nullptr;

    Glib::RefPtr<Gtk::SizeGroup> sg_name;
    Glib::RefPtr<Gtk::SizeGroup> sg_suffix;
    Glib::RefPtr<Gtk::SizeGroup> sg_swap_group;
    Glib::RefPtr<Gtk::SizeGroup> sg_unit;

    void handle_add();
    void handle_delete();

    void sort();
    SortHelper sort_helper;

    void bind_entry(Gtk::Entry *e, std::string &s);

    std::unique_ptr<HistoryManager::HistoryItem> make_history_item(const std::string &comment) override;
    void history_load(const HistoryManager::HistoryItem &it) override;
};
} // namespace horizon
