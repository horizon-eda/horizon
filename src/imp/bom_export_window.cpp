#include "bom_export_window.hpp"
#include "block/block.hpp"
#include "util/gtk_util.hpp"
#include "util/util.hpp"
#include "export_bom/export_bom.hpp"
#include "pool/part.hpp"
#include "pool/pool.hpp"
#include "widgets/generic_combo_box.hpp"
#include "widgets/pool_browser_parametric.hpp"
#include "preferences/preferences_provider.hpp"
#include "preferences/preferences.hpp"
#include "util/stock_info_provider_partinfo.hpp"

namespace horizon {

#define GET_WIDGET(name)                                                                                               \
    do {                                                                                                               \
        x->get_widget(#name, name);                                                                                    \
    } while (0)

#define GET_OBJECT(name)                                                                                               \
    do {                                                                                                               \
        name = name.cast_dynamic(x->get_object(#name));                                                                \
    } while (0)

void BOMExportWindow::MyExportFileChooser::prepare_chooser(Glib::RefPtr<Gtk::FileChooser> chooser)
{
    auto filter = Gtk::FileFilter::create();
    filter->set_name("CSV files");
    filter->add_pattern("*.csv");
    filter->add_pattern("*.CSV");
    chooser->add_filter(filter);
}

void BOMExportWindow::MyExportFileChooser::prepare_filename(std::string &filename)
{
    if (!endswith(filename, ".csv") && !endswith(filename, ".CSV")) {
        filename += ".csv";
    }
}

std::map<int, std::string> BOMExportWindow::MyAdapter::get_column_names() const
{
    std::map<int, std::string> r;
    for (const auto &it : bom_column_names) {
        r.emplace(static_cast<int>(it.first), it.second);
    }
    return r;
}

std::string BOMExportWindow::MyAdapter::get_column_name(int col) const
{
    return bom_column_names.at(static_cast<BOMColumn>(col));
}

BOMExportWindow::BOMExportWindow(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x, Block *blo,
                                 BOMExportSettings *s, Pool &p, const std::string &project_dir)
    : Gtk::Window(cobject), block(blo), settings(s), pool(p), pool_parametric(pool.get_base_path()),
      state_store(this, "imp-bom-export"), adapter(settings->csv_settings.columns)
{

    GET_WIDGET(export_button);
    GET_WIDGET(nopopulate_check);
    GET_WIDGET(filename_button);
    GET_WIDGET(filename_entry);
    GET_WIDGET(sort_column_combo);
    GET_WIDGET(sort_order_combo);
    GET_WIDGET(done_label);
    GET_WIDGET(done_revealer);
    GET_WIDGET(preview_tv);
    GET_WIDGET(orderable_MPNs_listbox);
    GET_OBJECT(sg_manufacturer);
    GET_OBJECT(sg_MPN);
    GET_OBJECT(sg_orderable_MPN);
    GET_WIDGET(meta_parts_tv);
    GET_WIDGET(param_browser_box);
    GET_WIDGET(concrete_parts_label);
    GET_WIDGET(rb_tol_10);
    GET_WIDGET(rb_tol_1);
    GET_WIDGET(button_clear_similar);
    GET_WIDGET(button_set_similar);

    export_filechooser.attach(filename_entry, filename_button, this);
    export_filechooser.set_project_dir(project_dir);
    export_filechooser.bind_filename(settings->output_filename);
    export_filechooser.signal_changed().connect([this] {
        s_signal_changed.emit();
        update_export_button();
    });

    nopopulate_check->set_active(settings->include_nopopulate);
    nopopulate_check->signal_toggled().connect([this] {
        settings->include_nopopulate = nopopulate_check->get_active();
        s_signal_changed.emit();
        update_preview();
    });

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

    {
        Gtk::Box *column_chooser_box;
        GET_WIDGET(column_chooser_box);
        column_chooser = ColumnChooser::create(adapter);
        column_chooser_box->pack_start(*column_chooser, true, true, 0);
        column_chooser->show();
        column_chooser->unreference();
        column_chooser->signal_changed().connect([this] {
            s_signal_changed.emit();
            update_preview();
        });
    }

    bom_store = Gtk::ListStore::create(list_columns_preview);
    preview_tv->set_model(bom_store);

    meta_parts_store = Gtk::ListStore::create(meta_parts_list_columns);
    meta_parts_tv->set_model(meta_parts_store);
    meta_parts_tv->append_column("QTY", meta_parts_list_columns.qty);
    meta_parts_tv->append_column("MPN", meta_parts_list_columns.MPN);
    meta_parts_tv->append_column("Value", meta_parts_list_columns.value);
    meta_parts_tv->append_column("Manufacturer", meta_parts_list_columns.manufacturer);
    meta_parts_tv->append_column("S MPN", meta_parts_list_columns.concrete_MPN);
    meta_parts_tv->append_column("S Value", meta_parts_list_columns.concrete_value);
    meta_parts_tv->append_column("S Manufacturer", meta_parts_list_columns.concrete_manufacturer);

    meta_parts_tv->get_selection()->signal_changed().connect(
            sigc::mem_fun(*this, &BOMExportWindow::update_concrete_parts));
    rb_tol_10->signal_toggled().connect(sigc::mem_fun(*this, &BOMExportWindow::update_concrete_parts));
    rb_tol_1->set_active(true);

    button_set_similar->signal_clicked().connect(sigc::mem_fun(*this, &BOMExportWindow::handle_set_similar));

    button_clear_similar->signal_clicked().connect([this] {
        settings->concrete_parts.erase(meta_part_current);
        update_meta_mapping();
        s_signal_changed.emit();
        update_similar_button_sensitivity();
        update_preview();
        update_orderable_MPNs();
    });
    update_similar_button_sensitivity();

    export_button->signal_clicked().connect(sigc::mem_fun(*this, &BOMExportWindow::generate));

    update();
    update_export_button();
}

void BOMExportWindow::update_similar_button_sensitivity()
{
    if (!meta_part_current) {
        button_set_similar->set_sensitive(false);
        button_clear_similar->set_sensitive(false);
        return;
    }
    button_clear_similar->set_sensitive(settings->concrete_parts.count(meta_part_current));
    button_set_similar->set_sensitive(browser_param ? browser_param->get_selected() : false);
}

void BOMExportWindow::handle_set_similar()
{
    auto sel = browser_param->get_selected();
    if (sel) {
        settings->concrete_parts[meta_part_current] = pool.get_part(sel);
        update_meta_mapping();
        s_signal_changed.emit();
    }
    update_similar_button_sensitivity();
    update_preview();
    update_orderable_MPNs();
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
        export_BOM(export_filechooser.get_filename_abs(), *block, *settings);
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

class OrderableMPNSelector : public Gtk::Box {
public:
    OrderableMPNSelector(const Part *p, BOMExportWindow *par)
        : Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 10), part(p), parent(par), settings(*parent->settings)
    {
        property_margin() = 5;
        set_margin_start(10);
        set_margin_end(10);
        auto la_manufacturer = Gtk::manage(new Gtk::Label(part->get_manufacturer()));
        la_manufacturer->set_xalign(0);
        la_manufacturer->show();
        pack_start(*la_manufacturer, false, false, 0);
        parent->sg_manufacturer->add_widget(*la_manufacturer);

        auto la_MPN = Gtk::manage(new Gtk::Label(part->get_MPN()));
        la_MPN->set_xalign(0);
        la_MPN->show();
        pack_start(*la_MPN, false, false, 0);
        parent->sg_MPN->add_widget(*la_MPN);

        auto combo = Gtk::manage(new GenericComboBox<UUID>());
        combo->append(UUID(), part->get_MPN() + " (base)");
        std::vector<std::pair<UUID, std::string>> orderable_MPNs_sorted;

        for (const auto &it : part->orderable_MPNs) {
            orderable_MPNs_sorted.push_back(it);
        }
        std::sort(orderable_MPNs_sorted.begin(), orderable_MPNs_sorted.end(),
                  [](const auto &a, const auto &b) { return strcmp_natural(a.second, b.second) < 0; });
        for (const auto &it : orderable_MPNs_sorted) {
            combo->append(it.first, it.second);
        }

        combo->show();
        pack_start(*combo, false, false, 0);
        parent->sg_orderable_MPN->add_widget(*combo);

        if (settings.orderable_MPNs.count(part->uuid))
            combo->set_active_key(settings.orderable_MPNs.at(part->uuid));
        else
            combo->set_active_key(UUID());
        combo->signal_changed().connect([this, combo] {
            settings.orderable_MPNs[part->uuid] = combo->get_active_key();
            parent->update_preview();
            parent->signal_changed().emit();
        });
    }

private:
    const Part *part;
    BOMExportWindow *parent;
    BOMExportSettings &settings;
};

void BOMExportWindow::update_orderable_MPNs()
{
    {
        auto children = orderable_MPNs_listbox->get_children();
        for (auto ch : children) {
            delete ch;
        }
    }
    BOMExportSettings my_settings(*settings);
    my_settings.include_nopopulate = true;
    auto rows = block->get_BOM(my_settings);
    for (const auto &row : rows) {
        auto part = row.first;
        if (part->orderable_MPNs.size()) {
            auto ed = Gtk::manage(new OrderableMPNSelector(part, this));
            ed->show();
            orderable_MPNs_listbox->append(*ed);
        }
    }
}

void BOMExportWindow::update()
{
    update_preview();
    update_orderable_MPNs();
    meta_parts_tv->unset_model();
    meta_parts_store->clear();
    std::map<const Part *, unsigned int> parts;
    for (const auto &it : block->components) {
        if (it.second.part) {
            parts[it.second.part]++;
        }
    }
    for (const auto &it : parts) {
        if (it.first->parametric.count("table")) {
            Gtk::TreeModel::Row row = *meta_parts_store->append();
            row[meta_parts_list_columns.MPN] = it.first->get_MPN();
            row[meta_parts_list_columns.value] = it.first->get_value();
            row[meta_parts_list_columns.manufacturer] = it.first->get_manufacturer();
            row[meta_parts_list_columns.uuid] = it.first->uuid;
            row[meta_parts_list_columns.qty] = it.second;
        }
    }
    meta_parts_tv->set_model(meta_parts_store);
    update_meta_mapping();
    update_concrete_parts();
}

void BOMExportWindow::update_meta_mapping()
{
    for (auto it : meta_parts_store->children()) {
        Gtk::TreeModel::Row row = *it;
        UUID uu = row[meta_parts_list_columns.uuid];
        if (settings->concrete_parts.count(uu)) {
            auto part = settings->concrete_parts.at(uu);
            row[meta_parts_list_columns.concrete_MPN] = part->get_MPN();
            row[meta_parts_list_columns.concrete_value] = part->get_value();
            row[meta_parts_list_columns.concrete_manufacturer] = part->get_manufacturer();
        }
        else {
            row[meta_parts_list_columns.concrete_MPN] = "";
            row[meta_parts_list_columns.concrete_value] = "";
            row[meta_parts_list_columns.concrete_manufacturer] = "";
        }
    }
}

void BOMExportWindow::update_concrete_parts()
{
    auto sel = meta_parts_tv->get_selection()->get_selected();
    if (browser_param) {
        delete browser_param;
        browser_param = nullptr;
    }
    if (sel) {
        auto row = *sel;
        auto part = pool.get_part(row[meta_parts_list_columns.uuid]);
        meta_part_current = part->uuid;
        concrete_parts_label->set_text("Similar parts to " + part->get_MPN());

        browser_param = Gtk::manage(new PoolBrowserParametric(&pool, &pool_parametric, part->parametric.at("table")));
        browser_param->set_similar_part_uuid(part->uuid);
        browser_param->search();
        float tol = rb_tol_10->get_active() ? .1 : .01;
        browser_param->filter_similar(part->uuid, tol);
        if (PreferencesProvider::get_prefs().partinfo.is_enabled()) {
            auto prv = std::make_unique<StockInfoProviderPartinfo>(pool.get_base_path());
            browser_param->add_stock_info_provider(std::move(prv));
        }
        browser_param->search();
        param_browser_box->pack_start(*browser_param, true, true);
        browser_param->show();
        browser_param->signal_activated().connect(sigc::mem_fun(*this, &BOMExportWindow::handle_set_similar));
        browser_param->signal_selected().connect(
                sigc::mem_fun(*this, &BOMExportWindow::update_similar_button_sensitivity));
        update_similar_button_sensitivity();
    }
    else {
        concrete_parts_label->set_text("Select part to pick similar part");
        meta_part_current = UUID();
    }
}

void BOMExportWindow::set_can_export(bool v)
{
    can_export = v;
    update_export_button();
}

void BOMExportWindow::update_export_button()
{
    std::string txt;
    if (can_export) {
        if (settings->output_filename.size() == 0) {
            txt = "output filename not set";
        }
    }
    else {
        txt = "tool is active";
    }
    widget_set_insensitive_tooltip(*export_button, txt);
}

BOMExportWindow *BOMExportWindow::create(Gtk::Window *p, Block *b, BOMExportSettings *s, Pool &pool,
                                         const std::string &project_dir)
{
    BOMExportWindow *w;
    Glib::RefPtr<Gtk::Builder> x = Gtk::Builder::create();
    x->add_from_resource("/org/horizon-eda/horizon/imp/bom_export.ui");
    x->get_widget_derived("window", w, b, s, pool, project_dir);

    w->set_transient_for(*p);

    return w;
}
} // namespace horizon
