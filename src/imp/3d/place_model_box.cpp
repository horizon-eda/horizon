#include "place_model_box.hpp"
#include "../imp_package.hpp"
#include "3d_view.hpp"
#include "canvas3d/canvas3d.hpp"
#include "util/util.hpp"
#include "util/gtk_util.hpp"

namespace horizon {


class PlaceAtPadDialog : public Gtk::Dialog {
public:
    PlaceAtPadDialog(const Package &pkg);
    UUID selected_pad;

private:
    const Package &pkg;
};

class MyLabel : public Gtk::Label {
public:
    MyLabel(const std::string &txt, const UUID &uu) : Gtk::Label(txt), uuid(uu)
    {
        set_xalign(0);
        property_margin() = 5;
    }

    const UUID uuid;
};

PlaceAtPadDialog::PlaceAtPadDialog(const Package &p)
    : Gtk::Dialog("Place at pad", Gtk::DIALOG_MODAL | Gtk::DIALOG_USE_HEADER_BAR), pkg(p)
{
    set_default_size(200, 400);
    auto sc = Gtk::manage(new Gtk::ScrolledWindow);
    sc->set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC);

    auto lb = Gtk::manage(new Gtk::ListBox);
    lb->set_selection_mode(Gtk::SELECTION_NONE);
    lb->set_activate_on_single_click(true);
    lb->set_header_func(sigc::ptr_fun(header_func_separator));
    sc->add(*lb);

    std::vector<const Pad *> pads;
    for (const auto &it : pkg.pads) {
        pads.push_back(&it.second);
    }
    std::sort(pads.begin(), pads.end(), [](auto a, auto b) { return strcmp_natural(a->name, b->name) < 0; });

    for (const auto it : pads) {
        auto la = Gtk::manage(new MyLabel(it->name, it->uuid));
        lb->append(*la);
    }
    lb->signal_row_activated().connect([this](Gtk::ListBoxRow *row) {
        auto la = dynamic_cast<MyLabel *>(row->get_child());
        selected_pad = la->uuid;
        response(Gtk::RESPONSE_OK);
    });

    sc->show_all();
    get_content_area()->set_border_width(0);
    get_content_area()->pack_start(*sc, true, true, 0);
}

