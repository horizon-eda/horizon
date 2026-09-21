#include "step_export_window.hpp"
#include "export_step/export_step.hpp"
#include "board/board.hpp"
#include "board/step_export_settings.hpp"
#include "util/util.hpp"
#include "util/gtk_util.hpp"
#include "widgets/spin_button_dim.hpp"
#include "document/idocument_board.hpp"
#include <thread>

namespace horizon {

void StepExportWindow::MyExportFileChooser::prepare_chooser(Glib::RefPtr<Gtk::FileChooser> chooser)
{
    auto filter = Gtk::FileFilter::create();
    filter->set_name("STEP models");
    filter->add_pattern("*.step");
    filter->add_pattern("*.stp");
    chooser->add_filter(filter);
}

void StepExportWindow::MyExportFileChooser::prepare_filename(std::string &filename)
{
    if (!endswith(filename, ".step") && !endswith(filename, ".stp")) {
        filename += ".step";
    }
}

StepExportWindow *StepExportWindow::create(Gtk::Window *p, IDocumentBoard &c, const std::string &project_dir)
{
    StepExportWindow *w;
    Glib::RefPtr<Gtk::Builder> x = Gtk::Builder::create();
    x->add_from_resource("/org/horizon-eda/horizon/imp/step_export.ui");
    x->get_widget_derived("window", w, c, project_dir);
    w->set_transient_for(*p);
    return w;
}

StepExportWindow::StepExportWindow(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x, IDocumentBoard &c,
                                   const std::string &project_dir)
    : Gtk::Window(cobject), core(c), settings(core.get_step_export_settings())
{
    set_modal(true);
    GET_WIDGET(header);
    GET_WIDGET(spinner);
    GET_WIDGET(export_button);
    GET_WIDGET(filename_button);
    GET_WIDGET(filename_entry);
    GET_WIDGET(prefix_entry);
    GET_WIDGET(log_textview);
    GET_WIDGET(include_3d_models_switch);
    GET_WIDGET(min_dia_box);
    GET_WIDGET(pkg_incl_button);
    GET_WIDGET(pkg_excl_button);
    GET_WIDGET(pkgs_included_tv);
    GET_WIDGET(pkgs_excluded_tv);

    auto width_chars = 10;
    min_dia_spin_button = Gtk::manage(new SpinButtonDim());
    min_dia_spin_button->set_width_chars(width_chars);
    min_dia_spin_button->set_range(0.00_mm, 99_mm);
    min_dia_spin_button->set_value(0.00_mm);
    min_dia_spin_button->show();

    min_dia_box->pack_start(*min_dia_spin_button, true, true, 0);

    export_button->signal_clicked().connect(sigc::mem_fun(*this, &StepExportWindow::generate));

    export_filechooser.attach(filename_entry, filename_button, this);
    export_filechooser.set_project_dir(project_dir);
    export_filechooser.bind_filename(settings.filename);
    export_filechooser.signal_changed().connect([this] {
        s_signal_changed.emit();
        update_export_button();
    });

    bind_widget(include_3d_models_switch, settings.include_3d_models);
    bind_widget(prefix_entry, settings.prefix);
    bind_widget(min_dia_spin_button, settings.min_diameter);

    include_3d_models_switch->property_active().signal_changed().connect([this] { s_signal_changed.emit(); });
    prefix_entry->signal_changed().connect([this] { s_signal_changed.emit(); });
    min_dia_spin_button->signal_changed().connect([this] { s_signal_changed.emit(); });

    pkgs_store = Gtk::ListStore::create(list_columns);
    pkgs_fill();

    pkgs_included = Gtk::TreeModelFilter::create(pkgs_store);
    pkgs_included->set_visible_func([this](const Gtk::TreeModel::const_iterator &it) -> bool {
        Gtk::TreeModel::Row row = *it;
        const UUID &uuid = row[list_columns.uuid];
        auto search = settings.pkgs_excluded.find(uuid);
        return search == settings.pkgs_excluded.end();
    });
    auto pkgs_included_sort = Gtk::TreeModelSort::create(pkgs_included);
    pkgs_included_tv->set_model(pkgs_included_sort);
    pkgs_included_tv->append_column("Package", list_columns.pkg_name);
    pkgs_included_tv->append_column("Count", list_columns.pkg_count);
    pkgs_included_sort->set_sort_column(0, Gtk::SortType::SORT_ASCENDING);

    pkgs_excluded = Gtk::TreeModelFilter::create(pkgs_store);
    pkgs_excluded->set_visible_func([this](const Gtk::TreeModel::const_iterator &it) -> bool {
        Gtk::TreeModel::Row row = *it;
        const UUID &uuid = row[list_columns.uuid];
        auto search = settings.pkgs_excluded.find(uuid);
        return search != settings.pkgs_excluded.end();
    });
    auto pkgs_excluded_sort = Gtk::TreeModelSort::create(pkgs_excluded);
    pkgs_excluded_tv->set_model(pkgs_excluded_sort);
    pkgs_excluded_tv->append_column("Package", list_columns.pkg_name);
    pkgs_excluded_tv->append_column("Count", list_columns.pkg_count);
    pkgs_excluded_sort->set_sort_column(0, Gtk::SortType::SORT_ASCENDING);

    pkg_incl_button->signal_clicked().connect([this] { pkg_incl_excl(true); });
    pkg_excl_button->signal_clicked().connect([this] { pkg_incl_excl(false); });

    pkgs_included_tv->get_selection()->signal_changed().connect(
            sigc::mem_fun(*this, &StepExportWindow::update_pkg_incl_excl_sensitivity));
    pkgs_excluded_tv->get_selection()->signal_changed().connect(
            sigc::mem_fun(*this, &StepExportWindow::update_pkg_incl_excl_sensitivity));
    update_pkg_incl_excl_sensitivity();

    tag = log_textview->get_buffer()->create_tag();
    tag->property_font_features() = "tnum 1";
    tag->property_font_features_set() = true;


    export_dispatcher.connect([this] {
        std::lock_guard<std::mutex> guard(msg_queue_mutex);
        auto buffer = log_textview->get_buffer();
        while (msg_queue.size()) {
            const std::string &msg = msg_queue.front();
            buffer->insert(buffer->end(), msg + "\n");
            auto adj = log_textview->get_vadjustment();
            adj->set_value(adj->get_upper());
            msg_queue.pop_front();
        }
        {
            Gtk::TextIter ibegin, iend;
            buffer->get_bounds(ibegin, iend);
            buffer->remove_all_tags(ibegin, iend);
            buffer->apply_tag(tag, ibegin, iend);
        }
        if (export_running == false)
            set_is_busy(false);
    });
    update_export_button();
}

void StepExportWindow::pkgs_fill()
{
    auto brd = core.get_board();
    struct pkg_info {
        std::string name;
        unsigned int count;

        pkg_info(const std::string name_) : name(name_), count(0)
        {
        }
    };
    std::map<UUID, pkg_info> summary;

    for (const auto &it : brd->packages) {
        const auto pkg = it.second;
        const auto sum_it = summary.try_emplace(pkg.package.uuid, pkg.package.name).first;
        sum_it->second.count++;
    }

    for (const auto &it : summary) {
        Gtk::TreeModel::Row row = *pkgs_store->append();
        row[list_columns.pkg_name] = it.second.name;
        row[list_columns.pkg_count] = it.second.count;
        row[list_columns.uuid] = it.first;
    }
}

void StepExportWindow::update_pkg_incl_excl_sensitivity()
{
    pkg_incl_button->set_sensitive(pkgs_excluded_tv->get_selection()->count_selected_rows());
    pkg_excl_button->set_sensitive(pkgs_included_tv->get_selection()->count_selected_rows());
}

void StepExportWindow::pkg_incl_excl(bool incl)
{
    Gtk::TreeView *tv;
    Glib::RefPtr<Gtk::TreeModel> model;

    if (incl)
        tv = pkgs_excluded_tv;
    else
        tv = pkgs_included_tv;

    for (const auto &path : tv->get_selection()->get_selected_rows(model)) {
        const auto &it = model->get_iter(path);
        const auto &uuid = (*it)[list_columns.uuid];

        if (incl)
            settings.pkgs_excluded.erase(uuid);
        else
            settings.pkgs_excluded.emplace(uuid);
    }

    pkgs_excluded->refilter();
    pkgs_included->refilter();
    s_signal_changed.emit();
}

void StepExportWindow::generate()
{
    if (export_running)
        return;
    if (settings.filename.size() == 0)
        return;
    set_is_busy(true);
    log_textview->get_buffer()->set_text("");
    msg_queue.clear();
    export_running = true;
    STEPExportSettings my_settings = settings;
    my_settings.filename = export_filechooser.get_filename_abs();
    std::thread thr(&StepExportWindow::export_thread, this, my_settings);
    thr.detach();
}

void StepExportWindow::export_thread(STEPExportSettings my_settings)
{
    try {
        export_step(
                my_settings.filename, *core.get_board(), core.get_pool_caching(), my_settings.include_3d_models,
                [this](const std::string &msg) {
                    {
                        std::lock_guard<std::mutex> guard(msg_queue_mutex);
                        msg_queue.push_back(msg);
                    }
                    export_dispatcher.emit();
                },
                &core.get_colors(), my_settings.prefix, my_settings.min_diameter, &my_settings.pkgs_excluded);
    }
    catch (const std::exception &e) {
        {
            std::lock_guard<std::mutex> guard(msg_queue_mutex);
            msg_queue.push_back(std::string("Error: ") + e.what());
        }
        export_dispatcher.emit();
    }
    catch (...) {
        {
            std::lock_guard<std::mutex> guard(msg_queue_mutex);
            msg_queue.push_back("Error: unknown error");
        }
        export_dispatcher.emit();
    }

    export_running = false;
    export_dispatcher.emit();
}

void StepExportWindow::set_can_export(bool v)
{
    can_export = v;
    update_export_button();
}

void StepExportWindow::update_export_button()
{
    std::string txt;
    if (!export_running) {
        if (settings.filename.size() == 0) {
            txt = "output filename not set";
        }
    }
    else {
        txt = "busy";
    }
    widget_set_insensitive_tooltip(*export_button, txt);
}

void StepExportWindow::set_is_busy(bool v)
{
    update_export_button();
    spinner->property_active() = v;
    header->set_show_close_button(!v);
}

} // namespace horizon
