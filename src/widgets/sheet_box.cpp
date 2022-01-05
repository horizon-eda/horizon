#include "sheet_box.hpp"
#include <algorithm>
#include <iostream>
#include "core/core_schematic.hpp"

namespace horizon {

class CellRendererButton : public Gtk::CellRendererPixbuf {
public:
    using Gtk::CellRendererPixbuf::CellRendererPixbuf;

    typedef sigc::signal<void, Gtk::TreePath> type_signal_activate;
    type_signal_activate signal_activate()
    {
        return s_signal_activate;
    }

    bool activate_vfunc(GdkEvent *event, Gtk::Widget &widget, const Glib::ustring &path,
                        const Gdk::Rectangle &background_area, const Gdk::Rectangle &cell_area,
                        Gtk::CellRendererState flags) override
    {
        s_signal_activate.emit(Gtk::TreePath(path));
        return true;
    }

private:
    type_signal_activate s_signal_activate;
};

SheetBox::SheetBox(CoreSchematic &c) : Gtk::Box(Gtk::Orientation::ORIENTATION_VERTICAL, 0), core(c)
{
    auto *la = Gtk::manage(new Gtk::Label());
    la->set_markup("<b>Sheets</b>");
    la->show();
    pack_start(*la, false, false, 0);

    store = Gtk::TreeStore::create(tree_columns);
    view = Gtk::manage(new Gtk::TreeView(store));
    view->get_selection()->set_mode(Gtk::SELECTION_BROWSE);


    {
        auto tvc = Gtk::manage(new Gtk::TreeViewColumn("Sheet"));
        tvc->set_expand(true);
        {
            auto cr = Gtk::manage(new Gtk::CellRendererText());
            tvc->pack_start(*cr, true);
            tvc->add_attribute(*cr, "text", tree_columns.name);
            cr->property_editable().set_value(true);
            cr->signal_edited().connect(sigc::mem_fun(*this, &SheetBox::name_edited));
            tvc->set_cell_data_func(*cr, [this](Gtk::CellRenderer *tcr, const Gtk::TreeModel::iterator &it) {
                Gtk::TreeModel::Row row = *it;
                auto mcr = dynamic_cast<Gtk::CellRendererText *>(tcr);
                Pango::AttrList alist;
                if (row[tree_columns.sheet] == UUID()) {
                    auto attr = Pango::Attribute::create_attr_style(Pango::STYLE_ITALIC);
                    alist.insert(attr);
                }
                mcr->property_attributes() = alist;
            });
        }
        {
            auto cr = Gtk::manage(new Gtk::CellRendererPixbuf());
            cr->property_icon_name().set_value("dialog-warning-symbolic");
            cr->property_xalign() = 1;
            auto cr_hi = Gtk::manage(new Gtk::CellRendererPixbuf());
            cr_hi->property_icon_name().set_value("display-brightness-symbolic");
            cr_hi->property_xalign() = 1;
            tvc->pack_start(*cr_hi, false);
            tvc->pack_start(*cr, false);
            tvc->add_attribute(*cr, "visible", tree_columns.has_warnings);
            tvc->add_attribute(*cr_hi, "visible", tree_columns.has_highlights);
        }
        view->append_column(*tvc);
    }

    {
        auto tvc = Gtk::manage(new Gtk::TreeViewColumn(""));
        {
            auto cr = Gtk::manage(new Gtk::CellRendererText());
            tvc->pack_start(*cr, false);
            tvc->add_attribute(cr->property_text(), tree_columns.index);
            tvc->set_cell_data_func(*cr, [this](Gtk::CellRenderer *tcr, const Gtk::TreeModel::iterator &it) {
                Gtk::TreeModel::Row row = *it;
                tcr->property_visible() = row[tree_columns.index] > 0;
            });
        }
        view->append_column(*tvc);
        {
            auto cr = Gtk::manage(new CellRendererButton());
            cr->property_mode() = Gtk::CELL_RENDERER_MODE_ACTIVATABLE;
            cr->property_icon_name().set_value("insert-object-symbolic");
            tvc->set_cell_data_func(*cr, [this](Gtk::CellRenderer *tcr, const Gtk::TreeModel::iterator &it) {
                Gtk::TreeModel::Row row = *it;
                tcr->set_visible(row[tree_columns.type] == RowType::BLOCK);
            });
            tvc->pack_start(*cr, false);
            cr->signal_activate().connect([this](Gtk::TreePath path) {
                last_iter = view->get_selection()->get_selected();
                Gtk::TreeModel::Row row = *store->get_iter(path);
                s_signal_place_block.emit(row[tree_columns.block]);
            });
            cr_place_block = cr;

            view->set_has_tooltip(true);
            view->signal_query_tooltip().connect(
                    [this](int x, int y, bool keyboard_tooltip, const Glib::RefPtr<Gtk::Tooltip> &tooltip) {
                        if (keyboard_tooltip)
                            return false;
                        Gtk::TreeModel::Path path;
                        Gtk::TreeViewColumn *column;
                        int cell_x, cell_y;
                        int bx, by;
                        view->convert_widget_to_bin_window_coords(x, y, bx, by);
                        if (!view->get_path_at_pos(bx, by, path, column, cell_x, cell_y))
                            return false;
                        auto cells = column->get_cells();
                        for (auto cell : cells) {
                            int start, width;
                            if (column->get_cell_position(*cell, start, width)) {
                                if ((cell_x >= start) && (cell_x < (start + width))) {
                                    if (cell == cr_place_block) {
                                        tooltip->set_text("Place block instance");
                                        view->set_tooltip_cell(tooltip, &path, column, cell);
                                        return true;
                                    }
                                    break;
                                }
                            }
                        }
                        return false;
                    });
        }
    }

    view->show();
    auto sc = Gtk::manage(new Gtk::ScrolledWindow());
    sc->set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC);
    sc->set_shadow_type(Gtk::SHADOW_IN);
    sc->set_min_content_height(150);
    sc->set_propagate_natural_height(true);
    sc->add(*view);
    sc->show_all();

