#include "edit_stackup.hpp"
#include <iostream>
#include <deque>
#include <algorithm>
#include "board/board.hpp"
#include "board/board_layers.hpp"
#include "widgets/spin_button_dim.hpp"
#include "util/util.hpp"
#include "document/idocument_board.hpp"

namespace horizon {

class StackupLayerEditor : public Gtk::Box {
public:
    StackupLayerEditor(EditStackupDialog &parent, int la, bool cu);
    SpinButtonDim *sp = nullptr;
    int layer;
    bool copper;
};

StackupLayerEditor::StackupLayerEditor(EditStackupDialog &parent, int ly, bool cu)
    : Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 10), layer(ly), copper(cu)
{
    auto colorbox = Gtk::manage(new Gtk::DrawingArea);
    colorbox->set_size_request(20, -1);
    colorbox->show();
    colorbox->signal_draw().connect([this](const Cairo::RefPtr<Cairo::Context> &cr) -> bool {
        if (copper) {
            cr->set_source_rgb(1, .8, 0);
        }
        else {
            cr->set_source_rgb(.2, .15, 0);
        }
        cr->paint();
        return true;
    });
    pack_start(*colorbox, false, false, 0);

    auto label_str = BoardLayers::get_layer_name(layer);
    if (cu) {
        label_str += " (Copper)";
    }
    else {
        label_str += " (Substrate)";
    }

    auto la = Gtk::manage(new Gtk::Label(label_str));
    parent.sg_layer_name->add_widget(*la);
    la->set_xalign(0);
    la->show();
    pack_start(*la, false, false, 0);

    sp = Gtk::manage(new SpinButtonDim());
    sp->set_range(0, 10_mm);
    sp->show();

    set_margin_start(8);
    set_margin_end(8);
    set_margin_top(4);
    set_margin_bottom(4);

    pack_start(*sp, true, true, 0);
}


EditStackupDialog::EditStackupDialog(Gtk::Window *parent, IDocumentBoard &c)
    : Gtk::Dialog("Edit Stackup", *parent, Gtk::DialogFlags::DIALOG_MODAL | Gtk::DialogFlags::DIALOG_USE_HEADER_BAR),
      core(c), board(*core.get_board())
{
    add_button("Cancel", Gtk::ResponseType::RESPONSE_CANCEL);
    auto ok_button = add_button("OK", Gtk::ResponseType::RESPONSE_OK);
    ok_button->signal_clicked().connect(sigc::mem_fun(*this, &EditStackupDialog::ok_clicked));
    set_default_response(Gtk::ResponseType::RESPONSE_OK);
    set_default_size(400, 300);

    sg_layer_name = Gtk::SizeGroup::create(Gtk::SIZE_GROUP_HORIZONTAL);

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
    lb->set_selection_mode(Gtk::SELECTION_NONE);
    sc->add(*lb);

    box->pack_start(*sc, true, true, 0);

    get_content_area()->pack_start(*box, true, true, 0);
    get_content_area()->set_border_width(0);


    show_all();

    create_editors();
    update_layers();
}

void EditStackupDialog::create_editor(int layer, bool cu)
{
    auto ed = Gtk::manage(new StackupLayerEditor(*this, layer, cu));
    if (board.stackup.count(layer))
        ed->sp->set_value(cu ? board.stackup.at(layer).thickness : board.stackup.at(layer).substrate_thickness);
    else if (cu)
        ed->sp->set_value(.035_mm);
    else
        ed->sp->set_value(.1_mm);
    lb->add(*ed);
    ed->show();
}

void EditStackupDialog::create_editors()
{
    create_editor(BoardLayers::TOP_COPPER, true);
    create_editor(BoardLayers::TOP_COPPER, false);

    for (unsigned int i = 0; i < BoardLayers::max_inner_layers; i++) {
        const int layer = -i - 1;
        for (const auto cu : {true, false}) {
            create_editor(layer, cu);
        }
    }

    create_editor(BoardLayers::BOTTOM_COPPER, true);
}

void EditStackupDialog::update_layers()
{
    auto n_inner_layers = sp_n_inner_layers->get_value_as_int();
    board.set_n_inner_layers(n_inner_layers);
    for (auto ch : lb->get_children()) {
        auto &ed = dynamic_cast<StackupLayerEditor &>(*dynamic_cast<Gtk::ListBoxRow &>(*ch).get_child());
        if (ed.layer < BoardLayers::TOP_COPPER && ed.layer > BoardLayers::BOTTOM_COPPER) {
            const int inner = -ed.layer - 1;
            ch->set_visible(inner < n_inner_layers);
        }
    }
}

void EditStackupDialog::ok_clicked()
{
    auto n_inner_layers = sp_n_inner_layers->get_value_as_int();
    board.set_n_inner_layers(n_inner_layers);
    for (auto ch : lb->get_children()) {
        if (ch->get_visible()) {
            auto &ed = dynamic_cast<StackupLayerEditor &>(*dynamic_cast<Gtk::ListBoxRow &>(*ch).get_child());
            if (ed.copper) {
                board.stackup.at(ed.layer).thickness = ed.sp->get_value_as_int();
            }
            else {
                board.stackup.at(ed.layer).substrate_thickness = ed.sp->get_value_as_int();
            }
        }
    }
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
