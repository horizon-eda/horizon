#pragma once
#include <gtkmm.h>
#include "pool/package.hpp"
#include "util/xyz_container.hpp"
#include <glm/glm.hpp>

namespace horizon {


class PlaceModelBox : public Gtk::Box {
public:
    PlaceModelBox(class ImpPackage &aimp);
    void handle_pick(const glm::dvec3 &p);
    void init(const Package::Model &model);

private:
    ImpPackage &imp;
    Gtk::Button *pick1_button;
    Gtk::Button *pick2_button;
    Gtk::Button *pick_cancel_button;
    Gtk::Label *pick_state_label;
    Gtk::Button *reset_button;
    Gtk::Button *move_button;

    std::array<int64_t, 3> shift_init;

    XYZContainer<class SpinButtonDim *> sp_from;

    XYZContainer<Gtk::CheckButton *> cb_to;
    XYZContainer<class SpinButtonDim *> sp_to;

    enum class PickState { IDLE, PICK_1, PICK_2_1, PICK_2_2 };
    PickState pick_state = PickState::IDLE;
    void update_pick_state();
    void start_pick(PickState which);
    void do_move();
    void reset();
};

} // namespace horizon
