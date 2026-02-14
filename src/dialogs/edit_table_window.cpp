#include "edit_table_window.hpp"

#include "common/object_descr.hpp"
#include "common/table.hpp"
#include "util/gtk_util.hpp"
#include "util/text_data.hpp"
#include "widgets/text_editor.hpp"

#include <cassert>

namespace horizon {
EditTableWindow::EditTableWindow(Window *parent, ImpInterface *intf, Table &tbl, bool use_ok)
    : ToolWindow(parent, intf), table(tbl)
{
    set_title("Edit Table");
    set_use_ok(use_ok);
    set_default_size(600, 400);

    auto box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL));

    // properties grid
    {
        auto props_grid = Gtk::manage(new Gtk::Grid);
        props_grid->set_margin_left(20);
        props_grid->set_margin_right(20);
        props_grid->set_margin_top(20);
        props_grid->set_row_spacing(10);
        props_grid->set_column_spacing(10);

        int left = 0;
        int top = -1;

        // lays out the properties widgets in a grid with ROWS_IN_PROPS_GRID rows
        auto attach_props_widget = [props_grid, &left, &top](const std::string &label, Widget *w) {
            static constexpr int ROWS_IN_PROPS_GRID = 3;

            if (++top == ROWS_IN_PROPS_GRID) {
                top = 0;
                left += 2;
            }

            auto lbl = Gtk::manage(new Gtk::Label(label));
            lbl->get_style_context()->add_class("dim-label");
            lbl->set_xalign(1);
            // add extra space between the property groups
            if (left > 0)
                lbl->set_margin_start(10);
            lbl->show();
            props_grid->attach(*lbl, left, top, 1, 1);

            w->set_hexpand(true);
            w->show();
            props_grid->attach(*w, left + 1, top, 1, 1);
        };

        // number of rows
        {
            sp_n_rows = Gtk::manage(new Gtk::SpinButton);
            sp_n_rows->set_range(1, 100);
            sp_n_rows->set_increments(1, 5);
            sp_n_rows->set_value(table.get_n_rows());
            sp_n_rows->signal_value_changed().connect([this] {
                table.resize(sp_n_rows->get_value_as_int(), table.get_n_columns());
                rebuild_cell_grid();
                emit_event(ToolDataWindow::Event::UPDATE);
            });
            spinbutton_connect_activate(sp_n_rows, [this] { emit_event(ToolDataWindow::Event::OK); });
            attach_props_widget("Rows", sp_n_rows);
        }

        // number of columns
        {
            sp_n_columns = Gtk::manage(new Gtk::SpinButton);
            sp_n_columns->set_range(1, 100);
            sp_n_columns->set_increments(1, 5);
            sp_n_columns->set_value(table.get_n_columns());
            sp_n_columns->signal_value_changed().connect([this] {
                table.resize(table.get_n_rows(), sp_n_columns->get_value_as_int());
                rebuild_cell_grid();
                emit_event(ToolDataWindow::Event::UPDATE);
            });
            spinbutton_connect_activate(sp_n_columns, [this] { emit_event(ToolDataWindow::Event::OK); });
            attach_props_widget("Columns", sp_n_columns);
        }

        // cell padding
        {
            sp_padding = Gtk::manage(new SpinButtonDim);
            sp_padding->set_range(0_mm, 100_mm);
            sp_padding->set_value(table.get_padding());
            sp_padding->set_increments(.01_mm, .001_mm);
            sp_padding->signal_value_changed().connect([this] {
                table.set_padding(sp_padding->get_value_as_int());
                emit_event(ToolDataWindow::Event::UPDATE);
            });
            spinbutton_connect_activate(sp_padding, [this] { emit_event(ToolDataWindow::Event::OK); });
            attach_props_widget("Padding", sp_padding);
        }

        // text size
        {
            sp_text_size = Gtk::manage(new SpinButtonDim);
            sp_text_size->set_hexpand(true);
            sp_text_size->set_range(.01_mm, 100_mm);
            sp_text_size->set_value(table.get_text_size());
            sp_text_size->signal_value_changed().connect([this] {
                table.set_text_size(sp_text_size->get_value_as_int());
                emit_event(ToolDataWindow::Event::UPDATE);
            });
            spinbutton_connect_activate(sp_text_size, [this] { emit_event(ToolDataWindow::Event::OK); });
            attach_props_widget("Text size", sp_text_size);
        }

        // line width
        {
            sp_line_width = Gtk::manage(new SpinButtonDim);
            sp_line_width->set_range(0_mm, 100_mm);
            sp_line_width->set_value(table.get_line_width());
            sp_line_width->set_increments(.01_mm, .001_mm);
            sp_line_width->signal_value_changed().connect([this] {
                table.set_line_width(sp_line_width->get_value_as_int());
                emit_event(ToolDataWindow::Event::UPDATE);
            });
            spinbutton_connect_activate(sp_line_width, [this] { emit_event(ToolDataWindow::Event::OK); });
            attach_props_widget("Line width", sp_line_width);
        }

        //
        {
            combo_font = Gtk::manage(new Gtk::ComboBoxText);
            {
                auto &items =
                        object_descriptions.at(ObjectType::TEXT).properties.at(ObjectProperty::ID::FONT).enum_items;
                for (const auto &[i, name] : items) {
                    combo_font->append(std::to_string(i), name);
                }
            }
            combo_font->set_active_id(std::to_string(static_cast<int>(table.get_font())));
            combo_font->signal_changed().connect([this] {
                table.set_font(static_cast<TextData::Font>(std::stoi(combo_font->get_active_id())));
                emit_event(ToolDataWindow::Event::UPDATE);
            });
            attach_props_widget("Font", combo_font);
        }

        box->pack_start(*props_grid, false, false);
    }

    // separator
    {
        auto sep = Gtk::manage(new Gtk::Separator(Gtk::ORIENTATION_HORIZONTAL));
        sep->set_margin_top(10);
        box->pack_start(*sep, false, false);
    }

    // cell grid
    {
        auto grid_container = Gtk::manage(new Gtk::ScrolledWindow);
        grid_container->set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
        grid_container->set_vexpand(true);

        cell_grid = Gtk::manage(new Gtk::Grid);
        cell_grid->set_margin_top(10);
        cell_grid->set_margin_bottom(20);
        cell_grid->set_margin_left(20);
        cell_grid->set_margin_right(20);
        cell_grid->set_row_spacing(10);
        cell_grid->set_column_spacing(10);
        grid_container->add(*cell_grid);

        box->pack_start(*grid_container, true, true);
    }

    // separator
    {
        auto sep = Gtk::manage(new Gtk::Separator(Gtk::ORIENTATION_HORIZONTAL));
        box->pack_start(*sep, false, false);
    }

    // we want the hint label to be initially hidden
    box->show_all();

    // hint label
    {
        static auto SINGLE_LINE_HINT = "Press Shift+Enter to switch to multi-line mode";
        static auto MULTI_LINE_HINT = "Press Shift+Enter to apply changes";

        auto la = Gtk::manage(new Gtk::Label());
        la->get_style_context()->add_class("dim-label");
        la->set_margin_top(5);
        la->set_margin_bottom(5);
        make_label_small(la);
        box->pack_start(*la, false, false, 0);

        // make the label text adapt to the state of the currently active TextEditor (if any)
        cell_grid->signal_set_focus_child().connect([la](Widget *w) {
            auto text_editor = dynamic_cast<TextEditor *>(w);
            if (text_editor != nullptr) {
                if (text_editor->is_multi_line()) {
                    la->set_text(MULTI_LINE_HINT);
                    la->show();
                }
                else {
                    la->set_text(SINGLE_LINE_HINT);
                    la->show();
                }
            }
            else {
                la->set_text("");
                la->hide();
            }
        });
    }

    rebuild_cell_grid();
    add(*box);
}

void EditTableWindow::focus_n_rows()
{
    sp_n_rows->grab_focus();
}

void EditTableWindow::rebuild_cell_grid()
{
    assert(cell_grid != nullptr);

    for (auto child : cell_grid->get_children()) {
        cell_grid->remove(*child);
    }

    for (size_t r = 0; r < table.get_n_rows(); r++) {
        for (size_t c = 0; c < table.get_n_columns(); c++) {
            auto entry = Gtk::manage(new TextEditor(TextEditor::Lines::MULTI, TextEditor::ShowHints::NO));
            entry->set_text(table.get_cell(r, c), TextEditor::Select::NO);
            entry->set_hexpand(true);
            entry->signal_changed().connect([this, r, c, entry] {
                table.set_cell(r, c, entry->get_text());
                emit_event(ToolDataWindow::Event::UPDATE);
            });
            entry->signal_activate().connect([this] { emit_event(ToolDataWindow::Event::OK); });
            cell_grid->attach(*entry, c, r, 1, 1);
        }
    }
    cell_grid->show_all();
}

} // namespace horizon