    view->set_headers_visible(false);
    view->get_selection()->signal_changed().connect(sigc::mem_fun(*this, &SheetBox::selection_changed));
    pack_start(*sc, true, true, 0);

    {
        auto item = Gtk::manage(new Gtk::MenuItem("Copy UUID"));
        item->signal_activate().connect([this] {
            auto it = view->get_selection()->get_selected();
            if (it) {
                Gtk::TreeModel::Row row = *it;
                horizon::UUID uuid = row[tree_columns.sheet];
                std::string str = uuid;
                auto clip = Gtk::Clipboard::get();
                clip->set_text(str);
            }
        });
        item->show();
        menu.append(*item);
    }

    view->signal_button_press_event().connect_notify([this](GdkEventButton *ev) {
        Gtk::TreeModel::Path path;
        if (ev->button == 3) {
#if GTK_CHECK_VERSION(3, 22, 0)
            menu.popup_at_pointer((GdkEvent *)ev);
#else
            menu.popup(ev->button, gtk_get_current_event_time());
#endif
        }
    });

    view->set_row_separator_func(
            [this](const Glib::RefPtr<Gtk::TreeModel> &model, const Gtk::TreeModel::iterator &iter) {
                Gtk::TreeModel::Row row = *iter;
                return row[tree_columns.type] == RowType::SEPARATOR;
            });

