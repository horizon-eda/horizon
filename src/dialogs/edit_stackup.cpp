#include "edit_stackup.hpp"
#include "board/board.hpp"
#include "board/board_layers.hpp"
#include "widgets/spin_button_dim.hpp"
#include "util/util.hpp"
#include "util/gtk_util.hpp"
#include "widgets/generic_combo_box.hpp"
#include "document/idocument_board.hpp"
#include "preferences/preferences.hpp"
#include "preferences/preferences_provider.hpp"
#include <iostream>

namespace horizon {

class StackupLayerEditor : public Gtk::Box {
public:
    StackupLayerEditor(EditStackupDialog &parent, int la);
    const int layer;
    void reload();

    typedef sigc::signal<void, Board::UserLayerOrder> type_signal_add_user_layer;
    type_signal_add_user_layer signal_add_user_layer()
    {
        return s_signal_add_user_layer;
    }

    typedef sigc::signal<void> type_signal_delete_user_layer;
    type_signal_delete_user_layer signal_delete_user_layer()
    {
        return s_signal_delete_user_layer;
    }

    typedef sigc::signal<void, int> type_signal_move_user_layer;
    type_signal_move_user_layer signal_move_user_layer()
    {
        return s_signal_move_user_layer;
    }

    void set_can_add_user_layer(bool can_add);

private:
    Gtk::Label *la = nullptr;
    Gtk::Entry *en = nullptr;
    SpinButtonDim *sp_height = nullptr;
    SpinButtonDim *sp_sub_height = nullptr;
    Gtk::Grid *grid = nullptr;
    Gtk::Box *add_user_layer_box = nullptr;
    Gtk::Box *move_user_layer_box = nullptr;
    Gtk::Box *delete_user_layer_box = nullptr;
    GenericComboBox<Board::UserLayer::Type> *type_combo = nullptr;
    bool reloading = false;
    Gtk::Menu menu;
    std::map<int, Gtk::RadioMenuItem *> color_radios;

    type_signal_add_user_layer s_signal_add_user_layer;
    type_signal_delete_user_layer s_signal_delete_user_layer;
    type_signal_move_user_layer s_signal_move_user_layer;