PlaceModelBox::PlaceModelBox(ImpPackage &aimp) : Gtk::Box(Gtk::ORIENTATION_VERTICAL), imp(aimp)
{
    {
        auto box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 10));
        box->property_margin() = 10;

        auto back_button = Gtk::manage(new Gtk::Button);
        back_button->set_image_from_icon_name("go-previous-symbolic", Gtk::ICON_SIZE_BUTTON);
        back_button->signal_clicked().connect([this] {
            start_pick(PickState::IDLE);
            imp.view_3d_stack->set_visible_child("models");
        });
        box->pack_start(*back_button, false, false, 0);

        auto la = Gtk::manage(new Gtk::Label);
        la->set_markup("<b>Align model</b>");
        la->get_style_context()->add_class("dim-label");
        la->set_xalign(0);
        box->pack_start(*la, false, false, 0);

        reset_button = Gtk::manage(new Gtk::Button("Reset"));
        reset_button->signal_clicked().connect(sigc::mem_fun(*this, &PlaceModelBox::reset));
        box->pack_end(*reset_button, false, false, 0);
        box->show_all();
        pack_start(*box, false, false, 0);
    }
    {
        auto sep = Gtk::manage(new Gtk::Separator(Gtk::ORIENTATION_HORIZONTAL));
        pack_start(*sep, false, false, 0);
        sep->show();
    }
    auto sg_from_to = Gtk::SizeGroup::create(Gtk::SIZE_GROUP_HORIZONTAL);
    {
        auto box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 10));
        box->property_margin() = 10;

        {
            auto la = Gtk::manage(new Gtk::Label);
            la->set_label("From");
            sg_from_to->add_widget(*la);
            la->get_style_context()->add_class("dim-label");
            la->set_xalign(0);
            box->pack_start(*la, false, false, 0);
        }

        {
            auto bbox = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL));
            bbox->get_style_context()->add_class("linked");
            pick1_button = Gtk::manage(new Gtk::Button("Pick"));
            pick1_button->signal_clicked().connect([this] { start_pick(PickState::PICK_1); });
            bbox->pack_start(*pick1_button, false, false, 0);
            pick2_button = Gtk::manage(new Gtk::Button("Pick two (center)"));
            pick2_button->signal_clicked().connect([this] { start_pick(PickState::PICK_2_1); });
            bbox->pack_start(*pick2_button, false, false, 0);
            pick_cancel_button = Gtk::manage(new Gtk::Button("Cancel"));
            pick_cancel_button->signal_clicked().connect([this] { start_pick(PickState::IDLE); });
            bbox->pack_start(*pick_cancel_button, false, false, 0);
            box->pack_start(*bbox, false, false, 0);
        }
        pick_state_label = Gtk::manage(new Gtk::Label);
        box->pack_start(*pick_state_label, false, false, 0);

        box->show_all();
        pack_start(*box, false, false, 0);
    }
    static const std::array<std::string, 3> s_xyz = {"X", "Y", "Z"};
    auto sg = Gtk::SizeGroup::create(Gtk::SIZE_GROUP_HORIZONTAL);
    {
        auto placement_grid = Gtk::manage(new Gtk::Grid);
        placement_grid->set_hexpand_set(true);
        placement_grid->set_row_spacing(5);
        placement_grid->set_column_spacing(5);
        placement_grid->set_margin_start(20);

        for (unsigned int ax = 0; ax < 3; ax++) {
            auto sp = Gtk::manage(new SpinButtonDim);
            sp->set_range(-1000_mm, 1000_mm);
            auto la = Gtk::manage(new Gtk::Label(s_xyz.at(ax)));
            la->set_xalign(1);
            la->set_margin_end(4);
            sg->add_widget(*la);
            placement_grid->attach(*la, 0, ax, 1, 1);
            placement_grid->attach(*sp, 1, ax, 1, 1);
            sp_from.set(ax, sp);
        }
        placement_grid->show_all();
        pack_start(*placement_grid, false, false, 0);
    }

    {
        auto box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 10));
        box->property_margin() = 10;

        {
            auto la = Gtk::manage(new Gtk::Label);
            la->set_label("To");
            la->get_style_context()->add_class("dim-label");
            la->set_xalign(0);
            sg_from_to->add_widget(*la);
            box->pack_start(*la, false, false, 0);
        }

        {
            auto bu = Gtk::manage(new Gtk::Button("Pad…"));
            box->pack_start(*bu, false, false, 0);
            bu->signal_clicked().connect([this] {
                PlaceAtPadDialog dia(imp.package);
                dia.set_transient_for(*imp.view_3d_window);
                if (dia.run() == Gtk::RESPONSE_OK) {
                    auto &pad = imp.package.pads.at(dia.selected_pad);
                    sp_to.x->set_value(pad.placement.shift.x);
                    sp_to.y->set_value(pad.placement.shift.y);
                    cb_to.x->set_active(true);
                    cb_to.y->set_active(true);
                    cb_to.z->set_active(false);
                }
            });
        }
        {
            auto bu = Gtk::manage(new Gtk::Button("Origin"));
            box->pack_start(*bu, false, false, 0);
            bu->signal_clicked().connect([this] {
                for (unsigned int ax = 0; ax < 3; ax++) {
                    sp_to.get(ax)->set_value(0);
                }
            });
        }

        box->show_all();
        pack_start(*box, false, false, 0);
    }
    {
        auto placement_grid = Gtk::manage(new Gtk::Grid);
        placement_grid->set_hexpand_set(true);
        placement_grid->set_row_spacing(5);
        placement_grid->set_column_spacing(5);
        placement_grid->set_margin_start(20);
        for (unsigned int ax = 0; ax < 3; ax++) {
            auto sp = Gtk::manage(new SpinButtonDim);
            sp->set_range(-1000_mm, 1000_mm);
            auto cb = Gtk::manage(new Gtk::CheckButton(s_xyz.at(ax)));
            sg->add_widget(*cb);
            placement_grid->attach(*cb, 0, ax, 1, 1);
            placement_grid->attach(*sp, 1, ax, 1, 1);
            sp_to.set(ax, sp);
            cb_to.set(ax, cb);
        }
        placement_grid->show_all();
        pack_start(*placement_grid, false, false, 0);
    }

    {
        move_button = Gtk::manage(new Gtk::Button("Move along selected axis"));
        move_button->get_style_context()->add_class("suggested-action");
        move_button->signal_clicked().connect(sigc::mem_fun(*this, &PlaceModelBox::do_move));
        move_button->show();
        move_button->property_margin() = 20;
        pack_start(*move_button, false, false, 0);
    }

    update_pick_state();
}

