#include "model_editor.hpp"
#include "imp/imp_package.hpp"
#include <glm/gtx/transform.hpp>
#include "widgets/spin_button_angle.hpp"
#include "widgets/spin_button_dim.hpp"
#include "3d_view.hpp"
#include "canvas3d/canvas3d.hpp"
#include "place_model_box.hpp"
#include "util/gtk_util.hpp"
#include "util/geom_util.hpp"

namespace horizon {

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

void ModelEditor::make_current()
{
    if (imp.current_model != uu) {
        imp.current_model = uu;
        imp.view_3d_window->set_needs_update();
        imp.view_3d_window->update();
    }
    imp.update_model_editors();
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
        imp.view_3d_window->set_needs_update();
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
                    imp.current_model = imp.core_package.default_model;
                }
                else {
                    imp.core_package.default_model = UUID();
                }
            }
            s_signal_changed.emit();
            imp.view_3d_window->set_needs_update();
            imp.view_3d_window->update();
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
                imp.view_3d_window->set_needs_update();
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
                make_current();
            }
            else {
                imp.update_model_editors();
            }
            s_signal_changed.emit();
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
            place_button->signal_clicked().connect(sigc::mem_fun(*this, &ModelEditor::make_current));
            auto place_menu = Gtk::manage(new Gtk::Menu);
            place_button->set_menu(*place_menu);

            {
                auto it = Gtk::manage(new Gtk::MenuItem("Center"));
                it->show();
                place_menu->append(*it);
                it->signal_activate().connect([this] {
                    const auto &filename = model.filename;
                    auto bb = imp.view_3d_window->get_canvas().get_model_bbox(filename);
                    origin_cb->set_active(false);
                    for (unsigned int ax = 0; ax < 3; ax++) {
                        sp_angle.get(ax)->set_value(0);
                    }
                    sp_shift.x->set_value((bb.xh + bb.xl) / -2 * 1e6);
                    sp_shift.y->set_value((bb.yh + bb.yl) / -2 * 1e6);
                    sp_shift.z->set_value((bb.zh + bb.zl) / -2 * 1e6);
                    origin_cb->set_active(true);
                });
                widgets_insenstive_without_model.push_back(it);
            }

            {
                auto it = Gtk::manage(new Gtk::MenuItem("Align…"));
                it->show();
                place_menu->append(*it);
                it->signal_activate().connect([this] {
                    imp.view_3d_stack->set_visible_child("place");
                    imp.place_model_box->init(model);
                });
                widgets_insenstive_without_model.push_back(it);
            }

            {
                auto it = Gtk::manage(new Gtk::MenuItem("Round to 1 µm"));
                it->show();
                place_menu->append(*it);
                it->signal_activate().connect([this] {
                    for (unsigned int ax = 0; ax < 3; ax++) {
                        sp_shift.get(ax)->set_value(round_multiple(sp_shift.get(ax)->get_value_as_int(), 1000));
                    }
                });
            }

            {
                auto it = Gtk::manage(new Gtk::MenuItem("Reset"));
                it->show();
                place_menu->append(*it);
                it->signal_activate().connect([this] {
                    origin_cb->set_active(false);
                    for (unsigned int ax = 0; ax < 3; ax++) {
                        sp_shift.get(ax)->set_value(0);
                        sp_angle.get(ax)->set_value(0);
                    }
                });
            }

            box->pack_start(*place_button, false, false, 0);
        }
        {
            auto project_button = Gtk::manage(new Gtk::Button("Project"));
            project_button->signal_clicked().connect([this] { imp.project_model(model); });
            box->pack_start(*project_button, false, false, 0);
            widgets_insenstive_without_model.push_back(project_button);
        }

        origin_cb = Gtk::manage(new Gtk::CheckButton("Rotate around package origin"));
        box->pack_end(*origin_cb, false, false, 0);

        box->show_all();
        pack_start(*box, false, false, 0);
    }

    update_widgets_insenstive();
    imp.view_3d_window->get_canvas().signal_models_loading().connect(
            sigc::track_obj([this](auto a, auto b) { update_widgets_insenstive(); }, *this));

    auto placement_grid = Gtk::manage(new Gtk::Grid);
    placement_grid->set_hexpand_set(true);
    placement_grid->set_row_spacing(5);
    placement_grid->set_column_spacing(5);
    std::set<Gtk::SpinButton *> placement_spin_buttons;
    static const std::array<std::string, 3> s_xyz = {"X", "Y", "Z"};
    for (unsigned int ax = 0; ax < 3; ax++) {
        placement_grid->attach(*make_label(s_xyz.at(ax)), 0, ax, 1, 1);
        auto sp = Gtk::manage(new SpinButtonDim);
        sp->set_range(-1000_mm, 1000_mm);
        placement_grid->attach(*sp, 1, ax, 1, 1);
        sp->set_value(model.get_shift(ax));
        placement_spin_buttons.insert(sp);
        sp_shift.set(ax, sp);
    }

    static const std::array<std::string, 3> s_rpy = {"Roll", "Pitch", "Yaw"};

    for (unsigned int ax = 0; ax < 3; ax++) {
        auto la = make_label(s_rpy.at(ax));
        la->set_hexpand(true);
        placement_grid->attach(*la, 2, ax, 1, 1);
        auto sp = Gtk::manage(new SpinButtonAngle);
        placement_grid->attach(*sp, 3, ax, 1, 1);
        sp->set_value(model.get_rotation(ax));
        placement_spin_buttons.insert(sp);
        sp_angle.set(ax, sp);
    }

    for (auto sp : placement_spin_buttons) {
        auto conn = sp->signal_value_changed().connect([this, sp] {
            if (sp == sp_shift.x || sp == sp_shift.y || sp == sp_shift.z) {
                for (unsigned int ax = 0; ax < 3; ax++) {
                    model.set_shift(ax, sp_shift.get(ax)->get_value_as_int());
                }
            }
            else {
                const auto oldmat = mat_from_model(model);
                const auto p = glm::inverse(oldmat) * glm::dvec4(0, 0, 0, 1);

                for (unsigned int ax = 0; ax < 3; ax++) {
                    model.set_rotation(ax, sp_angle.get(ax)->get_value_as_int());
                }

                if (origin_cb->get_active()) {
                    const auto newmat = mat_from_model(model);
                    const auto delta = newmat * p;

                    for (auto &cn : sp_connections) {
                        cn.block();
                    }
                    for (unsigned int ax = 0; ax < 3; ax++) {
                        sp_shift.get(ax)->set_value(model.get_shift(ax) - delta[ax]);
                        model.set_shift(ax, sp_shift.get(ax)->get_value_as_int());
                    }
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

void ModelEditor::update_widgets_insenstive()
{
    const auto has_model = imp.view_3d_window->get_canvas().model_is_loaded(model.filename);
    for (auto w : widgets_insenstive_without_model) {
        w->set_sensitive(has_model);
    }
}

void ModelEditor::set_is_current(const UUID &iuu)
{
    current_label->set_visible(iuu == uu);
}


void ModelEditor::set_is_default(const UUID &iuu)
{
    default_cb->set_active(iuu == uu);
}

void ModelEditor::reload()
{
    for (auto &cn : sp_connections) {
        cn.block();
    }
    for (unsigned int ax = 0; ax < 3; ax++) {
        sp_shift.get(ax)->set_value(model.get_shift(ax));
    }
    for (auto &cn : sp_connections) {
        cn.unblock();
    }
}

} // namespace horizon
