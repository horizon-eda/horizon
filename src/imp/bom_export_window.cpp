#include "bom_export_window.hpp"
#include "block/block.hpp"
#include "util/gtk_util.hpp"
#include "util/util.hpp"
#include "export_bom/export_bom.hpp"

namespace horizon {

#define GET_WIDGET(name)                                                                                               \
    do {                                                                                                               \
        x->get_widget(#name, name);                                                                                    \
    } while (0)

BOMExportWindow::BOMExportWindow(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x, Block *blo,
                                 BOMExportSettings *s)
    : Gtk::Window(cobject), block(blo), settings(s), state_store(this, "imp-bom-export")
{

    GET_WIDGET(export_button);
    GET_WIDGET(filename_button);
    GET_WIDGET(filename_entry);
    GET_WIDGET(sort_column_combo);
    GET_WIDGET(sort_order_combo);
    GET_WIDGET(cols_available_tv);
    GET_WIDGET(cols_included_tv);
    GET_WIDGET(col_inc_button);
    GET_WIDGET(col_excl_button);
    GET_WIDGET(col_up_button);
    GET_WIDGET(col_down_button);
    GET_WIDGET(done_label);
    GET_WIDGET(done_revealer);
    GET_WIDGET(preview_tv);

    filename_button->signal_clicked().connect([this] {
        GtkFileChooserNative *native = gtk_file_chooser_native_new("Set output file", GTK_WINDOW(gobj()),
                                                                   GTK_FILE_CHOOSER_ACTION_SAVE, "Set", "_Cancel");
        auto chooser = Glib::wrap(GTK_FILE_CHOOSER(native));
        auto filter = Gtk::FileFilter::create();
        filter->set_name("CSV files");
        filter->add_pattern("*.csv");
        filter->add_pattern("*.CSV");
        chooser->add_filter(filter);
        chooser->set_filename(filename_entry->get_text());

        if (gtk_native_dialog_run(GTK_NATIVE_DIALOG(native)) == GTK_RESPONSE_ACCEPT) {
            std::string filename = chooser->get_filename();
            if (!endswith(filename, ".csv")) {
                filename += ".csv";
            }
            filename_entry->set_text(filename);
        }
    });

    bind_widget(filename_entry, settings->output_filename, [this](const std::string &v) { s_signal_changed.emit(); });

    for (const auto &it : bom_column_names) {
        sort_column_combo->append(std::to_string(static_cast<int>(it.first)), it.second);
    }
    sort_column_combo->set_active_id(std::to_string(static_cast<int>(settings->csv_settings.sort_column)));
    sort_column_combo->signal_changed().connect([this] {
        settings->csv_settings.sort_column = static_cast<BOMColumn>(std::stoi(sort_column_combo->get_active_id()));
        update_preview();
        s_signal_changed.emit();
    });

    {
        const std::map<BOMExportSettings::CSVSettings::Order, std::string> its = {
                {BOMExportSettings::CSVSettings::Order::ASC, "Asc."},
                {BOMExportSettings::CSVSettings::Order::DESC, "Desc."},
        };

        bind_widget<BOMExportSettings::CSVSettings::Order>(sort_order_combo, its, settings->csv_settings.order,
                                                           [this](BOMExportSettings::CSVSettings::Order o) {
                                                               update_preview();
                                                               s_signal_changed.emit();
                                                           });
    }

    bom_store = Gtk::ListStore::create(list_columns_preview);
    preview_tv->set_model(bom_store);

    columns_store = Gtk::ListStore::create(list_columns);
    for (const auto &it : bom_column_names) {
        Gtk::TreeModel::Row row = *columns_store->append();
        row[list_columns.column] = it.first;
        row[list_columns.name] = it.second;
    }

    columns_available = Gtk::TreeModelFilter::create(columns_store);
    columns_available->set_visible_func([this](const Gtk::TreeModel::const_iterator &it) -> bool {
        Gtk::TreeModel::Row row = *it;
        BOMColumn col = row[list_columns.column];
        return std::count_if(settings->csv_settings.columns.begin(), settings->csv_settings.columns.end(),
                             [col](const auto a) { return a == col; })
               == 0;
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
            sigc::mem_fun(this, &BOMExportWindow::update_incl_excl_sensitivity));
    cols_available_tv->get_selection()->signal_changed().connect(
            sigc::mem_fun(this, &BOMExportWindow::update_incl_excl_sensitivity));

    update_incl_excl_sensitivity();

    export_button->signal_clicked().connect(sigc::mem_fun(this, &BOMExportWindow::generate));
}

void BOMExportWindow::update_incl_excl_sensitivity()
{
    col_inc_button->set_sensitive(cols_available_tv->get_selection()->count_selected_rows());
    auto count_inc = cols_included_tv->get_selection()->count_selected_rows();
    col_excl_button->set_sensitive(count_inc);
    col_up_button->set_sensitive(count_inc);
    col_down_button->set_sensitive(count_inc);
}

void BOMExportWindow::incl_excl_col(bool incl)
{
    Gtk::TreeView *tv;
    if (incl)
        tv = cols_available_tv;
    else
        tv = cols_included_tv;

    if (tv->get_selection()->count_selected_rows() != 1)
        return;

    BOMColumn col;
    {
        Gtk::TreeModel::Row row = *tv->get_selection()->get_selected();
        col = row[list_columns.column];
    }

    auto &cols = settings->csv_settings.columns;
    auto c = std::count(cols.begin(), cols.end(), col);

    if (incl && c == 0)
        cols.push_back(col);

    else if (!incl && c > 0)
        cols.erase(std::remove(cols.begin(), cols.end(), col), cols.end());

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

void BOMExportWindow::up_down_col(bool up)
{
    if (cols_included_tv->get_selection()->count_selected_rows() != 1)
        return;

    Gtk::TreeModel::Row row = *cols_included_tv->get_selection()->get_selected();
    BOMColumn col = row[list_columns.column];

    auto &cols = settings->csv_settings.columns;

    auto it = std::find(cols.begin(), cols.end(), col);
    if (it == cols.end())
        return;

    if (up && it == cols.begin()) // already at the top
        return;

    if (!up && it == cols.end() - 1) // already at the bottom
        return;

    auto it_other = it + (up ? -1 : +1);

    std::swap(*it_other, *it);
    update_cols_included();
    s_signal_changed.emit();
}

void BOMExportWindow::update_cols_included()
{
    bool selected = cols_included_tv->get_selection()->count_selected_rows() == 1;
    BOMColumn col_selected = BOMColumn::MPN;
    if (selected) {
        Gtk::TreeModel::Row row = *cols_included_tv->get_selection()->get_selected();
        col_selected = row[list_columns.column];
    }

    columns_store_included->clear();
    for (const auto &it : settings->csv_settings.columns) {
        Gtk::TreeModel::Row row = *columns_store_included->append();
        row[list_columns.column] = it;
        row[list_columns.name] = bom_column_names.at(it);
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
    update_preview();
}

void BOMExportWindow::flash(const std::string &s)
{
    done_label->set_text(s);
    done_revealer->set_reveal_child(true);
    flash_connection.disconnect();
    flash_connection = Glib::signal_timeout().connect(
            [this] {
                done_revealer->set_reveal_child(false);
                return false;
            },
            2000);
}

void BOMExportWindow::generate()
{
    try {
        export_BOM(settings->output_filename, *block, *settings);
        flash("BOM written to " + settings->output_filename);
    }
    catch (const std::exception &e) {
        flash("Error: " + std::string(e.what()));
    }
    catch (...) {
        flash("unknown error");
    }
    update_preview();
}

void BOMExportWindow::update_preview()
{
    auto rows = block->get_BOM(*settings);
    std::vector<BOMRow> bom;
    std::transform(rows.begin(), rows.end(), std::back_inserter(bom), [](const auto &it) { return it.second; });
    std::sort(bom.begin(), bom.end(), [this](const auto &a, const auto &b) {
        auto sa = a.get_column(settings->csv_settings.sort_column);
        auto sb = b.get_column(settings->csv_settings.sort_column);
        auto c = strcmp_natural(sa, sb);
        if (settings->csv_settings.order == BOMExportSettings::CSVSettings::Order::ASC)
            return c < 0;
        else
            return c > 0;
    });

    preview_tv->remove_all_columns();
    bom_store->clear();
    for (const auto &it : bom) {
        Gtk::TreeModel::Row row = *bom_store->append();
        row[list_columns_preview.row] = it;
    }

    for (auto col : settings->csv_settings.columns) {
        auto cr_text = Gtk::manage(new Gtk::CellRendererText());
        auto tvc = Gtk::manage(new Gtk::TreeViewColumn(bom_column_names.at(col)));
        tvc->set_resizable();
        tvc->pack_start(*cr_text, true);
        if (col == BOMColumn::REFDES) {
            cr_text->property_ellipsize() = Pango::ELLIPSIZE_END;
            tvc->set_expand(true);
        }
        else if (col == BOMColumn::DATASHEET) {
            cr_text->property_ellipsize() = Pango::ELLIPSIZE_END;
        }
        tvc->set_cell_data_func(*cr_text, [this, col](Gtk::CellRenderer *tcr, const Gtk::TreeModel::iterator &it) {
            Gtk::TreeModel::Row row = *it;
            BOMRow bomrow = row[list_columns_preview.row];
            auto mcr = dynamic_cast<Gtk::CellRendererText *>(tcr);
            mcr->property_text() = bomrow.get_column(col);
        });
        preview_tv->append_column(*tvc);
    }
}

void BOMExportWindow::set_can_export(bool v)
{
    export_button->set_sensitive(v);
}

BOMExportWindow *BOMExportWindow::create(Gtk::Window *p, Block *b, BOMExportSettings *s)
{
    BOMExportWindow *w;
    Glib::RefPtr<Gtk::Builder> x = Gtk::Builder::create();
    x->add_from_resource("/net/carrotIndustries/horizon/imp/bom_export.ui");
    x->get_widget_derived("window", w, b, s);

    w->set_transient_for(*p);

    return w;
}
} // namespace horizon
