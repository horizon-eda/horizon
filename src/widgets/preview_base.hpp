#pragma once
#include <gtkmm.h>
#include <set>
#include "util/uuid.hpp"
#include "common/common.hpp"
#include "util/pool_goto_provider.hpp"

namespace horizon {
class PreviewBase : public PoolGotoProvider {
protected:
    Gtk::Button *create_goto_button(ObjectType type, std::function<UUID(void)> fn);
    std::set<Gtk::Button *> goto_buttons;
};
} // namespace horizon
