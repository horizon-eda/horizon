#include "imp_frame.hpp"
#include "canvas/canvas_gl.hpp"
#include "header_button.hpp"
#include "core/tool_id.hpp"
#include "widgets/action_button.hpp"
#include "util/gtk_util.hpp"
#include "util/util.hpp"

namespace horizon {
ImpFrame::ImpFrame(const std::string &frame_filename, const std::string &pool_path, TempMode tmp_mode)
    : ImpBase(pool_path), core_frame(frame_filename, *pool.get()), frame(core_frame.get_frame())
{
    core = &core_frame;
    core_frame.signal_tool_changed().connect(sigc::mem_fun(*this, &ImpBase::handle_tool_change));
    if (tmp_mode == TempMode::YES) {
        core_frame.set_temp_mode();
        temp_mode = true;
    }
}

void ImpFrame::canvas_update()
{
    canvas->update(core_frame.get_canvas_data());
}

void ImpFrame::construct()
{

    state_store = std::make_unique<WindowStateStore>(main_window, "imp-frame");

    header_button = Gtk::manage(new HeaderButton);
    main_window->header->set_custom_title(*header_button);
    header_button->show();
    header_button->signal_closed().connect(sigc::mem_fun(*this, &ImpFrame::update_header));

    name_entry = header_button->add_entry("Name");
    name_entry->set_text(frame.name);
    name_entry->set_width_chars(std::max<int>(frame.name.size(), 20));
    name_entry->signal_changed().connect([this] { core_frame.set_needs_save(); });
    name_entry->signal_activate().connect(sigc::mem_fun(*this, &ImpFrame::update_header));

    core->signal_save().connect([this] { frame.name = name_entry->get_text(); });

    hamburger_menu->append("Frame properties", "win.edit_frame");
    add_tool_action(ToolID::EDIT_FRAME_PROPERTIES, "edit_frame");

    add_action_button_line();
    add_action_button_polygon();
    add_action_button(ToolID::PLACE_TEXT);


    update_header();
}

void ImpFrame::update_header()
{
    header_button->set_label(name_entry->get_text());
    set_window_title(name_entry->get_text());
}

bool ImpFrame::set_filename()
{
    GtkFileChooserNative *native = gtk_file_chooser_native_new("Save Frame", GTK_WINDOW(main_window->gobj()),
                                                               GTK_FILE_CHOOSER_ACTION_SAVE, "_Save", "_Cancel");
    auto chooser = Glib::wrap(GTK_FILE_CHOOSER(native));
    chooser->set_do_overwrite_confirmation(true);
    chooser->set_current_folder(Glib::build_filename(pool->get_base_path(), "frames"));
    chooser->set_current_name(name_entry->get_text() + ".json");

    std::string filename;
    auto success = run_native_filechooser_with_retry(chooser, "Error saving frame", [this, chooser, &filename] {
        filename = append_dot_json(chooser->get_filename());
        pool->check_filename_throw(ObjectType::FRAME, filename);
        core_frame.set_filename(filename);
    });
    return success;
}

} // namespace horizon
