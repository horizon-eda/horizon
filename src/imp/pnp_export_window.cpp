#include "pnp_export_window.hpp"
#include "board/board.hpp"
#include "util/gtk_util.hpp"
#include "util/util.hpp"
#include "export_pnp/export_pnp.hpp"
#include "widgets/help_button.hpp"
#include "help_texts.hpp"
#include <nlohmann/json.hpp>

#include <iostream>

namespace horizon {

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

    GET_WIDGET(pnp_header);
    GET_WIDGET(export_button);
    GET_WIDGET(directory_button);
    GET_WIDGET(directory_entry);
    GET_WIDGET(done_label);
    GET_WIDGET(done_revealer);
    GET_WIDGET(done_close_button);
    GET_WIDGET(mode_combo);
    GET_WIDGET(nopopulate_check);
    GET_WIDGET(filename_merged_entry);
    GET_WIDGET(filename_top_entry);
    GET_WIDGET(filename_bottom_entry);
    GET_WIDGET(filename_merged_label);
    GET_WIDGET(filename_top_label);
    GET_WIDGET(filename_bottom_label);
    GET_WIDGET(preview_tv);
    GET_WIDGET(customize_revealer);
    GET_WIDGET(customize_check);
    GET_WIDGET(customize_grid);

    done_revealer_controller.attach(done_revealer, done_label, done_close_button);

    {
        auto hamburger_button = Gtk::manage(new Gtk::MenuButton);
        hamburger_button->set_image_from_icon_name("open-menu-symbolic", Gtk::ICON_SIZE_BUTTON);

        hamburger_menu = Gio::Menu::create();
        hamburger_button->set_menu_model(hamburger_menu);
        hamburger_button->show();
        pnp_header->pack_end(*hamburger_button);

        auto action_group = Gio::SimpleActionGroup::create();
        insert_action_group("pnp_export", action_group);
        action_group->add_action("save_settings", [this] { save_to_file(); });
        action_group->add_action("load_settings", [this] { load_from_file(); });
        hamburger_menu->append("Save Settings", "pnp_export.save_settings");
        hamburger_menu->append("Load Settings", "pnp_export.load_settings");
    }

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
        settings.mode = PnPExportSettings::mode_lut.lookup(static_cast<std::string>(mode_combo->get_active_id()));
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

    {
        Gtk::Widget *position_format_box;
        GET_WIDGET(position_format_box);
        HelpButton::pack_into(x, "position_format_box", HelpTexts::POSITION_FORMAT, HelpButton::FLAG_NON_MODAL);
    }