void PlaceModelBox::update_pick_state()
{
    const char *s = nullptr;
    switch (pick_state) {
    case PickState::IDLE:
        s = "";
        break;

    case PickState::PICK_1:
        s = "Pick point";
        break;

    case PickState::PICK_2_1:
        s = "Pick first point";
        break;

    case PickState::PICK_2_2:
        s = "Pick second point";
        break;
    }
    pick_state_label->set_text(s);
    const bool is_idle = pick_state == PickState::IDLE;
    pick_cancel_button->set_visible(!is_idle);
    pick1_button->set_visible(is_idle);
    pick2_button->set_visible(is_idle);
    reset_button->set_sensitive(is_idle);
    move_button->set_sensitive(is_idle);
    for (unsigned int ax = 0; ax < 3; ax++) {
        sp_from.get(ax)->set_sensitive(is_idle);
    }
}

void PlaceModelBox::start_pick(PickState which)
{
    switch (which) {
    case PickState::IDLE:
        imp.view_3d_window->get_canvas().set_point_model("");
        imp.view_3d_window->get_canvas().request_push();
        break;

    case PickState::PICK_1:
    case PickState::PICK_2_1:
        imp.update_points();
        break;

    default:;
    }
    pick_state = which;
    update_pick_state();
}

static void set_sp(SpinButtonDim *sp, double v, bool avg)
{
    if (avg)
        sp->set_value((sp->get_value() + v * 1e6) / 2);
    else
        sp->set_value(v * 1e6);
}

void PlaceModelBox::handle_pick(const glm::dvec3 &p)
{
    const bool is_2 = pick_state == PickState::PICK_2_2;
    for (unsigned int ax = 0; ax < 3; ax++) {
        set_sp(sp_from.get(ax), p[ax], is_2);
    }
    switch (pick_state) {
    case PickState::PICK_1:
    case PickState::PICK_2_2:
        start_pick(PickState::IDLE);
        break;

    case PickState::PICK_2_1:
        pick_state = PickState::PICK_2_2;
        break;

    default:;
    }
    update_pick_state();
}

void PlaceModelBox::do_move()
{
    auto &model = imp.core_package.models.at(imp.current_model);
    for (unsigned int ax = 0; ax < 3; ax++) {
        if (cb_to.get(ax)->get_active()) {
            const auto v = model.get_shift(ax);
            auto delta = sp_to.get(ax)->get_value_as_int() - sp_from.get(ax)->get_value_as_int();
            model.set_shift(ax, v + delta);
            sp_from.get(ax)->set_value(sp_from.get(ax)->get_value_as_int() + delta);
        }
    }
    imp.reload_model_editor();
    imp.update_fake_board();
    imp.view_3d_window->get_canvas().update_packages();
    imp.core_package.set_needs_save();
}

void PlaceModelBox::init(const Package::Model &model)
{
    for (unsigned int ax = 0; ax < 3; ax++) {
        shift_init.at(ax) = model.get_shift(ax);
        sp_from.get(ax)->set_value(0);
        sp_to.get(ax)->set_value(0);
        cb_to.get(ax)->set_active(false);
    }
}

void PlaceModelBox::reset()
{
    auto &model = imp.core_package.models.at(imp.current_model);
    for (unsigned int ax = 0; ax < 3; ax++) {
        model.set_shift(ax, shift_init.at(ax));
    }
    imp.reload_model_editor();
    imp.update_fake_board();
    imp.view_3d_window->get_canvas().update_packages();
}
} // namespace horizon
