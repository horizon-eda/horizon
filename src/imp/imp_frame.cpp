#include "imp_frame.hpp"
#include "canvas/canvas_gl.hpp"
#include "header_button.hpp"

namespace horizon {
ImpFrame::ImpFrame(const std::string &frame_filename, const std::string &pool_path)
    : ImpBase(pool_path), core_frame(frame_filename)
{
    core = &core_frame;
    core_frame.signal_tool_changed().connect(sigc::mem_fun(*this, &ImpBase::handle_tool_change));
}

void ImpFrame::canvas_update()
{
    canvas->update(*core_frame.get_canvas_data());
}

void ImpFrame::construct()
{

    main_window->set_title("Frame - Interactive Manipulator");
    state_store = std::make_unique<WindowStateStore>(main_window, "imp-frame");

    auto header_button = Gtk::manage(new HeaderButton);
    header_button->set_label(core_frame.get_frame()->name);
    main_window->header->set_custom_title(*header_button);
    header_button->show();

    name_entry = header_button->add_entry("Name");
    name_entry->set_text(core_frame.get_frame()->name);
    name_entry->set_width_chars(std::min<int>(core_frame.get_frame()->name.size(), 20));
    name_entry->signal_changed().connect([this, header_button] {
        header_button->set_label(name_entry->get_text());
        core_frame.set_needs_save();
    });
    core->signal_save().connect([this] {
        auto frame = core_frame.get_frame();
        frame->name = name_entry->get_text();
    });

    hamburger_menu->append("Frame properties...", "win.edit_frame");
    add_tool_action(ToolID::EDIT_FRAME_PROPERTIES, "edit_frame");
}

} // namespace horizon
