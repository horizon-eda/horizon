#pragma once
#include "core/tool.hpp"

namespace horizon {
class ToolHelperEditPlane : public virtual ToolBase {

protected:
    void show_edit_plane_window(class Plane &plane, class Board &brd);
    ToolResponse update_for_plane(const ToolArgs &args);

private:
    bool pick_net_mode = false;
    class EditPlaneWindow *win = nullptr;
};
} // namespace horizon
