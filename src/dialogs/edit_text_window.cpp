#include "edit_text_window.hpp"
#include "common/text.hpp"
#include "widgets/text_editor.hpp"
#include "widgets/spin_button_dim.hpp"
#include "util/gtk_util.hpp"
#include "common/object_descr.hpp"

namespace horizon {

EditTextWindow::EditTextWindow(Gtk::Window *parent, ImpInterface *intf, Text &txt, bool use_ok)
    : ToolWindow(parent, intf), text(txt)
{
    set_title("Edit Text");
    set_use_ok(use_ok);

    auto grid = Gtk::manage(new Gtk::Grid);
    grid->property_margin() = 20;
    grid->set_row_spacing(10);
    grid->set_column_spacing(10);

    editor = Gtk::manage(new TextEditor(TextEditor::Lines::MULTI));
    editor->set_hexpand(true);
    grid->attach(*editor, 0, 0, 2, 1);
    editor->set_text(text.text, TextEditor::Select::YES);
    editor->signal_changed().connect([this] {
        text.text = editor->get_text();
        emit_event(ToolDataWindow::Event::UPDATE);
    });
    editor->signal_activate().connect([this] { emit_event(ToolDataWindow::Event::OK); });

    int top = 1;

    sp_size = Gtk::manage(new SpinButtonDim);
    sp_size->set_hexpand(true);
    sp_size->set_range(.01_mm, 100_mm);
    sp_size->set_value(text.size);
    sp_size->signal_value_changed().connect([this] {
        text.size = sp_size->get_value_as_int();
        emit_event(ToolDataWindow::Event::UPDATE);
    });
    spinbutton_connect_activate(sp_size, [this] { emit_event(ToolDataWindow::Event::OK); });
    grid_attach_label_and_widget(grid, "Size", sp_size, top);


    sp_width = Gtk::manage(new SpinButtonDim);
    sp_width->set_range(0, 100_mm);
    sp_width->set_value(text.width);
    sp_width->set_increments(.01_mm, .001_mm);
    sp_width->signal_value_changed().connect([this] {
        text.width = sp_width->get_value_as_int();
        emit_event(ToolDataWindow::Event::UPDATE);
    });
    spinbutton_connect_activate(sp_width, [this] { emit_event(ToolDataWindow::Event::OK); });
    grid_attach_label_and_widget(grid, "Width", sp_width, top);

    combo_font = Gtk::manage(new Gtk::ComboBoxText);
    {
        auto &items = object_descriptions.at(ObjectType::TEXT).properties.at(ObjectProperty::ID::FONT).enum_items;
        for (const auto &[i, name] : items) {
            combo_font->append(std::to_string(i), name);
        }
    }
    combo_font->set_active_id(std::to_string(static_cast<int>(text.font)));
    combo_font->signal_changed().connect([this] {
        text.font = static_cast<TextData::Font>(std::stoi(combo_font->get_active_id()));
        emit_event(ToolDataWindow::Event::UPDATE);
    });
    grid_attach_label_and_widget(grid, "Font", combo_font, top);


    grid->show_all();
    add(*grid);
}

void EditTextWindow::focus_text()
{
    editor->grab_focus();
}

void EditTextWindow::focus_size()
{
    sp_size->grab_focus();
}

void EditTextWindow::focus_width()
{
    sp_width->grab_focus();
}

void EditTextWindow::set_dims(uint64_t size, uint64_t width)
{
    sp_size->set_value(size);
    sp_width->set_value(width);
}

} // namespace horizon
