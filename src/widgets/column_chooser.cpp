#include "column_chooser.hpp"

namespace horizon {
ColumnChooser *ColumnChooser::create(IAdapter &adap)
{
    ColumnChooser *w;
    Glib::RefPtr<Gtk::Builder> x = Gtk::Builder::create();
    x->add_from_resource("/org/horizon-eda/horizon/widgets/column_chooser.ui");
    x->get_widget_derived("grid", w, adap);
    w->reference();
    return w;
}


#define GET_WIDGET(name)                                                                                               \
    do {                                                                                                               \
        x->get_widget(#name, name);                                                                                    \
    } while (0)

ColumnChooser::ColumnChooser(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x, IAdapter &adap)
    : Gtk::Grid(cobject), adapter(adap)
{
    GET_WIDGET(cols_available_tv);
    GET_WIDGET(cols_included_tv);
    GET_WIDGET(col_inc_button);
    GET_WIDGET(col_excl_button);
    GET_WIDGET(col_up_button);
    GET_WIDGET(col_down_button);

    columns_store = Gtk::ListStore::create(list_columns);
    for (const auto &it : adapter.get_column_names()) {
        Gtk::TreeModel::Row row = *columns_store->append();
        row[list_columns.column] = it.first;
        row[list_columns.name] = it.second;
    }

    columns_available = Gtk::TreeModelFilter::create(columns_store);
    columns_available->set_visible_func([this](const Gtk::TreeModel::const_iterator &it) -> bool {
        Gtk::TreeModel::Row row = *it;
        int col = row[list_columns.column];
        return !adapter.has_column(col);
    });
    cols_available_tv->set_model(columns_available);
    cols_available_tv->append_column("Column", list_columns.name);

    columns_store_included = Gtk::ListStore::create(list_columns);
    update_cols_included();

    cols_included_tv->set_model(columns_store_included);
    cols_included_tv->append_column("Column", list_columns.name);

    col_inc_button->signal_clicked().connect([this] { incl_excl_col(true); });
    col_excl_button->signal_clicked().connect([this] { incl_excl_col(false); });

    col_up_button->signal_clicked().connect([this] { up_down_col(true); });
    col_down_button->signal_clicked().connect([this] { up_down_col(false); });

    cols_included_tv->get_selection()->signal_changed().connect(
            sigc::mem_fun(*this, &ColumnChooser::update_incl_excl_sensitivity));
    cols_available_tv->get_selection()->signal_changed().connect(
            sigc::mem_fun(*this, &ColumnChooser::update_incl_excl_sensitivity));

    update_incl_excl_sensitivity();
}


void ColumnChooser::update_incl_excl_sensitivity()
{
    col_inc_button->set_sensitive(cols_available_tv->get_selection()->count_selected_rows());
    auto count_inc = cols_included_tv->get_selection()->count_selected_rows();
    col_excl_button->set_sensitive(count_inc);
    col_up_button->set_sensitive(count_inc);
    col_down_button->set_sensitive(count_inc);
}

void ColumnChooser::incl_excl_col(bool incl)
{
    Gtk::TreeView *tv;
    if (incl)
        tv = cols_available_tv;
    else
        tv = cols_included_tv;

    if (tv->get_selection()->count_selected_rows() != 1)
        return;

    int col = 0;
    {
        Gtk::TreeModel::Row row = *tv->get_selection()->get_selected();
        col = row[list_columns.column];
    }

    if (incl)
        adapter.include_column(col);
    else
        adapter.exclude_column(col);

    columns_available->refilter();
    update_cols_included();

    if (incl) {
        for (const auto &it : columns_store_included->children()) {
            Gtk::TreeModel::Row row = *it;
            if (row[list_columns.column] == col) {
                cols_included_tv->get_selection()->select(it);
                break;
            }
        }
    }

    s_signal_changed.emit();
}

void ColumnChooser::up_down_col(bool up)
{
    if (cols_included_tv->get_selection()->count_selected_rows() != 1)
        return;

    Gtk::TreeModel::Row row = *cols_included_tv->get_selection()->get_selected();
    int col = row[list_columns.column];

    adapter.move_column(col, up);

    update_cols_included();
    s_signal_changed.emit();
}

void ColumnChooser::update_cols_included()
{
    bool selected = cols_included_tv->get_selection()->count_selected_rows() == 1;
    int col_selected = 0;
    if (selected) {
        Gtk::TreeModel::Row row = *cols_included_tv->get_selection()->get_selected();
        col_selected = row[list_columns.column];
    }

    columns_store_included->clear();
    for (const auto &it : adapter.get_columns()) {
        Gtk::TreeModel::Row row = *columns_store_included->append();
        row[list_columns.column] = it;
        row[list_columns.name] = adapter.get_column_name(it);
    }

    if (selected) {
        for (const auto &it : columns_store_included->children()) {
            Gtk::TreeModel::Row row = *it;
            if (row[list_columns.column] == col_selected) {
                cols_included_tv->get_selection()->select(it);
                break;
            }
        }
    }
}


} // namespace horizon