    view->get_selection()->set_select_function([this](const Glib::RefPtr<Gtk::TreeModel> &,
                                                      const Gtk::TreeModel::Path &path, bool path_currently_selected) {
        auto iter = store->get_iter(path);
        Gtk::TreeModel::Row row = *iter;
        const auto type = row.get_value(tree_columns.type);
        return type == RowType::SHEET || type == RowType::BLOCK;
    });
    {
        auto tb = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 0));
        tb->get_style_context()->add_class("inline-toolbar");
        {
            auto box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 0));
            tb->pack_start(*box, false, false, 0);
            {
                auto tbo = Gtk::manage(new Gtk::Button());
                tbo->set_tooltip_text("Add sheet");
                tbo->set_image_from_icon_name("list-add-symbolic");
                tbo->signal_clicked().connect([this] { core.add_sheet(); });
                box->pack_start(*tbo, false, false, 0);
                add_button = tbo;
            }
            {
                auto tbo = Gtk::manage(new Gtk::Button());
                tbo->set_tooltip_text("Remove current sheet");
                tbo->set_image_from_icon_name("list-remove-symbolic");
                tbo->signal_clicked().connect(sigc::mem_fun(*this, &SheetBox::remove_clicked));
                box->pack_start(*tbo, false, false, 0);
                remove_button = tbo;
            }
            {
                auto tbo = Gtk::manage(new Gtk::Button());
                tbo->set_tooltip_text("Move current sheet up");
                tbo->set_image_from_icon_name("go-up-symbolic");
                tbo->signal_clicked().connect([this] { sheet_move(-1); });
                box->pack_start(*tbo, false, false, 0);
                move_up_button = tbo;
            }
            {
                auto tbo = Gtk::manage(new Gtk::Button());
                tbo->set_tooltip_text("Move current sheet down");
                tbo->set_image_from_icon_name("go-down-symbolic");
                tbo->signal_clicked().connect([this] { sheet_move(1); });
                box->pack_start(*tbo, false, false, 0);
                move_down_button = tbo;
            }
        }
        {
            auto box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 0));
            tb->pack_end(*box, false, false, 0);
            auto tbo = Gtk::manage(new Gtk::Button());
            tbo->set_tooltip_text("Schematic properties");
            tbo->set_image_from_icon_name("view-more-symbolic");
            tbo->signal_clicked().connect([this] { s_signal_edit_more.emit(); });
            box->pack_start(*tbo, false, false, 0);
        }
        tb->show_all();
        pack_start(*tb, false, false, 0);
    }
    update();
    selection_changed();
}

void SheetBox::sheet_move(int dir)
{
    auto it = view->get_selection()->get_selected();
    if (it) {
        Gtk::TreeModel::Row row = *it;
        auto sch = core.get_current_schematic();
        auto &sheets = sch->sheets;
        auto *sheet = &sch->sheets.at(row[tree_columns.sheet]);
        if (dir < 0 && sheet->index == 1)
            return;
        if (dir > 0 && sheet->index == sch->sheets.size())
            return;
        auto sheet_other = std::find_if(sheets.begin(), sheets.end(),
                                        [sheet, dir](const auto x) { return x.second.index == sheet->index + dir; });
        assert(sheet_other != sheets.end());
        std::swap(sheet_other->second.index, sheet->index);
        core.set_needs_save();
        core.rebuild("reorder sheet");
    }
}

void SheetBox::remove_clicked()
{
    auto it = view->get_selection()->get_selected();
    if (it) {
        Gtk::TreeModel::Row row = *it;
        core.delete_sheet(row[tree_columns.sheet]);
        selection_changed();
    }
}

void SheetBox::selection_changed()
{
    if (last_iter) {
        view->get_selection()->select(*last_iter);
        last_iter.reset();
        return;
    }
    if (updating)
        return;
    auto it = view->get_selection()->get_selected();
    if (it) {
        Gtk::TreeModel::Row row = *it;
        const auto &sheets = core.get_blocks().blocks.at(row[tree_columns.block]).schematic.sheets;
        const auto sheet_uu = row.get_value(tree_columns.sheet);
        cr_place_block->set_sensitive(sheet_uu);
        if (sheet_uu == UUID()) { // block symbol
            signal_select_sheet().emit(UUID(), row[tree_columns.block], {});
            remove_button->set_sensitive(false);
            move_up_button->set_sensitive(false);
            move_down_button->set_sensitive(false);
            add_button->set_sensitive(false);
        }
        else if (sheets.count(sheet_uu)) {
            const auto &sh = sheets.at(row[tree_columns.sheet]);
            signal_select_sheet().emit(sh.uuid, row[tree_columns.block], row[tree_columns.instance_path]);
            remove_button->set_sensitive(sh.can_be_removed() && sheets.size() > 1);
            move_up_button->set_sensitive(sh.index != 1);
            move_down_button->set_sensitive(sh.index != sheets.size());
            add_button->set_sensitive(true);
        }
    }
}

