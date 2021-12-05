#include "footprint_generator_window.hpp"
#include "footprint_generator_base.hpp"
#include "footprint_generator_dual.hpp"
#include "footprint_generator_grid.hpp"
#include "footprint_generator_quad.hpp"
#include "footprint_generator_single.hpp"
#include "footprint_generator_footag.hpp"
#include "widgets/spin_button_dim.hpp"
#include "core/core_package.hpp"
#include "util/gtk_util.hpp"
#include <pangomm/layout.h>

namespace horizon {

FootprintGeneratorWindow::FootprintGeneratorWindow(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x,
                                                   CorePackage &c)
    : Gtk::Window(cobject), core(c)
{
    GET_WIDGET(stack);
    GET_WIDGET(generate_button);

    {
        auto gen = Gtk::manage(new FootprintGeneratorFootag(c));
        gen->show();
        stack->add(*gen, "footag", "IPC-7351B");
    }
    {
        auto gen = Gtk::manage(new FootprintGeneratorDual(c));
        gen->show();
        gen->property_can_generate().signal_changed().connect(
                sigc::mem_fun(*this, &FootprintGeneratorWindow::update_can_generate));
        stack->add(*gen, "dual", "Dual");
    }
    {
        auto gen = Gtk::manage(new FootprintGeneratorSingle(c));
        gen->show();
        gen->property_can_generate().signal_changed().connect(
                sigc::mem_fun(*this, &FootprintGeneratorWindow::update_can_generate));
        stack->add(*gen, "single", "Single");
    }
    {
        auto gen = Gtk::manage(new FootprintGeneratorQuad(c));
        gen->show();
        gen->property_can_generate().signal_changed().connect(
                sigc::mem_fun(*this, &FootprintGeneratorWindow::update_can_generate));
        stack->add(*gen, "quad", "Quad");
    }
    {
        auto gen = Gtk::manage(new FootprintGeneratorGrid(c));
        gen->show();
        gen->property_can_generate().signal_changed().connect(
                sigc::mem_fun(*this, &FootprintGeneratorWindow::update_can_generate));
        stack->add(*gen, "grid", "Grid");
    }
    stack->property_visible_child().signal_changed().connect(
            sigc::mem_fun(*this, &FootprintGeneratorWindow::update_can_generate));
    update_can_generate();
    generate_button->signal_clicked().connect([this] {
        {
            auto gen = dynamic_cast<FootprintGeneratorBase *>(stack->get_visible_child());
            if (gen) {
                if (gen->generate()) {
                    core.set_needs_save();
                    core.rebuild("generate footprint");
                    signal_generated().emit();
                    hide();
                }
            }
        }
        {
            auto gen = dynamic_cast<FootprintGeneratorFootag *>(stack->get_visible_child());
            if (gen) {
                if (gen->generate()) {
                    core.set_needs_save();
                    core.rebuild("generate footprint");
                    signal_generated().emit();
                    hide();
                }
            }
        }
    });
}

void FootprintGeneratorWindow::update_can_generate()
{
    bool sens = false;
    {
        auto w = dynamic_cast<FootprintGeneratorBase *>(stack->get_visible_child());
        if (w)
            sens = w->property_can_generate();
    }
    {
        auto w = dynamic_cast<FootprintGeneratorFootag *>(stack->get_visible_child());
        if (w)
            sens = true;
    }
    generate_button->set_sensitive(sens);
}

FootprintGeneratorWindow *FootprintGeneratorWindow::create(Gtk::Window *p, CorePackage &c)
{
    FootprintGeneratorWindow *w;
    Glib::RefPtr<Gtk::Builder> x = Gtk::Builder::create();
    x->add_from_resource(
            "/org/horizon-eda/horizon/imp/footprint_generator/"
            "footprint_generator.ui");
    x->get_widget_derived("window", w, c);
    w->set_transient_for(*p);
    return w;
}
} // namespace horizon