    {
        int top = 3;
        for (const auto &[id, name] : pnp_column_names) {
            auto entry = Gtk::manage(new Gtk::Entry);
            column_name_entries[id] = entry;
            entry->show();
            auto box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 5));
            {
                auto la = Gtk::manage(new Gtk::Label("Col:"));
                la->get_style_context()->add_class("dim-label");
                la->set_xalign(0);
                box->pack_start(*la, true, true, 0);
            }
            {
                auto la = Gtk::manage(new Gtk::Label(name));
                la->get_style_context()->add_class("dim-label");
                la->set_xalign(1);
                box->pack_start(*la, true, true, 0);
            }
            box->show_all();
            if (top == 3) {
                box->set_margin_top(10);
                entry->set_margin_top(10);
            }
            customize_grid->attach(*box, 0, top, 1, 1);
            customize_grid->attach(*entry, 1, top, 1, 1);
            top++;
            bind_widget(entry, settings.column_names[id], [this](std::string &) {
                s_signal_changed.emit();
                update_preview();
            });
        }

        {
            auto reset_button = Gtk::manage(new Gtk::Button("Reset to default"));
            reset_button->set_tooltip_text("Reset position format, side and column names to default values");
            reset_button->set_margin_top(10);

            reset_button->signal_clicked().connect([this] {
                for (const auto &[id, name] : pnp_column_names) {
                    column_name_entries[id]->set_text(name);
                }
                const PnPExportSettings default_settings;
                top_side_entry->set_text(default_settings.top_side);
                bottom_side_entry->set_text(default_settings.bottom_side);
                position_format_entry->set_text(default_settings.position_format);
            });
            customize_grid->attach(*reset_button, 0, top++, 2, 1);
            reset_button->show();
        }
    }
    customize_check->set_active(settings.customize);
    customize_revealer->set_reveal_child(settings.customize);
    customize_check->signal_toggled().connect([this] {
        const auto a = customize_check->get_active();
        customize_revealer->set_reveal_child(a);
        settings.customize = a;
        update_preview();
        s_signal_changed.emit();
    });
    {
#define BIND_ENTRY(name_)                                                                                              \
    do {                                                                                                               \
        GET_WIDGET(name_##_entry);                                                                                     \
        bind_widget(name_##_entry, settings.name_, [this](std::string &) {                                             \
            s_signal_changed.emit();                                                                                   \
            update_preview();                                                                                          \
        });                                                                                                            \
    } while (0)

        BIND_ENTRY(position_format);
        BIND_ENTRY(top_side);
        BIND_ENTRY(bottom_side);
#undef BIND_ENTRY
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

void PnPExportWindow::generate()
{
    try {
        PnPExportSettings my_settings = settings;
        my_settings.output_directory = export_filechooser.get_filename_abs();
        export_PnP(board, my_settings);
        done_revealer_controller.show_success("Pick & place export done");
    }
    catch (const std::exception &e) {
        done_revealer_controller.show_error("Error: " + std::string(e.what()));
    }
    catch (...) {
        done_revealer_controller.show_error("unknown error");
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
        auto tvc = Gtk::manage(new Gtk::TreeViewColumn(settings.get_column_name(col)));
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
            mcr->property_text() = pnprow.get_column(col, settings);
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

void PnPExportWindow::save_to_file()
{
    GtkFileChooserNative *native = gtk_file_chooser_native_new("Save Settings", GTK_WINDOW(gobj()),
                                                               GTK_FILE_CHOOSER_ACTION_SAVE, "_Save", "_Cancel");
    auto chooser = Glib::wrap(GTK_FILE_CHOOSER(native));
    chooser->set_do_overwrite_confirmation(true);

    std::string path;
    auto success = run_native_filechooser_with_retry(chooser, "Error saving settings", [this, chooser, &path] {
        path = append_dot_json(chooser->get_filename());
    });
    if (success) {
        json j = settings.serialize();
        save_json_to_file(path, j);
        std::cout << "Saved settings to: " << path << std::endl;
    }
}

void PnPExportWindow::load_from_file()
{
    GtkFileChooserNative *native = gtk_file_chooser_native_new("Save Settings", GTK_WINDOW(gobj()),
                                                               GTK_FILE_CHOOSER_ACTION_OPEN, "_Open", "_Cancel");

    auto chooser = Glib::wrap(GTK_FILE_CHOOSER(native));

    if (gtk_native_dialog_run(GTK_NATIVE_DIALOG(native)) == GTK_RESPONSE_ACCEPT) {
        std::string path = chooser->get_filename();
        json j = load_json_from_file(path);
        auto new_settings = PnPExportSettings(j);
        mode_combo->set_active_id(PnPExportSettings::mode_lut.lookup_reverse(new_settings.mode));
        nopopulate_check->set_active(new_settings.include_nopopulate);
        customize_check->set_active(new_settings.customize);

        settings.columns = new_settings.columns;
        column_chooser->update_from_adapter();
        for (const auto &[id, name] : new_settings.column_names) {
            column_name_entries[id]->set_text(name);
        }

#define SET_ENTRY(name_)                                                                                               \
    do {                                                                                                               \
        name_##_entry->set_text(new_settings.name_);                                                                   \
    } while (0)

        SET_ENTRY(filename_top);
        SET_ENTRY(filename_bottom);
        SET_ENTRY(filename_merged);
        SET_ENTRY(position_format);
        SET_ENTRY(top_side);
        SET_ENTRY(bottom_side);
        std::cout << "Loaded settings from: " << path << std::endl;
    }
#undef SET_ENTRY
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