void SheetBox::name_edited(const Glib::ustring &path, const Glib::ustring &new_text)
{
    auto it = store->get_iter(path);
    if (it) {
        Gtk::TreeModel::Row row = *it;
        const auto type = row.get_value(tree_columns.type);
        if (type == RowType::SHEET) {
            auto &sh = core.get_blocks().get_schematic(row[tree_columns.block]).sheets.at(row[tree_columns.sheet]);
            if (sh.name != new_text) {
                sh.name = new_text;
                core.set_needs_save();
                core.rebuild("rename sheet");
            }
        }
        else if (type == RowType::BLOCK) {
            auto &bl = core.get_blocks().get_block(row[tree_columns.block]);
            if (bl.name != new_text) {
                bl.name = new_text;
                core.set_needs_save();
                core.rebuild("rename block");
            }
        }
    }
}

void SheetBox::clear_highlights()
{

    store->foreach_iter([this](const Gtk::TreeIter &it) {
        Gtk::TreeModel::Row row = *it;
        row[tree_columns.has_highlights] = false;
        return false;
    });
}

void SheetBox::add_highlights(const UUID &sheet, const UUIDVec &path)
{
    store->foreach_iter([this, &path, &sheet](const Gtk::TreeIter &it) {
        Gtk::TreeModel::Row row = *it;
        const auto type = row.get_value(tree_columns.type);
        if (row.get_value(tree_columns.instance_path) == path
            && ((row.get_value(tree_columns.sheet) == sheet && type == RowType::SHEET)
                || type == RowType::BLOCK_INSTANCE)) {
            row[tree_columns.has_highlights] = true;
        }
        return false;
    });
}

void SheetBox::sheets_to_row(std::function<Gtk::TreeModel::Row()> make_row, const Schematic &sch,
                             const UUID &block_uuid, const UUIDVec &instance_path, bool in_hierarchy)
{
    if (Block::instance_path_too_long(instance_path, __FUNCTION__))
        return;
    for (auto sh : sch.get_sheets_sorted()) {
        auto row = make_row();
        row[tree_columns.name] = sh->name;
        row[tree_columns.type] = RowType::SHEET;
        row[tree_columns.sheet] = sh->uuid;
        row[tree_columns.block] = block_uuid;
        row[tree_columns.has_warnings] = sh->warnings.size() > 0;
        if (in_hierarchy)
            row[tree_columns.index] = core.get_sheet_index_for_path(sh->uuid, instance_path);
        else
            row[tree_columns.index] = sh->index;
        row[tree_columns.instance_path] = instance_path;
        if (in_hierarchy) {
            for (const auto sym : sh->get_block_symbols_sorted()) {
                Gtk::TreeModel::Row row_inst = *store->append(row.children());
                row_inst[tree_columns.name] = sym->block_instance->refdes;
                row_inst[tree_columns.type] = RowType::BLOCK_INSTANCE;
                row_inst[tree_columns.block] = UUID();
                row_inst[tree_columns.sheet] = UUID();
                row_inst[tree_columns.index] = 0;
                const auto path = uuid_vec_append(instance_path, sym->block_instance->uuid);
                row_inst[tree_columns.instance_path] = path;

                sheets_to_row([this, &row_inst] { return *store->append(row_inst.children()); },
                              core.get_blocks().blocks.at(sym->block_instance->block->uuid).schematic,
                              sym->block_instance->block->uuid, path, true);
            }
        }
    }
}