    EditStackupDialog &parent;
};

StackupLayerEditor::StackupLayerEditor(EditStackupDialog &pa, int ly)
    : Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 10), layer(ly), parent(pa)
{
    if (!BoardLayers::is_user(layer)) {
        auto colorbox = Gtk::manage(new Gtk::DrawingArea);
        colorbox->set_size_request(20, -1);
        colorbox->show();
        colorbox->signal_draw().connect([this](const Cairo::RefPtr<Cairo::Context> &cr) -> bool {
            auto &colors = PreferencesProvider::get_prefs().canvas_layer.appearance.layer_colors;
            if (colors.count(layer)) {
                auto layer_color = colors.at(layer);
                cr->set_source_rgb(layer_color.r, layer_color.g, layer_color.b);
            }
            cr->paint();
            return true;
        });
        pack_start(*colorbox, false, false, 0);
        parent.sg_color_box->add_widget(*colorbox);
    }
    else {
        auto colorbutton = Gtk::manage(new Gtk::MenuButton);
        colorbutton->set_menu(menu);
        auto colorbox = Gtk::manage(new Gtk::DrawingArea);
        colorbox->set_size_request(20, -1);
        colorbox->show();

        colorbox->signal_draw().connect([this](const Cairo::RefPtr<Cairo::Context> &cr) -> bool {
            auto &colors = PreferencesProvider::get_prefs().canvas_layer.appearance.layer_colors;
            if (parent.board.get_user_layers().count(layer)) {
                auto layer_color = colors.at(parent.board.get_user_layers().at(layer).id_color);
                cr->set_source_rgb(layer_color.r, layer_color.g, layer_color.b);
            }
            cr->paint();
            return true;
        });
        colorbutton->add(*colorbox);
        pack_start(*colorbutton, false, false, 0);
        parent.sg_color_box->add_widget(*colorbutton);


        Gtk::RadioMenuItem *group = nullptr;
        for (int l = BoardLayers::FIRST_USER_LAYER; l <= BoardLayers::LAST_USER_LAYER; l++) {
            auto it = Gtk::manage(new Gtk::RadioMenuItem());
            color_radios.emplace(l, it);
            auto b = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 5));
            auto la = Gtk::manage(new Gtk::Label(BoardLayers::get_layer_name(l)));
            la->set_xalign(0);
            auto cb = Gtk::manage(new Gtk::DrawingArea);
            cb->set_size_request(18, 18);
            cb->signal_draw().connect([l](const Cairo::RefPtr<Cairo::Context> &cr) -> bool {
                auto &colors = PreferencesProvider::get_prefs().canvas_layer.appearance.layer_colors;
                if (colors.count(l)) {
                    auto layer_color = colors.at(l);
                    cr->set_source_rgb(layer_color.r, layer_color.g, layer_color.b);
                }
                cr->paint();
                return true;
            });


            b->pack_start(*cb, false, false, 0);
            b->pack_start(*la, false, false, 0);
            it->add(*b);
            if (!group)
                group = it;
            else
                it->join_group(*group);
            it->show_all();
            menu.append(*it);
            it->signal_toggled().connect([this, it, l] {
                if (reloading)
                    return;
                if (it->get_active())
                    parent.board.set_user_layer_color(layer, l);
            });
        }
    }

    if (BoardLayers::is_user(layer)) {
        en = Gtk::manage(new Gtk::Entry);
        en->set_valign(Gtk::ALIGN_CENTER);
        en->signal_changed().connect([this] {
            if (reloading)
                return;
            parent.board.set_user_layer_name(layer, en->get_text());
        });
        parent.sg_layer_name->add_widget(*en);
        pack_start(*en, true, true, 0);
    }
    else {
        la = Gtk::manage(new Gtk::Label());
        parent.sg_layer_name->add_widget(*la);
        la->set_xalign(0);
        pack_start(*la, true, true, 0);
    }

    grid = Gtk::manage(new Gtk::Grid);
    grid->set_row_spacing(5);
    grid->set_column_spacing(5);
    grid->set_valign(Gtk::ALIGN_CENTER);
    parent.sg_layer_grid->add_widget(*grid);


    if (BoardLayers::is_copper(layer)) {
        sp_height = Gtk::manage(new SpinButtonDim());
        parent.sg_adj->add_widget(*sp_height);
        widget_remove_scroll_events(*sp_height);
        sp_height->set_range(0, 10_mm);

        sp_height->signal_value_changed().connect([this] {
            if (reloading)
                return;
            parent.board.stackup.at(layer).thickness = sp_height->get_value_as_int();
        });
        {
            auto hl = Gtk::manage(new Gtk::Label("Height"));
            hl->set_hexpand(true);
            hl->set_xalign(1);
            hl->get_style_context()->add_class("dim-label");
            grid->attach(*hl, 0, 0, 1, 1);
        }
        grid->attach(*sp_height, 1, 0, 1, 1);
    }
    if (BoardLayers::is_copper(layer) && layer != BoardLayers::BOTTOM_COPPER) {
        sp_sub_height = Gtk::manage(new SpinButtonDim());
        parent.sg_adj->add_widget(*sp_sub_height);
        widget_remove_scroll_events(*sp_sub_height);
        sp_sub_height->set_range(0, 10_mm);

        sp_sub_height->signal_value_changed().connect([this] {
            if (reloading)
                return;
            parent.board.stackup.at(layer).substrate_thickness = sp_sub_height->get_value_as_int();
        });
        {
            auto hl = Gtk::manage(new Gtk::Label("Substrate height"));
            hl->set_hexpand(true);

            hl->set_xalign(1);
            hl->get_style_context()->add_class("dim-label");
            grid->attach(*hl, 0, 1, 1, 1);
        }
        grid->attach(*sp_sub_height, 1, 1, 1, 1);
    }
    else if (BoardLayers::is_user(layer)) {
        type_combo = Gtk::manage(new GenericComboBox<Board::UserLayer::Type>());
        parent.sg_adj->add_widget(*type_combo);
        widget_remove_scroll_events(*type_combo);
        type_combo->append(Board::UserLayer::Type::DOCUMENTATION, "Documentation");
        type_combo->append(Board::UserLayer::Type::STIFFENER, "Stiffener");
        type_combo->append(Board::UserLayer::Type::BEND_AREA, "Bend area");
        type_combo->append(Board::UserLayer::Type::FLEX_AREA, "Flex area");
        type_combo->append(Board::UserLayer::Type::RIGID_AREA, "Rigid area");
        type_combo->append(Board::UserLayer::Type::CARBON_MASK, "Carbon mask");
        type_combo->append(Board::UserLayer::Type::SILVER_MASK, "Silver mask");
        type_combo->append(Board::UserLayer::Type::COVERCOAT, "Covercoat");
        type_combo->append(Board::UserLayer::Type::COVERLAY, "Coverlay");
        type_combo->append(Board::UserLayer::Type::PSA, "PSA");

        type_combo->signal_changed().connect([this] {
            if (reloading)
                return;
            parent.board.set_user_layer_type(layer, type_combo->get_active_key());
        });

        grid->attach(*type_combo, 1, 1, 1, 1);
        {
            auto hl = Gtk::manage(new Gtk::Label("Type"));
            hl->set_hexpand(true);

            hl->set_xalign(1);
            hl->get_style_context()->add_class("dim-label");
            grid->attach(*hl, 0, 1, 1, 1);
        }
    }


    set_margin_start(8);
    set_margin_end(8);
    set_margin_top(4);
    set_margin_bottom(4);

    pack_start(*grid, true, true, 0);
    add_user_layer_box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL));
    move_user_layer_box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL));
    delete_user_layer_box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL));
    parent.sg_layer_add->add_widget(*add_user_layer_box);
    parent.sg_layer_delete->add_widget(*delete_user_layer_box);
    parent.sg_layer_move->add_widget(*move_user_layer_box);

    if (BoardLayers::is_user(layer)) {
        move_user_layer_box->get_style_context()->add_class("linked");
        auto move_down_button = Gtk::manage(new Gtk::Button);
        move_down_button->set_image_from_icon_name("go-down-symbolic", Gtk::ICON_SIZE_BUTTON);
        move_down_button->set_tooltip_text("Move down");
        move_down_button->signal_clicked().connect([this] { s_signal_move_user_layer.emit(-1); });
        move_user_layer_box->pack_start(*move_down_button, true, true, 0);


        auto move_up_button = Gtk::manage(new Gtk::Button);
        move_up_button->set_image_from_icon_name("go-up-symbolic", Gtk::ICON_SIZE_BUTTON);
        move_up_button->set_tooltip_text("Move up");
        move_up_button->signal_clicked().connect([this] { s_signal_move_user_layer.emit(1); });
        move_user_layer_box->pack_start(*move_up_button, true, true, 0);
    }
    pack_start(*move_user_layer_box, false, false, 0);

    {
        auto add_menu_button = Gtk::manage(new Gtk::MenuButton);
        add_menu_button->set_valign(Gtk::ALIGN_CENTER);

        add_user_layer_box->pack_start(*add_menu_button, true, true, 0);
        add_menu_button->set_image_from_icon_name("list-add-symbolic", Gtk::ICON_SIZE_BUTTON);

        auto action_group = Gio::SimpleActionGroup::create();
        insert_action_group("layer", action_group);
        action_group->add_action("add_above", [this] { s_signal_add_user_layer.emit(Board::UserLayerOrder::ABOVE); });
        action_group->add_action("add_below", [this] { s_signal_add_user_layer.emit(Board::UserLayerOrder::BELOW); });

        auto menu = Gio::Menu::create();
        add_menu_button->set_menu_model(menu);
        add_menu_button->get_popover()->set_position(Gtk::POS_LEFT);
        menu->append("Add user layer above", "layer.add_above");
        menu->append("Add user layer below", "layer.add_below");
    }
    pack_start(*add_user_layer_box, false, false, 0);


    if (BoardLayers::is_user(layer)) {
        auto delete_button = Gtk::manage(new Gtk::Button);
        delete_button->set_valign(Gtk::ALIGN_CENTER);
        delete_button->set_image_from_icon_name("list-remove-symbolic", Gtk::ICON_SIZE_BUTTON);
        delete_button->signal_clicked().connect([this] { s_signal_delete_user_layer.emit(); });
        delete_user_layer_box->pack_start(*delete_button, true, true, 0);
    }
    pack_start(*delete_user_layer_box, false, false, 0);


    show_all();

    reload();
}

