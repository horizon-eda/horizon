#include "schematic_properties.hpp"
#include "schematic/schematic.hpp"
#include "widgets/pool_browser_button.hpp"
#include "widgets/pool_browser.hpp"
#include "widgets/title_block_values_editor.hpp"
#include "widgets/project_meta_editor.hpp"
#include "pool/ipool.hpp"
#include "document/idocument_schematic.hpp"
#include "blocks/blocks_schematic.hpp"
#include <iostream>
#include <deque>
#include <algorithm>
#include "dialogs/dialogs.hpp"

namespace horizon {

class SheetEditor : public Gtk::Box {
public:
    SheetEditor(Sheet &s, IPool &pool, class IPool &pool_caching);

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

SheetEditor::SheetEditor(Sheet &s, IPool &pool, class IPool &pool_caching)
    : Gtk::Box(Gtk::ORIENTATION_VERTICAL, 0), sheet(s)
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
    });

    append_widget("Title", title_entry);

    auto frame_button = Gtk::manage(new PoolBrowserButton(ObjectType::FRAME, pool));
    frame_button->get_browser().set_show_none(true);
    if (sheet.pool_frame)
        frame_button->property_selected_uuid() = sheet.pool_frame->uuid;
    frame_button->property_selected_uuid().signal_changed().connect([this, frame_button, &pool_caching] {
        UUID uu = frame_button->property_selected_uuid();
        if (uu)
            sheet.pool_frame = pool_caching.get_frame(uu);
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

SchematicPropertiesDialog::SchematicPropertiesDialog(Gtk::Window *parent, IDocumentSchematic &adoc)
    : Gtk::Dialog("Schematic properties", *parent,
                  Gtk::DialogFlags::DIALOG_MODAL | Gtk::DialogFlags::DIALOG_USE_HEADER_BAR),
      doc(adoc), sch(*doc.get_top_schematic())
{
    add_button("Cancel", Gtk::ResponseType::RESPONSE_CANCEL);
    add_button("OK", Gtk::ResponseType::RESPONSE_OK);
    set_default_response(Gtk::ResponseType::RESPONSE_OK);
    set_default_size(400, 300);

    box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 0));
    store = Gtk::TreeStore::create(tree_columns);
    view = Gtk::manage(new Gtk::TreeView(store));
    view->get_selection()->set_mode(Gtk::SELECTION_BROWSE);
    view->set_headers_visible(false);
    {
        auto sc = Gtk::manage(new Gtk::ScrolledWindow);
        sc->set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC);
        sc->add(*view);


        auto lbox = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL, 0));
        lbox->pack_start(*sc, true, true, 0);

        {
            auto sep = Gtk::manage(new Gtk::Separator(Gtk::ORIENTATION_HORIZONTAL));
            lbox->pack_start(*sep, false, false, 0);
        }
        {
            auto tb = Gtk::manage(new Gtk::Toolbar());
            tb->set_icon_size(Gtk::ICON_SIZE_MENU);
            tb->set_toolbar_style(Gtk::TOOLBAR_ICONS);
            {
                auto tbo = Gtk::manage(new Gtk::ToolButton());
                tbo->set_icon_name("list-add-symbolic");
                tbo->signal_clicked().connect([this] { add_block(); });
                tb->insert(*tbo, -1);
            }
            {
                auto tbo = Gtk::manage(new Gtk::ToolButton());
                tbo->set_icon_name("list-remove-symbolic");
                tbo->signal_clicked().connect([this] {

                });

                tb->insert(*tbo, -1);
            }

            tb->show_all();
            lbox->pack_start(*tb, false, false, 0);
        }

        box->pack_start(*lbox, false, false, 0);
    }
    view->append_column("", tree_columns.name);


    update_view();

    {
        auto sep = Gtk::manage(new Gtk::Separator(Gtk::ORIENTATION_VERTICAL));
        box->pack_start(*sep, false, false, 0);
    }

    view->get_selection()->signal_changed().connect(
            sigc::mem_fun(*this, &SchematicPropertiesDialog::selection_changed));

    selection_changed();

    get_content_area()->pack_start(*box, true, true, 0);
    get_content_area()->set_border_width(0);


    show_all();
}

void SchematicPropertiesDialog::update_view()
{
    store->clear();
    {
        Gtk::TreeModel::Row row = *store->append();
        row[tree_columns.name] = "Global";
        row[tree_columns.block] = UUID();
        row[tree_columns.sheet] = UUID();
        view->get_selection()->select(row);
    }

    auto &blocks = doc.get_blocks();
    {
        Gtk::TreeModel::Row row = *store->append();
        row[tree_columns.name] = "Top block";
        row[tree_columns.block] = blocks.top_block;
        row[tree_columns.sheet] = UUID();
        sheets_to_row(row, blocks.get_top_block().schematic, blocks.top_block);
    }

    for (const auto &[block_uu, block] : blocks.blocks) {
        if (block_uu == blocks.top_block)
            continue;
        Gtk::TreeModel::Row row = *store->append();
        row[tree_columns.name] = block.block.name;
        row[tree_columns.block] = block_uu;
        row[tree_columns.sheet] = UUID();
        sheets_to_row(row, block.schematic, block_uu);
    }
    view->expand_all();
}


void SchematicPropertiesDialog::sheets_to_row(Gtk::TreeModel::Row &parent, const Schematic &s, const UUID &block_uuid)
{
    for (auto sh : s.get_sheets_sorted()) {
        auto row = *store->append(parent.children());
        row[tree_columns.name] = sh->name;
        row[tree_columns.sheet] = sh->uuid;
        row[tree_columns.block] = block_uuid;
    }
}

void SchematicPropertiesDialog::selection_changed()
{
    if (current) {
        delete current;
        current = nullptr;
    }
    if (auto it = view->get_selection()->get_selected()) {
        Gtk::TreeModel::Row row = *it;
        const auto sheet = row.get_value(tree_columns.sheet);
        const auto block = row.get_value(tree_columns.block);
        if (!sheet && !block) {
            current = Gtk::manage(new ProjectMetaEditor(sch.block->project_meta));
            current->property_margin() = 20;
        }
        else if (sheet && block) {
            current = Gtk::manage(new SheetEditor(doc.get_blocks().blocks.at(block).schematic.sheets.at(sheet),
                                                  doc.get_pool(), doc.get_pool_caching()));
        }
    }
    if (current) {
        current->show();
        box->pack_start(*current, true, true, 0);
    }
}

void SchematicPropertiesDialog::add_block()
{
    Dialogs dias;
    dias.set_parent(this);
    if (const auto r = dias.ask_datum_string("Enter new block name", "Channel")) {
        const auto uu = UUID::random();
        doc.get_blocks().blocks.emplace(std::piecewise_construct, std::forward_as_tuple(uu),
                                        std::forward_as_tuple(uu, *r));
        update_view();
    }
}

} // namespace horizon
