#pragma once
#include <gtkmm.h>
#include "pool/package.hpp"
#include <glm/glm.hpp>
#include "util/changeable.hpp"
#include "util/xyz_container.hpp"

namespace horizon {

glm::dmat4 mat_from_model(const Package::Model &model, double scale = 1);

class ModelEditor : public Gtk::Box, public Changeable {
public:
    ModelEditor(class ImpPackage &iimp, const UUID &iuu);
    const UUID uu;

    void set_is_current(const UUID &iuu);
    void set_is_default(const UUID &iuu);
    void make_current();
    void reload();

private:
    ImpPackage &imp;
    Package::Model &model;
    Gtk::CheckButton *default_cb = nullptr;
    Gtk::CheckButton *origin_cb = nullptr;
    Gtk::Label *current_label = nullptr;
    std::vector<Gtk::Widget *> widgets_insenstive_without_model;
    void update_widgets_insenstive();

    XYZContainer<class SpinButtonDim *> sp_shift;
    XYZContainer<class SpinButtonAngle *> sp_angle;
    SpinButtonDim *sp_height_top = nullptr;
    SpinButtonDim *sp_height_bot = nullptr;

    void update_height_from_model();

    std::vector<sigc::connection> sp_connections;
};
} // namespace horizon