void SheetBox::update()
{
    const auto current_sheet = core.get_block_symbol_mode() ? UUID() : core.get_sheet()->uuid;
    const auto current_block = core.get_current_block_uuid();
    const auto current_inst = core.get_instance_path();

    std::set<UUID> blocks_expanded;
    for (const auto &it : store->children()) {
        const auto path = store->get_path(it);
        if (view->row_expanded(path)) {
            Gtk::TreeModel::Row row = *it;
            if (row[tree_columns.type] == RowType::BLOCK)
                blocks_expanded.insert(row[tree_columns.block]);
        }
    }

    std::set<std::pair<UUIDVec, UUID>> sheets_expanded;
    store->foreach_iter([this, &sheets_expanded](const Gtk::TreeIter &it) {
        Gtk::TreeModel::Row row = *it;
        const auto type = row.get_value(tree_columns.type);
        const auto path = store->get_path(it);
        if ((type == RowType::SHEET || type == RowType::BLOCK_INSTANCE) && view->row_expanded(path)) {
            sheets_expanded.emplace(row.get_value(tree_columns.instance_path), row.get_value(tree_columns.sheet));
        }
        return false;
    });


    updating = true;
    store->clear();
    sheets_to_row([this] { return *store->append(); }, *core.get_top_schematic(), core.get_top_block_uuid(), {}, true);

    const auto &blocks = core.get_blocks();
    if (blocks.blocks.size() > 1) {
        Gtk::TreeModel::Row row_blocks = *store->append();
        row_blocks[tree_columns.type] = RowType::SEPARATOR;
        row_blocks[tree_columns.sheet] = UUID();
        row_blocks[tree_columns.block] = UUID();
        row_blocks[tree_columns.has_warnings] = false;
        row_blocks[tree_columns.index] = 0;
        for (const auto &[block_uu, block] : blocks.blocks) {
            if (block_uu == blocks.top_block)
                continue;

            auto iter = store->append();
            Gtk::TreeModel::Row row_block = *iter;
            row_block[tree_columns.name] = block.block.name;
            row_block[tree_columns.type] = RowType::BLOCK;
            row_block[tree_columns.sheet] = UUID();
            row_block[tree_columns.block] = block_uu;
            row_block[tree_columns.has_warnings] = false;
            row_block[tree_columns.has_highlights] = false;
            row_block[tree_columns.index] = 0;

            sheets_to_row([this, &row_block] { return *store->append(row_block.children()); }, block.schematic,
                          block_uu, {}, false);

            if (blocks_expanded.count(block_uu)) {
                const auto path = store->get_path(iter);
                view->expand_row(path, false);
            }
        }
    }
    updating = false;

    store->foreach_iter([this, &current_sheet, &current_block, &current_inst](const Gtk::TreeIter &it) {
        Gtk::TreeModel::Row row = *it;
        if (row[tree_columns.sheet] == current_sheet && row[tree_columns.block] == current_block
            && row.get_value(tree_columns.instance_path) == current_inst) {
            view->expand_to_path(store->get_path(it));
            view->get_selection()->select(it);
            return true;
        }
        return false;
    });

    store->foreach_iter([this, &sheets_expanded](const Gtk::TreeIter &it) {
        Gtk::TreeModel::Row row = *it;
        const auto type = row.get_value(tree_columns.type);
        if (type == RowType::SHEET || type == RowType::BLOCK_INSTANCE) {
            if (sheets_expanded.count({row.get_value(tree_columns.instance_path), row.get_value(tree_columns.sheet)}))
                view->expand_row(store->get_path(it), false);
        }
        return false;
    });
}

void SheetBox::select_sheet(const UUID &sheet)
{
    go_to_instance(core.get_instance_path(), sheet);
}

void SheetBox::go_to_instance(const UUIDVec &path, const UUID &sheet)
{
    store->foreach_iter([this, &path, &sheet](const Gtk::TreeIter &it) {
        Gtk::TreeModel::Row row = *it;
        if (row.get_value(tree_columns.instance_path) == path && row.get_value(tree_columns.type) == RowType::SHEET) {
            bool found = true;
            if (sheet)
                found = row[tree_columns.sheet] == sheet;

            if (found) {
                view->expand_to_path(store->get_path(it));
                view->get_selection()->select(it);
                return true;
            }
        }
        return false;
    });
}

void SheetBox::go_to_block_symbol(const UUID &block)
{
    for (auto &it : store->children()) {
        Gtk::TreeModel::Row row = *it;
        if (row.get_value(tree_columns.type) == RowType::BLOCK && row.get_value(tree_columns.block) == block) {
            view->get_selection()->select(row);
            return;
        }
    }
}


} // namespace horizon
