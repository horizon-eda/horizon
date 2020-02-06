#include "footprint_generator_footag.hpp"
#include "footag/display.hpp"

namespace horizon {
FootprintGeneratorFootag::FootprintGeneratorFootag(IDocumentPackage *c)
    : Glib::ObjectBase(typeid(FootprintGeneratorFootag)), sidebar(), separator(Gtk::Orientation::ORIENTATION_VERTICAL),
      stack()
{
    add(sidebar);
    add(separator);
    add(stack);

    stack.set_transition_type(Gtk::StackTransitionType::STACK_TRANSITION_TYPE_CROSSFADE);
    sidebar.set_stack(stack);

    for (int type = 0; type < FOOTAG_TYPE_NUM; type++) {
        auto ti = footag_get_typeinfo((enum footag_type)type);
        if (!ti) {
            continue;
        }
        auto ed = FootagDisplay::create(c, (enum footag_type)type);
        if (ed->isopen()) {
            stack.add(*ed, ti->name, ti->name);
            ed->show();
        }
        ed->unreference();
    }
}

bool FootprintGeneratorFootag::generate(void)
{
    auto gen = dynamic_cast<FootagDisplay *>(stack.get_visible_child());
    if (gen) {
        return gen->generate();
    }
    return false;
}
} // namespace horizon
