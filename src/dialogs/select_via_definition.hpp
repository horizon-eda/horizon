#pragma once
#include <gtkmm.h>
#include <array>
#include <set>
#include "util/uuid.hpp"
namespace horizon {


class SelectViaDefinitionDialog : public Gtk::Dialog {
public:
    SelectViaDefinitionDialog(Gtk::Window *parent, const class RuleViaDefinitions &defs,
                              const class LayerProvider &lprv, class IPool &pool);
    UUID selected_uuid;
    bool valid = false;
    // virtual ~MainWindow();
private:
    class ListColumns : public Gtk::TreeModelColumnRecord {
    public:
        ListColumns()
        {
            Gtk::TreeModelColumnRecord::add(name);
            Gtk::TreeModelColumnRecord::add(via_def);
        }
        Gtk::TreeModelColumn<Glib::ustring> name;
        Gtk::TreeModelColumn<const class ViaDefinition *> via_def;
    };
    ListColumns list_columns;
    const class LayerProvider &layer_provider;
    class IPool &pool;

    Gtk::TreeView *view;
    Glib::RefPtr<Gtk::ListStore> store;

    void ok_clicked();
    void row_activated(const Gtk::TreeModel::Path &path, Gtk::TreeViewColumn *column);
};
} // namespace horizon
