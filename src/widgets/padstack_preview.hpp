#pragma once
#include <gtkmm.h>
#include <set>
#include "common/common.hpp"
#include "util/uuid.hpp"
#include "util/pool_goto_provider.hpp"

namespace horizon {
class PadstackPreview : public Gtk::Box, public PoolGotoProvider {
public:
    PadstackPreview(class Pool &pool);

    void load(const UUID &uu);

private:
    class Pool &pool;
    class PreviewCanvas *canvas_padstack = nullptr;

    Gtk::Label *package_label = nullptr;
    Gtk::Box *top_box = nullptr;
    Gtk::Separator *sep = nullptr;
};
} // namespace horizon
