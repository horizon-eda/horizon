#pragma once
#include "tool_window.hpp"
#include "util/uuid.hpp"

namespace horizon {

class ToolDataEditPlane : public ToolDataWindow {
public:
    bool pick_net = false;
};

class EditPlaneWindow : public ToolWindow {
public:
    EditPlaneWindow(Gtk::Window *parent, ImpInterface *intf, class Plane &p, class Board &brd);
    void set_net(const UUID &uu);

    class Board &get_board()
    {
        return brd;
    }
    class Plane *get_plane_and_reset_usage();

private:
    Gtk::ToggleButton *pick_button = nullptr;
    class NetButton *net_button = nullptr;

    class Plane &plane;
    const UUID plane_uuid;
    class Board &brd;
    class Polygon &poly;
};
} // namespace horizon
