#pragma once
#include <gtkmm.h>
#include "util/changeable.hpp"

namespace horizon {
class ColumnChooser : public Gtk::Grid, public Changeable {
public:
    class IAdapter {
    public:
        virtual std::string get_column_name(int col) const = 0;
        virtual std::map<int, std::string> get_column_names() const = 0;
        virtual std::vector<int> get_columns() const = 0;
        virtual bool has_column(int col) const = 0;
        virtual void include_column(int col) = 0;
        virtual void exclude_column(int col) = 0;
        virtual void move_column(int col, bool up) = 0;
    };

    ColumnChooser(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x, IAdapter &adap);
    static ColumnChooser *create(IAdapter &adap);


    template <typename T> class Adapter : public IAdapter {
    public:
        Adapter(std::vector<T> &cols) : columns(cols)
        {
        }

        bool has_column(int col) const override
        {
            return std::count(columns.begin(), columns.end(), static_cast<T>(col));
        };

        void include_column(int col) override
        {
            auto c = std::count(columns.begin(), columns.end(), static_cast<T>(col));

            if (c == 0)
                columns.push_back(static_cast<T>(col));
        }

        void exclude_column(int col) override
        {
            auto c = std::count(columns.begin(), columns.end(), static_cast<T>(col));
            if (c != 0)
                columns.erase(std::remove(columns.begin(), columns.end(), static_cast<T>(col)), columns.end());
        }

        void move_column(int col, bool up) override
        {
            auto it = std::find(columns.begin(), columns.end(), static_cast<T>(col));
            if (it == columns.end())
                return;

            if (up && it == columns.begin()) // already at the top
                return;

            if (!up && it == columns.end() - 1) // already at the bottom
                return;

            auto it_other = it + (up ? -1 : +1);

            std::swap(*it_other, *it);
        }

        std::vector<int> get_columns() const override
        {
            std::vector<int> r;
            std::transform(columns.begin(), columns.end(), std::back_inserter(r),
                           [](auto x) { return static_cast<int>(x); });
            return r;
        }

    private:
        std::vector<T> &columns;
    };

private:
    IAdapter &adapter;

    class ListColumns : public Gtk::TreeModelColumnRecord {
    public:
        ListColumns()
        {
            Gtk::TreeModelColumnRecord::add(name);
            Gtk::TreeModelColumnRecord::add(column);
        }
        Gtk::TreeModelColumn<int> column;
        Gtk::TreeModelColumn<Glib::ustring> name;
    };
    ListColumns list_columns;

    Glib::RefPtr<Gtk::ListStore> columns_store;
    Glib::RefPtr<Gtk::TreeModelFilter> columns_available;

    Glib::RefPtr<Gtk::ListStore> columns_store_included;

    Gtk::TreeView *cols_available_tv = nullptr;
    Gtk::TreeView *cols_included_tv = nullptr;
    Gtk::TreeView *preview_tv = nullptr;

    Gtk::Button *col_inc_button = nullptr;
    Gtk::Button *col_excl_button = nullptr;
    Gtk::Button *col_up_button = nullptr;
    Gtk::Button *col_down_button = nullptr;

    void incl_excl_col(bool incl);
    void up_down_col(bool up);
    void update_incl_excl_sensitivity();
    void update_cols_included();
};
} // namespace horizon
