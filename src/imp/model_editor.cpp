#include "model_editor.hpp"
#include "imp/imp_package.hpp"
#include <glm/gtx/transform.hpp>
#include "widgets/spin_button_angle.hpp"
#include "widgets/spin_button_dim.hpp"
#include "3d_view.hpp"
#include "canvas3d/canvas3d.hpp"
#include "util/util.hpp"
#include "util/gtk_util.hpp"

namespace horizon {

static double angle_to_rad(int16_t a)
{
    return a / 32768. * M_PI;
}

glm::dmat4 mat_from_model(const Package::Model &model, double scale)
{
    glm::dmat4 mat = glm::dmat4(1);
    mat = glm::translate(mat, glm::dvec3(model.x, model.y, model.z) * scale);
    mat = glm::rotate(mat, angle_to_rad(model.roll), glm::dvec3(-1, 0, 0));
    mat = glm::rotate(mat, angle_to_rad(model.pitch), glm::dvec3(0, -1, 0));
    mat = glm::rotate(mat, angle_to_rad(model.yaw), glm::dvec3(0, 0, -1));
    return mat;
}


static Gtk::Label *make_label(const std::string &text)
{
    auto la = Gtk::manage(new Gtk::Label(text));
    la->get_style_context()->add_class("dim-label");
    la->set_halign(Gtk::ALIGN_END);
    return la;
}


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

ModelEditor::ModelEditor(ImpPackage &iimp, const UUID &iuu)
    : Gtk::Box(Gtk::ORIENTATION_VERTICAL, 5), uu(iuu), imp(iimp), model(imp.core_package.models.at(uu))
{
    property_margin() = 10;
    auto entry = Gtk::manage(new Gtk::Entry);
    pack_start(*entry, false, false, 0);
    entry->show();
    entry->set_width_chars(45);
    entry->signal_focus_in_event().connect([this](GdkEventFocus *ev) {
        imp.current_model = uu;
        imp.view_3d_window->update();
        imp.update_model_editors();
        return false;
    });
    bind_widget(entry, model.filename);
    entry->signal_changed().connect([this] { s_signal_changed.emit(); });

    {
        auto box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 5));
        auto delete_button = Gtk::manage(new Gtk::Button);
        delete_button->set_image_from_icon_name("list-remove-symbolic", Gtk::ICON_SIZE_BUTTON);
        delete_button->signal_clicked().connect([this] {
            imp.core_package.models.erase(uu);
            if (imp.core_package.default_model == uu) {
                if (imp.core_package.models.size()) {
                    imp.core_package.default_model = imp.core_package.models.begin()->first;
                }
                else {
                    imp.core_package.default_model = UUID();
                }
            }
            s_signal_changed.emit();
            imp.update_model_editors();
            delete this->get_parent();
        });
        box->pack_end(*delete_button, false, false, 0);

        auto browse_button = Gtk::manage(new Gtk::Button("Browse…"));
        browse_button->signal_clicked().connect([this, entry] {
            Package::Model *model2 = nullptr;
            if (imp.core_package.models.count(uu)) {
                model2 = &imp.core_package.models.at(uu);
            }
            auto mfn = imp.ask_3d_model_filename(model2 ? model2->filename : "");
            if (mfn.size()) {
                entry->set_text(mfn);
                imp.view_3d_window->update(true);
            }
        });
        box->pack_end(*browse_button, false, false, 0);

        default_cb = Gtk::manage(new Gtk::CheckButton("Default"));
        if (imp.core_package.default_model == uu) {
            default_cb->set_active(true);
        }
        default_cb->signal_toggled().connect([this] {
            if (default_cb->get_active()) {
                imp.core_package.default_model = uu;
                imp.current_model = uu;
            }
            imp.view_3d_window->update();
            s_signal_changed.emit();
            imp.update_model_editors();
        });
        box->pack_start(*default_cb, false, false, 0);

        current_label = Gtk::manage(new Gtk::Label("Current"));
        current_label->get_style_context()->add_class("dim-label");
        current_label->set_no_show_all(true);
        box->pack_start(*current_label, false, false, 0);

