#include "schematic_properties.hpp"
#include "schematic/schematic.hpp"
#include "widgets/pool_browser_button.hpp"
#include "widgets/pool_browser.hpp"
#include "widgets/title_block_values_editor.hpp"
#include "widgets/project_meta_editor.hpp"
#include "pool/ipool.hpp"
#include <iostream>
#include <deque>
#include <algorithm>

namespace horizon {

class SheetEditor : public Gtk::Box {
public:
    SheetEditor(Sheet &s, Schematic &c, IPool &pool);

private:
    Gtk::Grid *grid = nullptr;
    Sheet &sheet;
    int top = 0;
    void append_widget(const std::string &label, Gtk::Widget *w);
};

void SheetEditor::append_widget(const std::string &label, Gtk::Widget *w)
{
    auto la = Gtk::manage(new Gtk::Label(label));
    la->get_style_context()->add_class("dim-label");
    la->show();
    grid->attach(*la, 0, top, 1, 1);

    w->show();
    w->set_hexpand(true);
    grid->attach(*w, 1, top, 1, 1);
    top++;
}

SheetEditor::SheetEditor(Sheet &s, Schematic &c, IPool &pool) : Gtk::Box(Gtk::ORIENTATION_VERTICAL, 0), sheet(s)
{
    grid = Gtk::manage(new Gtk::Grid);
    grid->set_column_spacing(10);
    grid->set_row_spacing(10);
    grid->set_margin_bottom(20);
    grid->set_margin_top(20);
    grid->set_margin_end(20);
    grid->set_margin_start(20);

    auto title_entry = Gtk::manage(new Gtk::Entry());
    title_entry->set_text(s.name);
    title_entry->signal_changed().connect([this, title_entry] {
        auto ti = title_entry->get_text();
        sheet.name = ti;
        auto stack = dynamic_cast<Gtk::Stack *>(get_ancestor(GTK_TYPE_STACK));
        stack->child_property_title(*this) = "Sheet " + ti;
    });

    append_widget("Title", title_entry);

    auto frame_button = Gtk::manage(new PoolBrowserButton(ObjectType::FRAME, pool));
    frame_button->get_browser().set_show_none(true);
    if (sheet.pool_frame)
        frame_button->property_selected_uuid() = sheet.pool_frame->uuid;
    frame_button->property_selected_uuid().signal_changed().connect([this, frame_button, &pool] {
        UUID uu = frame_button->property_selected_uuid();
        if (uu)
            sheet.pool_frame = pool.get_frame(uu);
        else
            sheet.pool_frame = nullptr;
    });
    append_widget("Frame", frame_button);

    pack_start(*grid, false, false, 0);
    grid->show();


    auto ed = Gtk::manage(new TitleBlockValuesEditor(sheet.title_block_values));
    pack_start(*ed, true, true, 0);
    ed->show();
}

SchematicPropertiesDialog::SchematicPropertiesDialog(Gtk::Window *parent, Schematic &c, IPool &pool)
    : Gtk::Dialog("Schematic properties", *parent,
                  Gtk::DialogFlags::DIALOG_MODAL | Gtk::DialogFlags::DIALOG_USE_HEADER_BAR),
      sch(c)
{
    add_button("Cancel", Gtk::ResponseType::RESPONSE_CANCEL);
    add_button("OK", Gtk::ResponseType::RESPONSE_OK);
    set_default_response(Gtk::ResponseType::RESPONSE_OK);
    set_default_size(400, 300);

    auto box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 0));

    auto stack = Gtk::manage(new Gtk::Stack());
    stack->set_transition_type(Gtk::STACK_TRANSITION_TYPE_SLIDE_RIGHT);
    stack->set_transition_duration(100);
    auto sidebar = Gtk::manage(new Gtk::StackSidebar());
    sidebar->set_stack(*stack);

    box->pack_start(*sidebar, false, false, 0);
    box->pack_start(*stack, true, true, 0);

    {
        auto ed = Gtk::manage(new ProjectMetaEditor(sch.block->project_meta));
        ed->property_margin() = 20;
        ed->show();
        stack->add(*ed, "global", "Global");
    }

    std::vector<Sheet *> sheets;
    for (auto &it : sch.sheets) {
        sheets.push_back(&it.second);
    }
    std::sort(sheets.begin(), sheets.end(), [](auto a, auto b) { return a->index < b->index; });
    for (auto &it : sheets) {
        auto ed = Gtk::manage(new SheetEditor(*it, sch, pool));
        stack->add(*ed, (std::string)it->uuid, "Sheet " + it->name);
    }

    get_content_area()->pack_start(*box, true, true, 0);
    get_content_area()->set_border_width(0);


    show_all();
}
} // namespace horizon
