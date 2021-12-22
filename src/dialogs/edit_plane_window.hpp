#pragma once
#include "tool_window.hpp"
#include "util/uuid.hpp"

namespace horizon {
class EditPlaneWindow : public ToolWindow {
public:
    EditPlaneWindow(Gtk::Window *parent, ImpInterface *intf, class Plane &p, class Board &brd);

private:
    void on_event(ToolDataWindow::Event ev) override;

    class Plane &plane;
    const UUID plane_uuid;
    class Board &brd;
    class Polygon &poly;
};
} // namespace horizon