        box->show_all();
        pack_start(*box, false, false, 0);
    }

    {
        auto box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 5));

        {
            auto place_button = Gtk::manage(new Gtk::MenuButton());
            place_button->set_label("Place…");
            auto place_menu = Gtk::manage(new Gtk::Menu);
            place_button->set_menu(*place_menu);
            {
                auto it = Gtk::manage(new Gtk::MenuItem("Reset"));
                it->show();
                place_menu->append(*it);
                it->signal_activate().connect([this] {
                    origin_cb->set_active(false);
                    sp_x->set_value(0);
                    sp_y->set_value(0);
                    sp_z->set_value(0);
                    sp_roll->set_value(0);
                    sp_pitch->set_value(0);
                    sp_yaw->set_value(0);
                });
            }
            {
                auto it = Gtk::manage(new Gtk::MenuItem("At pad"));
                it->show();
                place_menu->append(*it);
                it->signal_activate().connect([this] {
                    PlaceAtPadDialog dia(imp.package);
                    dia.set_transient_for(*imp.view_3d_window);
                    if (dia.run() == Gtk::RESPONSE_OK) {
                        auto &pad = imp.package.pads.at(dia.selected_pad);
                        sp_x->set_value(pad.placement.shift.x);
                        sp_y->set_value(pad.placement.shift.y);
                    }
                });
            }
            {
                auto it = Gtk::manage(new Gtk::MenuItem("Center"));
                it->show();
                place_menu->append(*it);
                it->signal_activate().connect([this] {
                    const auto &filename = model.filename;
                    auto bb = imp.view_3d_window->get_canvas().get_model_bbox(filename);
                    origin_cb->set_active(false);
                    sp_roll->set_value(0);
                    sp_pitch->set_value(0);
                    sp_yaw->set_value(0);
                    sp_x->set_value((bb.xh + bb.xl) / -2 * 1e6);
                    sp_y->set_value((bb.yh + bb.yl) / -2 * 1e6);
                    sp_z->set_value((bb.zh + bb.zl) / -2 * 1e6);
                    origin_cb->set_active(true);
                });
            }
            box->pack_start(*place_button, false, false, 0);
        }

        origin_cb = Gtk::manage(new Gtk::CheckButton("Rotate around package origin"));
        box->pack_start(*origin_cb, false, false, 0);

        box->show_all();
        pack_start(*box, false, false, 0);
    }

    auto placement_grid = Gtk::manage(new Gtk::Grid);
    placement_grid->set_hexpand_set(true);
    placement_grid->set_row_spacing(5);
    placement_grid->set_column_spacing(5);
    placement_grid->attach(*make_label("X"), 0, 0, 1, 1);
    placement_grid->attach(*make_label("Y"), 0, 1, 1, 1);
    placement_grid->attach(*make_label("Z"), 0, 2, 1, 1);
    std::set<Gtk::SpinButton *> placement_spin_buttons;
    {
        auto sp = Gtk::manage(new SpinButtonDim);
        sp->set_range(-1000_mm, 1000_mm);
        placement_grid->attach(*sp, 1, 0, 1, 1);
        sp->set_value(model.x);
        placement_spin_buttons.insert(sp);
        sp_x = sp;
    }
    {
        auto sp = Gtk::manage(new SpinButtonDim);
        sp->set_range(-1000_mm, 1000_mm);
        placement_grid->attach(*sp, 1, 1, 1, 1);
        sp->set_value(model.y);
        placement_spin_buttons.insert(sp);
        sp_y = sp;
    }
    {
        auto sp = Gtk::manage(new SpinButtonDim);
        sp->set_range(-1000_mm, 1000_mm);
        placement_grid->attach(*sp, 1, 2, 1, 1);
        sp->set_value(model.z);
        placement_spin_buttons.insert(sp);
        sp_z = sp;
    }

    {
        auto la = make_label("Roll");
        la->set_hexpand(true);
        placement_grid->attach(*la, 2, 0, 1, 1);
    }
    placement_grid->attach(*make_label("Pitch"), 2, 1, 1, 1);
    placement_grid->attach(*make_label("Yaw"), 2, 2, 1, 1);
    {
        auto sp = Gtk::manage(new SpinButtonAngle);
        placement_grid->attach(*sp, 3, 0, 1, 1);
        sp->set_value(model.roll);
        placement_spin_buttons.insert(sp);
        sp_roll = sp;
    }
    {
        auto sp = Gtk::manage(new SpinButtonAngle);
        placement_grid->attach(*sp, 3, 1, 1, 1);
        sp->set_value(model.pitch);
        placement_spin_buttons.insert(sp);
        sp_pitch = sp;
    }
    {
        auto sp = Gtk::manage(new SpinButtonAngle);
        placement_grid->attach(*sp, 3, 2, 1, 1);
        sp->set_value(model.yaw);
        placement_spin_buttons.insert(sp);
        sp_yaw = sp;
    }
    for (auto sp : placement_spin_buttons) {
        auto conn = sp->signal_value_changed().connect([this, sp] {
            if (sp == sp_x || sp == sp_y || sp == sp_z) {
                model.x = sp_x->get_value_as_int();
                model.y = sp_y->get_value_as_int();
                model.z = sp_z->get_value_as_int();
            }
            else {
                const auto oldmat = mat_from_model(model);
                const auto p = glm::inverse(oldmat) * glm::dvec4(0, 0, 0, 1);

                model.roll = sp_roll->get_value_as_int();
                model.pitch = sp_pitch->get_value_as_int();
                model.yaw = sp_yaw->get_value_as_int();

                if (origin_cb->get_active()) {
                    const auto newmat = mat_from_model(model);
                    const auto delta = newmat * p;

                    for (auto &cn : sp_connections) {
                        cn.block();
                    }
                    sp_x->set_value(model.x - delta.x);
                    sp_y->set_value(model.y - delta.y);
                    sp_z->set_value(model.z - delta.z);
                    model.x = sp_x->get_value_as_int();
                    model.y = sp_y->get_value_as_int();
                    model.z = sp_z->get_value_as_int();
                    for (auto &cn : sp_connections) {
                        cn.unblock();
                    }
                }
            }

            imp.update_fake_board();
            imp.view_3d_window->get_canvas().update_packages();
            s_signal_changed.emit();
        });
        sp_connections.push_back(conn);
    }

    placement_grid->show_all();
    pack_start(*placement_grid, false, false, 0);

    imp.update_model_editors();
}

void ModelEditor::set_is_current(const UUID &iuu)
{
    current_label->set_visible(iuu == uu);
}


void ModelEditor::set_is_default(const UUID &iuu)
{
    default_cb->set_active(iuu == uu);
}
} // namespace horizon