void StackupLayerEditor::reload()
{
    reloading = true;
    std::string label_str = "Layer " + std::to_string(layer);
    if (parent.board.get_layers().count(layer))
        label_str = parent.board.get_layers().at(layer).name;
    if (la)
        la->set_label(label_str);
    if (en)
        en->set_text(label_str);
    if (parent.board.stackup.count(layer)) {
        if (sp_sub_height)
            sp_sub_height->set_value(parent.board.stackup.at(layer).substrate_thickness);
        if (sp_height)
            sp_height->set_value(parent.board.stackup.at(layer).thickness);
    }
    auto &user_layers = parent.board.get_user_layers();
    if (user_layers.count(layer)) {
        auto &ul = user_layers.at(layer);
        if (type_combo)
            type_combo->set_active_key(ul.type);
        if (color_radios.count(ul.id_color))
            color_radios.at(ul.id_color)->set_active(true);
    }


    reloading = false;
    queue_draw();
}

void StackupLayerEditor::set_can_add_user_layer(bool can_add)
{
    add_user_layer_box->set_sensitive(can_add);
    if (can_add)
        add_user_layer_box->set_has_tooltip(false);
    else
        add_user_layer_box->set_tooltip_text("No more user layers available");
}


EditStackupDialog::EditStackupDialog(Gtk::Window *parent, IDocumentBoard &c)
    : Gtk::Dialog("Edit Stackup", *parent, Gtk::DialogFlags::DIALOG_MODAL | Gtk::DialogFlags::DIALOG_USE_HEADER_BAR),
      core(c), board(*core.get_board())
{
    add_button("Cancel", Gtk::ResponseType::RESPONSE_CANCEL);
    auto ok_button = add_button("OK", Gtk::ResponseType::RESPONSE_OK);
    ok_button->signal_clicked().connect(sigc::mem_fun(*this, &EditStackupDialog::ok_clicked));
    set_default_response(Gtk::ResponseType::RESPONSE_OK);
    set_default_size(400, 700);

    sg_color_box = Gtk::SizeGroup::create(Gtk::SIZE_GROUP_HORIZONTAL);
    sg_layer_name = Gtk::SizeGroup::create(Gtk::SIZE_GROUP_HORIZONTAL);
    sg_layer_grid = Gtk::SizeGroup::create(Gtk::SIZE_GROUP_HORIZONTAL);
    sg_layer_move = Gtk::SizeGroup::create(Gtk::SIZE_GROUP_HORIZONTAL);
    sg_layer_add = Gtk::SizeGroup::create(Gtk::SIZE_GROUP_HORIZONTAL);
    sg_layer_delete = Gtk::SizeGroup::create(Gtk::SIZE_GROUP_HORIZONTAL);
    sg_adj = Gtk::SizeGroup::create(Gtk::SIZE_GROUP_HORIZONTAL);

    auto box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL));
    auto box2 = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 8));
    box2->property_margin() = 8;
    auto la = Gtk::manage(new Gtk::Label("Inner Layers"));
    la->get_style_context()->add_class("dim-label");
    box2->pack_start(*la, false, false, 0);

    sp_n_inner_layers = Gtk::manage(new Gtk::SpinButton());
    sp_n_inner_layers->set_range(0, BoardLayers::max_inner_layers);
    sp_n_inner_layers->set_digits(0);
    sp_n_inner_layers->set_increments(1, 1);
    sp_n_inner_layers->set_value(board.get_n_inner_layers());
    sp_n_inner_layers->signal_value_changed().connect(sigc::mem_fun(*this, &EditStackupDialog::update_layers));
    box2->pack_start(*sp_n_inner_layers, true, true, 0);

    box->pack_start(*box2, false, false, 0);

    {
        auto sep = Gtk::manage(new Gtk::Separator(Gtk::ORIENTATION_HORIZONTAL));
        box->pack_start(*sep, false, false, 0);
    }

    auto sc = Gtk::manage(new Gtk::ScrolledWindow);
    sc->set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC);

    lb = Gtk::manage(new Gtk::ListBox);
    lb->get_style_context()->add_class("stackup-editor");
    lb->set_selection_mode(Gtk::SELECTION_NONE);
    sc->add(*lb);

    box->pack_start(*sc, true, true, 0);

    get_content_area()->pack_start(*box, true, true, 0);
    get_content_area()->set_border_width(0);


    show_all();

    create_editors();
    update_layers();
    lb->set_sort_func([this](Gtk::ListBoxRow *ra, Gtk::ListBoxRow *rb) {
        auto &ca = dynamic_cast<StackupLayerEditor &>(*ra->get_child());
        auto &cb = dynamic_cast<StackupLayerEditor &>(*rb->get_child());
        double pa = 0, pb = 0;
        auto &layers = board.get_layers();
        if (layers.count(ca.layer))
            pa = layers.at(ca.layer).position;
        if (layers.count(cb.layer))
            pb = layers.at(cb.layer).position;
        if (pa < pb)
            return 1;
        else if (pa > pb)
            return -1;
        else
            return 0;
    });

    lb->set_filter_func([this](Gtk::ListBoxRow *row) {
        auto &c = dynamic_cast<StackupLayerEditor &>(*row->get_child());
        return board.get_layers().count(c.layer);
    });
}

