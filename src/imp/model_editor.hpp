#pragma once
#include <gtkmm.h>
#include "pool/package.hpp"
#include <glm/glm.hpp>
#include "util/changeable.hpp"

namespace horizon {

glm::dmat4 mat_from_model(const Package::Model &model, double scale = 1);

class ModelEditor : public Gtk::Box, public Changeable {
public:
    ModelEditor(class ImpPackage &iimp, const UUID &iuu);
    const UUID uu;

    void set_is_current(const UUID &iuu);
    void set_is_default(const UUID &iuu);

private:
    ImpPackage &imp;
    Package::Model &model;
    Gtk::CheckButton *default_cb = nullptr;
    Gtk::CheckButton *origin_cb = nullptr;
    Gtk::Label *current_label = nullptr;

    class SpinButtonDim *sp_x = nullptr;
    class SpinButtonDim *sp_y = nullptr;
    class SpinButtonDim *sp_z = nullptr;

    class SpinButtonAngle *sp_roll = nullptr;
    class SpinButtonAngle *sp_pitch = nullptr;
    class SpinButtonAngle *sp_yaw = nullptr;

    std::vector<sigc::connection> sp_connections;
};
} // namespace horizon
