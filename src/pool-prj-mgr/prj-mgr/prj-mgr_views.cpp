#include "prj-mgr_views.hpp"
#include "pool-prj-mgr/pool-prj-mgr-app_win.hpp"
#include "pool-prj-mgr/pool-prj-mgr-app.hpp"
#include "project/project.hpp"
#include "pool/pool_manager.hpp"
#include "nlohmann/json.hpp"
#include "part_browser/part_browser_window.hpp"
#include "widgets/project_meta_editor.hpp"
#include "blocks/blocks.hpp"
#include "pool/pool_cache_status.hpp"
#include "util/gtk_util.hpp"
#include "pool/pool.hpp"

namespace horizon {

PoolProjectManagerViewCreateProject::PoolProjectManagerViewCreateProject(const Glib::RefPtr<Gtk::Builder> &builder,
                                                                         class PoolProjectManagerAppWindow &w)
    : win(w)
{
    builder->get_widget("create_project_path_chooser", project_path_chooser);
    builder->get_widget("create_project_dir_label", project_dir_label);
    builder->get_widget("create_project_pool_combo", project_pool_combo);

    Gtk::Box *meta_box;
    builder->get_widget("new_project_meta_box", meta_box);
    meta_editor = Gtk::manage(new ProjectMetaEditor(meta_values));
    meta_box->pack_start(*meta_editor, false, false, 0);
    meta_editor->set_row_spacing(20);
    meta_editor->show();
    meta_editor->signal_changed().connect(sigc::mem_fun(*this, &PoolProjectManagerViewCreateProject::update));
    project_pool_combo->signal_changed().connect(sigc::mem_fun(*this, &PoolProjectManagerViewCreateProject::update));
    project_path_chooser->signal_file_set().connect(sigc::mem_fun(*this, &PoolProjectManagerViewCreateProject::update));
}

void PoolProjectManagerViewCreateProject::clear()
{
    meta_editor->clear();
    meta_editor->preset(win.app.user_config.project_author);
    meta_editor->set_use_automatic_name();
    project_dir_label->set_text("");
    project_path_chooser->unselect_all();
    if (win.app.user_config.project_base_path.size())
        project_path_chooser->set_filename(win.app.user_config.project_base_path);
}

void PoolProjectManagerViewCreateProject::focus()
{
    meta_editor->focus_title();
}

void PoolProjectManagerViewCreateProject::populate_pool_combo()
{
    project_pool_combo->remove_all();
    for (const auto &it : PoolManager::get().get_pools()) {
        if (it.second.enabled)
            project_pool_combo->append((std::string)it.second.uuid, it.second.name);
    }
    project_pool_combo->set_active(0);
}

std::optional<std::string> PoolProjectManagerViewCreateProject::create()
{
    try {
        Project prj(UUID::random());
        const auto base_path = project_path_chooser->get_file()->get_path();
        prj.base_path = Glib::build_filename(base_path, meta_values.at("project_name"));
        auto pool_uuid = static_cast<std::string>(project_pool_combo->get_active_id());
        auto pool = PoolManager::get().get_by_uuid(pool_uuid);
        if (!pool)
            throw std::runtime_error("pool not found");
        win.app.user_config.project_author = meta_values.at("author");
        win.app.user_config.project_base_path = base_path;
        return prj.create(meta_values, *pool);
    }
    catch (const std::exception &e) {
        Gtk::MessageDialog md(win, "Error creating project", false /* use_markup */, Gtk::MESSAGE_ERROR,
                              Gtk::BUTTONS_OK);
        md.set_secondary_text(e.what());
        md.run();
    }
    catch (const Glib::Error &e) {
        Gtk::MessageDialog md(win, "Error creating project", false /* use_markup */, Gtk::MESSAGE_ERROR,
                              Gtk::BUTTONS_OK);
        md.set_secondary_text(e.what());
        md.run();
    }
    return {};
}

void PoolProjectManagerViewCreateProject::update()
{
    bool valid = true;
    if (meta_values.at("project_name").size() == 0)
        valid = false;
    if (meta_values.at("project_title").size() == 0)
        valid = false;
    if (project_path_chooser->get_filenames().size() == 0)
        valid = false;
    if (project_pool_combo->get_active_row_number() == -1)
        valid = false;
    if (valid) {
        auto base_path = project_path_chooser->get_file()->get_path();
        auto dir = Glib::build_filename(base_path, meta_values.at("project_name"));
        project_dir_label->set_text(dir);
    }
    else {
        project_dir_label->set_text("");
    }
    signal_valid_change().emit(valid);
}

class OpeningSpinner : public Gtk::Revealer {
public:
    OpeningSpinner();
    void set_active(bool a);

private:
    Gtk::Spinner *spinner = nullptr;
};

OpeningSpinner::OpeningSpinner()
{
    set_transition_type(Gtk::REVEALER_TRANSITION_TYPE_CROSSFADE);
    auto box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 4));
    spinner = Gtk::manage(new Gtk::Spinner());
    box->pack_start(*spinner, false, false, 0);

    auto la = Gtk::manage(new Gtk::Label("Opening…"));
    box->pack_start(*la, false, false, 0);

    box->show_all();
    add(*box);
}

void OpeningSpinner::set_active(bool a)

{
    set_reveal_child(a);
    if (a)
        spinner->start();
    else
        spinner->stop();
}