void EditStackupDialog::create_editor(int layer)
{
    auto ed = Gtk::manage(new StackupLayerEditor(*this, layer));
    ed->signal_add_user_layer().connect([this, ed](Board::UserLayerOrder order) {
        board.add_user_layer(ed->layer, order);
        reload();
    });
    ed->signal_delete_user_layer().connect([this, ed] {
        board.delete_user_layer(ed->layer);
        reload();
    });
    ed->signal_move_user_layer().connect([this, ed](int dir) {
        auto other = board.get_adjacent_layer(ed->layer, dir);
        if (other == ed->layer)
            return;
        board.move_user_layer(ed->layer, other, dir > 0 ? Board::UserLayerOrder::ABOVE : Board::UserLayerOrder::BELOW);
        reload();
    });
    lb->add(*ed);
    ed->show();
}

void EditStackupDialog::create_editors()
{
    std::set<int> layers;
    for (const auto &[i, l] : board.get_layers()) {
        layers.insert(i);
    }
    for (unsigned int i = 0; i < BoardLayers::max_inner_layers; i++) {
        const int layer = -i - 1;
        layers.insert(layer);
    }
    for (int l = BoardLayers::FIRST_USER_LAYER; l <= BoardLayers::LAST_USER_LAYER; l++) {
        layers.insert(l);
    }
    for (const auto l : layers) {
        create_editor(l);
    }
    lb->invalidate_filter();
    lb->invalidate_sort();

    /*
    create_editor(BoardLayers::TOP_COPPER, true);
    create_editor(BoardLayers::TOP_COPPER, false);

    for (unsigned int i = 0; i < BoardLayers::max_inner_layers; i++) {
        const int layer = -i - 1;
        for (const auto cu : {true, false}) {
            create_editor(layer, cu);
        }
    }

    create_editor(BoardLayers::BOTTOM_COPPER, true);
    */
}

