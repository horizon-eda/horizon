#include "edit_shape.hpp"
#include "common/shape.hpp"
#include <iostream>
#include <deque>
#include <algorithm>
#include "widgets/spin_button_dim.hpp"
#include "util/gtk_util.hpp"
#include "util/geom_util.hpp"

namespace horizon {

class ShapeEditor : public Gtk::Grid {
public:
    ShapeEditor(Shape *sh, ShapeDialog *p);

private:
    Shape *shape;
    ShapeDialog *parent;
    Gtk::ComboBoxText *w_form = nullptr;
    std::vector<Gtk::Widget *> widgets;
    void update(Shape::Form form);
    class SpinButtonDim *add_dim(const std::string &text, int top);
};

ShapeEditor::ShapeEditor(Shape *sh, ShapeDialog *p) : Gtk::Grid(), shape(sh), parent(p)
{
    set_column_spacing(10);
    set_row_spacing(10);
    set_margin_bottom(20);
    set_margin_top(10);
    set_margin_end(20);
    set_margin_start(20);
    int top = 0;
    {
        w_form = Gtk::manage(new Gtk::ComboBoxText());
        w_form->set_hexpand(true);
        std::map<Shape::Form, std::string> items = {
                {Shape::Form::CIRCLE, "Circle"},
                {Shape::Form::OBROUND, "Obround"},
                {Shape::Form::RECTANGLE, "Rectangle"},
        };
        bind_widget<Shape::Form>(w_form, items, shape->form, sigc::mem_fun(*this, &ShapeEditor::update));
        auto la = grid_attach_label_and_widget(this, "Form", w_form, top);
        la->set_size_request(70, -1);
        la->set_xalign(1);
        w_form->show();

        {
            auto button = Gtk::manage(new Gtk::Button());
            button->set_image_from_icon_name("object-select-symbolic", Gtk::ICON_SIZE_BUTTON);
            button->set_tooltip_text("Apply form and parameters to other selected shapes");
            attach(*button, 2, 0, 1, 1);
            button->show();
            button->signal_clicked().connect([this] {
                for (auto s : parent->shapes) {
                    if (s != shape) {
                        s->form = shape->form;
                        s->params = shape->params;
                    }
                }
            });
        }
    }
    update(shape->form);
}

static int64_t get_or_0(const std::vector<int64_t> &v, unsigned int i)
{
    if (i < v.size())
        return v[i];
    else
        return 0;
}

SpinButtonDim *ShapeEditor::add_dim(const std::string &text, int top)
{
    auto la = Gtk::manage(new Gtk::Label(text));
    la->set_halign(Gtk::ALIGN_END);
    la->get_style_context()->add_class("dim-label");
    auto adj = Gtk::manage(new SpinButtonDim());
    attach(*la, 0, top, 1, 1);
    attach(*adj, 1, top, 1, 1);
    widgets.push_back(la);
    widgets.push_back(adj);
    return adj;
}


void ShapeEditor::update(Shape::Form form)
{
    for (auto &it : widgets) {
        delete it;
    }
    widgets.clear();
    int top = 1;
    int n_params = 0;
    if (form == Shape::Form::RECTANGLE || form == Shape::Form::OBROUND) {
        shape->params.resize(2);
        auto w = add_dim("Width", top++);
        w->set_range(0, 1000_mm);
        w->set_value(get_or_0(shape->params, 0));
        w->signal_value_changed().connect([this, w] { shape->params[0] = w->get_value_as_int(); });

        auto w2 = add_dim("Height", top++);
        w2->set_range(0, 1000_mm);
        w2->set_value(get_or_0(shape->params, 1));
        w2->signal_value_changed().connect([this, w2] { shape->params[1] = w2->get_value_as_int(); });

        w->signal_activate().connect([w2] { w2->grab_focus(); });
        w2->signal_activate().connect([this] { parent->response(Gtk::RESPONSE_OK); });
        w->grab_focus();
        n_params = 2;
    }
    else if (form == Shape::Form::CIRCLE) {
        shape->params.resize(1);
        auto w = add_dim("Diameter", top++);
        w->set_range(0, 1000_mm);
        w->set_value(get_or_0(shape->params, 0));
        w->signal_value_changed().connect([this, w] { shape->params[0] = w->get_value_as_int(); });
        w->signal_activate().connect([this] { parent->response(Gtk::RESPONSE_OK); });
        w->grab_focus();
        n_params = 1;
    }
    for (int i = 0; i < n_params; i++) {
        auto button = Gtk::manage(new Gtk::Button());
        button->set_image_from_icon_name("object-select-symbolic", Gtk::ICON_SIZE_BUTTON);
        button->set_tooltip_text("Apply parameter to other selected shapes of the same form");
        attach(*button, 2, i + 1, 1, 1);
        button->show();
        widgets.push_back(button);
        button->signal_clicked().connect([this, i] {
            for (auto s : parent->shapes) {
                if (s != shape) {
                    if (s->form == shape->form)
                        s->params.at(i) = get_or_0(shape->params, i);
                }
            }
        });
    }
    show_all();
}


ShapeDialog::ShapeDialog(Gtk::Window *parent, std::set<Shape *> sh)
    : Gtk::Dialog("Edit shape", *parent, Gtk::DialogFlags::DIALOG_MODAL | Gtk::DialogFlags::DIALOG_USE_HEADER_BAR),
      shapes(sh)
{
    add_button("Cancel", Gtk::ResponseType::RESPONSE_CANCEL);
    add_button("OK", Gtk::ResponseType::RESPONSE_OK);
    set_default_response(Gtk::ResponseType::RESPONSE_OK);

    auto box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL, 0));
    auto combo = Gtk::manage(new Gtk::ComboBoxText());
    combo->set_margin_start(20);
    combo->set_margin_end(20);
    combo->set_margin_top(20);
    box->pack_start(*combo, false, false, 0);

    std::map<UUID, Shape *> shapemap;

    for (auto it : shapes) {
        combo->append(static_cast<std::string>(it->uuid), coord_to_string(it->placement.shift));
        shapemap.emplace(it->uuid, it);
    }

    combo->signal_changed().connect([this, combo, shapemap, box] {
        UUID uu(combo->get_active_id());
        auto shape = shapemap.at(uu);
        if (editor)
            delete editor;
        editor = Gtk::manage(new ShapeEditor(shape, this));
        /*editor->populate();
        editor->signal_apply_all().connect([this, pads, pad](ParameterID id) {
            for (auto it : pads) {
                it->parameter_set[id] = pad->parameter_set.at(id);
            }
        });
        editor->signal_activate_last().connect([this] { response(Gtk::RESPONSE_OK); });*/
        box->pack_start(*editor, true, true, 0);
        editor->show();
    });

    combo->set_active(0);

    get_content_area()->pack_start(*box, true, true, 0);
    get_content_area()->set_border_width(0);

    /*    int top = 0;
        {
            auto la = Gtk::manage(new Gtk::Label("Form"));
            la->set_halign(Gtk::ALIGN_END);
            la->get_style_context()->add_class("dim-label");
            w_form = Gtk::manage(new Gtk::ComboBoxText());
            w_form->append(std::to_string(static_cast<int>(Shape::Form::CIRCLE)), "Circle");
            w_form->append(std::to_string(static_cast<int>(Shape::Form::RECTANGLE)), "Rectangle");
            w_form->append(std::to_string(static_cast<int>(Shape::Form::OBROUND)), "Obround");
            w_form->signal_changed().connect(sigc::mem_fun(*this, &ShapeDialog::update));
            w_form->set_active_id(std::to_string(static_cast<int>(shape->form)));
            w_form->set_hexpand(true);

            grid->attach(*la, 0, top, 1, 1);
            grid->attach(*w_form, 1, top, 1, 1);
            top++;
        }
        get_content_area()->pack_start(*grid, true, true, 0);*/


    show_all();
}
/*





void ShapeDialog::ok_clicked()
{
    shape->form = static_cast<Shape::Form>(std::stoi(w_form->get_active_id()));
    shape->params.clear();
    for (auto &it : widgets) {
        auto sp = dynamic_cast<SpinButtonDim *>(it.second);
        shape->params.push_back(sp->get_value_as_int());
    }
}
*/
} // namespace horizon
