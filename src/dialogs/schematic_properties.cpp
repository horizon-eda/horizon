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
#include "util/changeable.hpp"

namespace horizon {

class SheetEditor : public Gtk::Box, public Changeable {
public:
    SheetEditor(Sheet &s, Block &b, IPool &pool, class IPool &pool_caching);
    std::pair<UUID, UUID> get_sheet_and_block() const;

private:
    Gtk::Grid *grid = nullptr;
    Sheet &sheet;
    Block &block;
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

SheetEditor::SheetEditor(Sheet &s, Block &b, IPool &pool, class IPool &pool_caching)
    : Gtk::Box(Gtk::ORIENTATION_VERTICAL, 0), sheet(s), block(b)
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
        s_signal_changed.emit();
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

std::pair<UUID, UUID> SheetEditor::get_sheet_and_block() const
{
    return {sheet.uuid, block.uuid};
}

SchematicPropertiesDialog::SchematicPropertiesDialog(Gtk::Window *parent, IDocumentSchematicBlockSymbol &adoc)
    : Gtk::Dialog("Schematic properties", *parent,
                  Gtk::DialogFlags::DIALOG_MODAL | Gtk::DialogFlags::DIALOG_USE_HEADER_BAR),
      doc(adoc)
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
        auto it = Gtk::manage(new Gtk::MenuItem("Block"));
        it->signal_activate().connect(sigc::mem_fun(*this, &SchematicPropertiesDialog::add_block));
        it->show();
        add_menu.append(*it);
    }
    {
        auto it = Gtk::manage(new Gtk::MenuItem("Sheet"));
        it->signal_activate().connect(sigc::mem_fun(*this, &SchematicPropertiesDialog::add_sheet));
        it->show();
        add_menu.append(*it);
        add_sheet_menu_item = it;
    }
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
            auto tb = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 1));
            tb->property_margin() = 4;
            {
                auto tbo = Gtk::manage(new Gtk::MenuButton());
                tbo->set_menu(add_menu);
                tbo->set_relief(Gtk::RELIEF_NONE);
                tbo->set_image_from_icon_name("list-add-symbolic");
                tb->pack_start(*tbo, false, false, 0);
            }
            {
                auto tbo = Gtk::manage(new Gtk::Button());
                tbo->set_relief(Gtk::RELIEF_NONE);
                tbo->set_image_from_icon_name("list-remove-symbolic");
                tbo->signal_clicked().connect(sigc::mem_fun(*this, &SchematicPropertiesDialog::handle_remove));
                tb->pack_start(*tbo, false, false, 0);
                remove_button = tbo;
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
    updating = true;
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
    updating = false;
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
    if (updating)
        return;
    if (current) {
        delete current;
        current = nullptr;
        sheet_editor = nullptr;
    }
    if (auto it = view->get_selection()->get_selected()) {
        Gtk::TreeModel::Row row = *it;
        const auto sheet = row.get_value(tree_columns.sheet);
        const auto block = row.get_value(tree_columns.block);
        if (!sheet && !block) {
            current = Gtk::manage(new ProjectMetaEditor(doc.get_blocks().get_top_block().block.project_meta));
            current->property_margin() = 20;
            remove_button->set_sensitive(false);
        }
        else if (sheet && block) {
            auto &b = doc.get_blocks().blocks.at(block);
            auto &sh = b.schematic.sheets.at(sheet);
            auto w = Gtk::manage(new SheetEditor(sh, b.block, doc.get_pool(), doc.get_pool_caching()));
            current = w;
            sheet_editor = w;
            w->signal_changed().connect(sigc::mem_fun(*this, &SchematicPropertiesDialog::update_for_sheet));
            remove_button->set_sensitive(sh.can_be_removed() && b.schematic.sheets.size() > 1);
        }
        else if (!sheet && block) {
            bool can_be_removed = true;
            if (block == doc.get_blocks().top_block) {
                can_be_removed = false;
            }
            else {
                for (const auto &[uu, b] : doc.get_blocks().blocks) {
                    if (std::any_of(b.block.block_instances.begin(), b.block.block_instances.end(),
                                    [&block](const auto &x) { return x.second.block->uuid == block; })) {
                        can_be_removed = false;
                        break;
                    }
                }
            }
            remove_button->set_sensitive(can_be_removed);
        }
        add_sheet_menu_item->set_sensitive(block);
    }
    if (current) {
        current->show();
        box->pack_start(*current, true, true, 0);
    }
}

void SchematicPropertiesDialog::update_for_sheet()
{
    const auto x = sheet_editor->get_sheet_and_block();
    const auto &sheet = x.first;
    const auto &block = x.second;
    store->foreach_iter([this, sheet, block](const Gtk::TreeIter &it) {
        Gtk::TreeModel::Row row = *it;
        if (row[tree_columns.sheet] == sheet && row[tree_columns.block] == block) {
            row[tree_columns.name] = doc.get_blocks().blocks.at(block).schematic.sheets.at(sheet).name;
            return true;
        }
        return false;
    });
}

void SchematicPropertiesDialog::add_block()
{
    Dialogs dias;
    dias.set_parent(this);
    if (const auto r = dias.ask_datum_string("Enter new block name", "Channel")) {
        auto &b = doc.get_blocks().add_block(*r);
        b.schematic.get_sheet_at_index(1).pool_frame =
                doc.get_blocks().get_top_block().schematic.get_sheet_at_index(1).pool_frame;
        b.symbol.create_template();
        update_view();
        store->foreach_iter([this, &b](const Gtk::TreeIter &it) {
            Gtk::TreeModel::Row row = *it;
            if (row[tree_columns.block] == b.uuid) {
                view->get_selection()->select(it);
                return true;
            }
            return false;
        });
    }
}

void SchematicPropertiesDialog::add_sheet()
{
    if (auto it_sel = view->get_selection()->get_selected()) {
        Gtk::TreeModel::Row row_sel = *it_sel;
        const auto block = row_sel.get_value(tree_columns.block);
        auto &sch = doc.get_blocks().blocks.at(block).schematic;
        auto &sheet = sch.add_sheet();
        const Frame *frame = nullptr;
        for (const auto sh : sch.get_sheets_sorted()) {
            if (sh->pool_frame)
                frame = sh->pool_frame;
        }
        sheet.pool_frame = frame;
        update_view();
        store->foreach_iter([this, &sheet, &block](const Gtk::TreeIter &it) {
            Gtk::TreeModel::Row row = *it;
            if (row[tree_columns.sheet] == sheet.uuid && row[tree_columns.block] == block) {
                view->get_selection()->select(it);
                return true;
            }
            return false;
        });
    }
}

void SchematicPropertiesDialog::handle_remove()
{
    if (auto it_sel = view->get_selection()->get_selected()) {
        Gtk::TreeModel::Row row_sel = *it_sel;
        const auto sheet = row_sel.get_value(tree_columns.sheet);
        const auto block = row_sel.get_value(tree_columns.block);
        if (sheet && block) {
            doc.get_blocks().blocks.at(block).schematic.delete_sheet(sheet);
            update_view();
            store->foreach_iter([this, &block](const Gtk::TreeIter &it) {
                Gtk::TreeModel::Row row = *it;
                if (row[tree_columns.sheet] == UUID() && row[tree_columns.block] == block) {
                    view->get_selection()->select(it);
                    return true;
                }
                return false;
            });
        }
        else if (!sheet && block) {
            doc.get_blocks().blocks.erase(block);
            update_view();
        }
    }
}

} // namespace horizon
