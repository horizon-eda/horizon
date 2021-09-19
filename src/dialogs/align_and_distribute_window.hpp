#pragma once
#include <gtkmm.h>
#include <vector>
#include "util/uuid.hpp"
#include "tool_window.hpp"
#include "core/tools/tool_align_and_distribute.hpp"

namespace horizon {
class ToolDataAlignAndDistribute : public ToolDataWindow {
public:
    using Operation = ToolAlignAndDistribute::Operation;
    Operation operation = Operation::RESET;
    bool preview = false;
};

class AlignAndDistributeWindow : public ToolWindow {
public:
    AlignAndDistributeWindow(Gtk::Window *parent, ImpInterface *intf);

    void update();

private:
    using Operation = ToolAlignAndDistribute::Operation;
    Gtk::Button *make_button(Operation op);
    Gtk::Box *make_box(const std::string &title, const std::vector<Operation> &ops);
    void emit(Operation op, bool preview);
};
} // namespace horizon
