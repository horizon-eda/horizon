#include "pnp_export_window.hpp"
#include "board/board.hpp"
#include "util/gtk_util.hpp"
#include "util/util.hpp"
#include "export_pnp/export_pnp.hpp"

namespace horizon {

#define GET_WIDGET(name)                                                                                               \
    do {                                                                                                               \
        x->get_widget(#name, name);                                                                                    \
    } while (0)

#define GET_OBJECT(name)                                                                                               \
    do {                                                                                                               \
        name = name.cast_dynamic(x->get_object(#name));                                                                \
    } while (0)

std::map<int, std::string> PnPExportWindow::MyAdapter::get_column_names() const
{
    std::map<int, std::string> r;
    for (const auto &it : pnp_column_names) {
        r.emplace(static_cast<int>(it.first), it.second);
    }
    return r;
}

std::string PnPExportWindow::MyAdapter::get_column_name(int col) const
{
    return pnp_column_names.at(static_cast<PnPColumn>(col));
}

PnPExportWindow::PnPExportWindow(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x, const class Board &brd,
                                 class PnPExportSettings &s, const std::string &project_dir)
    : Gtk::Window(cobject), board(brd), settings(s), state_store(this, "imp-pnp-export"), adapter(settings.columns)
{

    GET_WIDGET(export_button);
    GET_WIDGET(directory_button);
    GET_WIDGET(directory_entry);
    GET_WIDGET(done_label);
    GET_WIDGET(done_revealer);
    GET_WIDGET(mode_combo);
    GET_WIDGET(nopopulate_check);
    GET_WIDGET(filename_merged_entry);
    GET_WIDGET(filename_top_entry);
    GET_WIDGET(filename_bottom_entry);
    GET_WIDGET(filename_merged_label);
    GET_WIDGET(filename_top_label);
    GET_WIDGET(filename_bottom_label);
    GET_WIDGET(preview_tv);

    export_filechooser.attach(directory_entry, directory_button, this);
    export_filechooser.set_project_dir(project_dir);
    export_filechooser.bind_filename(settings.output_directory);
    export_filechooser.set_action(GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER);
    export_filechooser.signal_changed().connect([this] {
        s_signal_changed.emit();
        update_export_button();
    });
    bind_widget(filename_top_entry, settings.filename_top, [this](std::string &) {
        s_signal_changed.emit();
        update_export_button();
    });
    bind_widget(filename_bottom_entry, settings.filename_bottom, [this](std::string &) {
        s_signal_changed.emit();
        update_export_button();
    });
    bind_widget(filename_merged_entry, settings.filename_merged, [this](std::string &) {
        s_signal_changed.emit();
        update_export_button();
    });

    mode_combo->set_active_id(PnPExportSettings::mode_lut.lookup_reverse(settings.mode));
    mode_combo->signal_changed().connect([this] {
        settings.mode = PnPExportSettings::mode_lut.lookup(mode_combo->get_active_id());
        update_filename_visibility();
        s_signal_changed.emit();
        update_export_button();
    });
    update_filename_visibility();

    nopopulate_check->set_active(settings.include_nopopulate);
    nopopulate_check->signal_toggled().connect([this] {
        settings.include_nopopulate = nopopulate_check->get_active();
        s_signal_changed.emit();
        update_preview();
    });

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

    store = Gtk::ListStore::create(list_columns_preview);
    preview_tv->set_model(store);

    export_button->signal_clicked().connect(sigc::mem_fun(*this, &PnPExportWindow::generate));

    update();
    update_export_button();
}

void PnPExportWindow::update_filename_visibility()
{
    bool is_individual = settings.mode == PnPExportSettings::Mode::INDIVIDUAL;
    filename_bottom_entry->set_visible(is_individual);
    filename_bottom_label->set_visible(is_individual);
    filename_top_entry->set_visible(is_individual);
    filename_top_label->set_visible(is_individual);
    filename_merged_entry->set_visible(!is_individual);
    filename_merged_label->set_visible(!is_individual);
}

void PnPExportWindow::flash(const std::string &s)
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

void PnPExportWindow::generate()
{
    try {
        PnPExportSettings my_settings = settings;
        my_settings.output_directory = export_filechooser.get_filename_abs();
        export_PnP(board, my_settings);
        flash("Pick & place export done");
    }
    catch (const std::exception &e) {
        flash("Error: " + std::string(e.what()));
    }
    catch (...) {
        flash("unknown error");
    }
}

void PnPExportWindow::update_preview()
{
    auto rows = board.get_PnP(settings);
    std::vector<PnPRow> pnp;
    std::transform(rows.begin(), rows.end(), std::back_inserter(pnp), [](const auto &it) { return it.second; });
    std::sort(pnp.begin(), pnp.end(),
              [](const auto &a, const auto &b) { return strcmp_natural(a.refdes, b.refdes) < 0; });

    preview_tv->remove_all_columns();
    store->clear();
    for (const auto &it : pnp) {
        Gtk::TreeModel::Row row = *store->append();
        row[list_columns_preview.row] = it;
    }

    for (auto col : settings.columns) {
        auto cr_text = Gtk::manage(new Gtk::CellRendererText());
        auto tvc = Gtk::manage(new Gtk::TreeViewColumn(pnp_column_names.at(col)));
        auto attributes_list = pango_attr_list_new();
        auto attribute_font_features = pango_attr_font_features_new("tnum 1");
        pango_attr_list_insert(attributes_list, attribute_font_features);
        g_object_set(G_OBJECT(cr_text->gobj()), "attributes", attributes_list, NULL);
        pango_attr_list_unref(attributes_list);

        switch (col) {
        case PnPColumn::X:
        case PnPColumn::Y:
        case PnPColumn::ANGLE:
            cr_text->property_xalign() = 1;
            break;
        default:;
        }

        tvc->pack_start(*cr_text, true);
        tvc->set_cell_data_func(*cr_text, [this, col](Gtk::CellRenderer *tcr, const Gtk::TreeModel::iterator &it) {
            Gtk::TreeModel::Row row = *it;
            PnPRow pnprow = row[list_columns_preview.row];
            auto mcr = dynamic_cast<Gtk::CellRendererText *>(tcr);
            mcr->property_text() = pnprow.get_column(col);
        });
        preview_tv->append_column(*tvc);
    }
}

void PnPExportWindow::update()
{
    update_preview();
}

void PnPExportWindow::set_can_export(bool v)
{
    can_export = v;
    update_export_button();
}

void PnPExportWindow::update_export_button()
{
    std::string txt;
    if (can_export) {
        if (settings.output_directory.size()) {
            if (settings.mode == PnPExportSettings::Mode::INDIVIDUAL) {
                if (settings.filename_bottom.size() == 0 || settings.filename_top.size() == 0) {
                    txt = "output filename not set";
                }
            }
            else {
                if (settings.filename_merged.size() == 0) {
                    txt = "output filename not set";
                }
            }
        }
        else {
            txt = "output directory not set";
        }
    }
    else {
        txt = "tool is active";
    }
    widget_set_insensitive_tooltip(*export_button, txt);
}

PnPExportWindow *PnPExportWindow::create(Gtk::Window *p, const class Board &brd, class PnPExportSettings &settings,
                                         const std::string &project_dir)
{
    PnPExportWindow *w;
    Glib::RefPtr<Gtk::Builder> x = Gtk::Builder::create();
    x->add_from_resource("/org/horizon-eda/horizon/imp/pnp_export.ui");
    x->get_widget_derived("window", w, brd, settings, project_dir);

    w->set_transient_for(*p);

    return w;
}
} // namespace horizon
