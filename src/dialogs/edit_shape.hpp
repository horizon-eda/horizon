#pragma once
#include <gtkmm.h>
#include <array>
#include <set>
#include "common/common.hpp"
#include "util/uuid.hpp"
namespace horizon {

class ShapeDialog : public Gtk::Dialog {
    friend class ShapeEditor;

public:
    ShapeDialog(Gtk::Window *parent, std::set<class Shape *> sh);
    bool valid = false;


private:
    std::set<class Shape *> shapes;
    class ShapeEditor *editor = nullptr;
};
} // namespace horizon