PoolProjectManagerViewProject::PoolProjectManagerViewProject(const Glib::RefPtr<Gtk::Builder> &builder,
                                                             PoolProjectManagerAppWindow &w)
    : win(w)
{
    builder->get_widget("button_top_schematic", button_top_schematic);
    builder->get_widget("button_board", button_board);
    builder->get_widget("button_part_browser", button_part_browser);
    builder->get_widget("button_project_pool", button_project_pool);
    builder->get_widget("label_project_title", label_project_title);
    builder->get_widget("label_project_author", label_project_author);
    builder->get_widget("label_project_directory", label_project_directory);
    builder->get_widget("label_project_pools", label_project_pools);
    builder->get_widget("pool_cache_status_label", pool_cache_status_label);

    {
        Gtk::Box *board_opening_box;
        builder->get_widget("board_opening_box", board_opening_box);
        board_spinner = Gtk::manage(new OpeningSpinner());
        board_opening_box->pack_start(*board_spinner, false, false, 0);
        board_spinner->show();
    }
    {
        Gtk::Box *schematic_opening_box;
        builder->get_widget("schematic_opening_box", schematic_opening_box);
        schematic_spinner = Gtk::manage(new OpeningSpinner());
        schematic_opening_box->pack_start(*schematic_spinner, false, false, 0);
        schematic_spinner->show();
    }

    Gtk::Button *open_button;
    builder->get_widget("prj_open_dir_button", open_button);
    open_button->signal_clicked().connect([this] { open_directory(win, win.project_filename); });

    Gtk::Button *edit_button;
    builder->get_widget("prj_edit_meta_button", edit_button);
    edit_button->signal_clicked().connect([this] {
        if (auto proc = win.find_top_schematic_process()) {
            win.app.send_json(proc->proc->get_pid(), {{"op", "edit-meta"}});
        }
        else {
            open_top_schematic();
            if (auto proc2 = win.find_top_schematic_process()) {
                proc2->signal_ready().connect([this, proc2] {
                    win.app.send_json(proc2->proc->get_pid(), {{"op", "edit-meta"}});
                });
            }
        }
    });

    Gtk::Button *change_button;
    builder->get_widget("prj_change_pools_button", change_button);
    change_button->signal_clicked().connect([this] {
        auto path = Glib::build_filename(win.project->pool_directory, "pool.json");
        auto &win2 = win.app.open_pool(path);
        win2.pool_notebook_show_settings_tab();
    });

    button_top_schematic->signal_clicked().connect(
            sigc::mem_fun(*this, &PoolProjectManagerViewProject::open_top_schematic));
    button_board->signal_clicked().connect(sigc::mem_fun(*this, &PoolProjectManagerViewProject::open_board));
    button_part_browser->signal_clicked().connect(
            sigc::mem_fun(*this, &PoolProjectManagerViewProject::handle_button_part_browser));
    button_project_pool->signal_clicked().connect(
            sigc::mem_fun(*this, &PoolProjectManagerViewProject::handle_button_project_pool));
}

void PoolProjectManagerViewProject::open_top_schematic()
{
    auto prj = win.project.get();
    std::vector<std::string> args = {prj->blocks_filename, prj->pictures_directory};
    auto proc = win.spawn(PoolProjectManagerProcess::Type::IMP_SCHEMATIC, args);
    if (proc.spawned) {
        schematic_spinner->set_active(true);
        proc.proc->signal_exited().connect([this](int, bool) { schematic_spinner->set_active(false); });
        proc.proc->signal_ready().connect([this] { schematic_spinner->set_active(false); });
    }
}

void PoolProjectManagerViewProject::open_board()
{
    auto prj = win.project.get();
    std::vector<std::string> args = {prj->board_filename, prj->blocks_filename, prj->pictures_directory};
    auto proc = win.spawn(PoolProjectManagerProcess::Type::IMP_BOARD, args);
    if (proc.spawned) {
        board_spinner->set_active(true);
        proc.proc->signal_exited().connect([this](int, bool) { board_spinner->set_active(false); });
        proc.proc->signal_ready().connect([this] { board_spinner->set_active(false); });
    }
}

void PoolProjectManagerViewProject::handle_button_part_browser()
{
    win.part_browser_window->present();
}

void PoolProjectManagerViewProject::handle_button_project_pool()
{
    auto path = Glib::build_filename(win.project->pool_directory, "pool.json");
    win.app.open_pool(path);
}

bool PoolProjectManagerViewProject::update_meta()
{
    auto meta = BlocksBase::peek_project_meta(win.project->blocks_filename);
    std::string title;
    std::string author;
    if (meta.count("project_title"))
        title = meta.at("project_title");
    if (meta.count("author"))
        author = meta.at("author");
    if (title.size())
        win.set_title(title + " – Horizon EDA");
    else
        win.set_title("Project Manager");
    label_project_title->set_text(title);
    label_project_author->set_text(author);
    win.project_title = title;
    return meta.size();
}

void PoolProjectManagerViewProject::update_pools_label()
{
    Pool my_pool(win.project->pool_directory);
    std::string pools_label;
    std::string pools_tooltip;
    for (const auto &[bp, uu] : my_pool.get_actually_included_pools(false)) {
        const auto inc_pool = PoolManager::get().get_by_uuid(uu);
        if (inc_pool) {
            if (pools_label.size()) {
                pools_label += ", ";
                pools_tooltip += "\n";
            }
            pools_label += inc_pool->name;
            pools_tooltip += inc_pool->name;
        }
    }
    label_project_pools->set_text(pools_label);
    label_project_pools->set_tooltip_text(pools_tooltip);
}

void PoolProjectManagerViewProject::update_pool_cache_status(const PoolCacheStatus &status)
{
    std::string txt;
    if (status.n_current == status.n_total) {
        txt = "All current";
    }
    else {
        txt = std::to_string(status.n_out_of_date) + " out of date";
    }
    pool_cache_status_label->set_text(txt);
}

void PoolProjectManagerViewProject::reset_pool_cache_status()
{
    pool_cache_status_label->set_text("");
}

} // namespace horizon
