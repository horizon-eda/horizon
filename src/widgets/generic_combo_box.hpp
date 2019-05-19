#pragma once
#include <gtkmm.h>

namespace horizon {
template <typename T> class GenericComboBox : public Gtk::ComboBox {
public:
    GenericComboBox() : Gtk::ComboBox()
    {
        store = Gtk::ListStore::create(list_columns);
        set_model(store);
        cr_text = Gtk::manage(new Gtk::CellRendererText);
        pack_start(*cr_text, true);
        add_attribute(cr_text->property_text(), list_columns.value);

        signal_changed().connect([this] {
            auto it = get_active();
            if (store->iter_is_valid(it)) {
                Gtk::TreeModel::Row row = *it;
                set_tooltip_text(row[list_columns.value]);
            }
            else {
                set_has_tooltip(false);
            }
        });
    }

    Gtk::CellRendererText &get_cr_text()
    {
        return *cr_text;
    }

    void set_active_key(const T &key)
    {
        for (const auto &it : store->children()) {
            Gtk::TreeModel::Row row = *it;
            if (row[list_columns.key] == key) {
                set_active(it);
                break;
            }
        }
    }

    const T get_active_key()
    {
        auto it = get_active();
        if (store->iter_is_valid(it)) {
            Gtk::TreeModel::Row row = *it;
            return row[list_columns.key];
        }
        else {
            return T();
        }
    }

    void remove_all()
    {
        store->clear();
    }

    void append(const T &key, const Glib::ustring &value)
    {
        Gtk::TreeModel::Row row = *store->append();
        row[list_columns.key] = key;
        row[list_columns.value] = value;
    }

private:
    class ListColumns : public Gtk::TreeModelColumnRecord {
    public:
        ListColumns()
        {
            Gtk::TreeModelColumnRecord::add(key);
            Gtk::TreeModelColumnRecord::add(value);
        }
        Gtk::TreeModelColumn<T> key;
        Gtk::TreeModelColumn<Glib::ustring> value;
    };
    ListColumns list_columns;

    Glib::RefPtr<Gtk::ListStore> store;
    Gtk::CellRendererText *cr_text = nullptr;
};
} // namespace horizon