void EditStackupDialog::update_layers()
{
    auto n_inner_layers = sp_n_inner_layers->get_value_as_int();
    board.set_n_inner_layers(n_inner_layers);
    reload();
}

void EditStackupDialog::reload()
{
    const auto can_add_user_layer = board.count_available_user_layers() > 0;
    for (auto ch : lb->get_children()) {
        auto &ed = dynamic_cast<StackupLayerEditor &>(*dynamic_cast<Gtk::ListBoxRow &>(*ch).get_child());
        ed.reload();
        ed.set_can_add_user_layer(can_add_user_layer);
    }
    lb->invalidate_filter();
    lb->invalidate_sort();
}

void EditStackupDialog::ok_clicked()
{
    auto n_inner_layers = sp_n_inner_layers->get_value_as_int();
    board.set_n_inner_layers(n_inner_layers);
    /* for (auto ch : lb->get_children()) {
         if (ch->get_visible()) {
             auto &ed = dynamic_cast<StackupLayerEditor &>(*dynamic_cast<Gtk::ListBoxRow &>(*ch).get_child());
             if (ed.copper) {
                 board.stackup.at(ed.layer).thickness = ed.sp->get_value_as_int();
             }
             else {
                 board.stackup.at(ed.layer).substrate_thickness = ed.sp->get_value_as_int();
             }
         }
     }*/
    auto &rules = dynamic_cast<BoardRules &>(*core.get_rules());
    rules.update_for_board(board);
    map_erase_if(board.tracks, [this](const auto &x) { return board.get_layers().count(x.second.layer) == 0; });
    map_erase_if(board.polygons, [this](const auto &x) { return board.get_layers().count(x.second.layer) == 0; });
    map_erase_if(board.lines, [this](const auto &x) { return board.get_layers().count(x.second.layer) == 0; });
    map_erase_if(board.texts, [this](const auto &x) { return board.get_layers().count(x.second.layer) == 0; });
    core.get_gerber_output_settings().update_for_board(board);
    board.update_pdf_export_settings(core.get_pdf_export_settings());
    board.expand_flags = Board::EXPAND_VIAS | Board::EXPAND_PACKAGES
                         | Board::EXPAND_ALL_AIRWIRES; // expand inner layers of padstacks
}
} // namespace horizon
