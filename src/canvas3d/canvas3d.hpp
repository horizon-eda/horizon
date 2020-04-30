#pragma once
#include "canvas_mesh.hpp"
#include "canvas/appearance.hpp"
#include "clipper/clipper.hpp"
#include "common/common.hpp"
#include "util/msd_animator.hpp"
#include <epoxy/gl.h>
#include <glm/glm.hpp>
#include <gtkmm.h>
#include <unordered_map>
#include "canvas3d_base.hpp"
#include <atomic>
#include <thread>

namespace horizon {
class Canvas3D : public Gtk::GLArea, public Canvas3DBase {
public:
    friend class CoverRenderer;
    friend class WallRenderer;
    friend class FaceRenderer;
    friend class BackgroundRenderer;
    Canvas3D();

    float far;
    float near;

    bool smooth_zoom = false;

    void request_push();
    void update(const class Board &brd);
    void update_packages();
    void set_highlights(const std::set<UUID> &pkgs);

    void inc_cam_azimuth(float v);
    void set_appearance(const Appearance &a);

    void set_msaa(unsigned int samples);

    void load_models_async(class Pool *pool);

    void view_all();

    typedef sigc::signal<void, unsigned int, unsigned int> type_signal_models_loading;
    type_signal_models_loading signal_models_loading()
    {
        return s_signal_models_loading;
    }

    int _animate_step(GdkFrameClock *frame_clock);

    ~Canvas3D();

private:
    bool needs_push = false;
    bool needs_view_all = false;
    int a_get_scale_factor() const override;

    void on_size_allocate(Gtk::Allocation &alloc) override;
    void on_realize() override;
    bool on_render(const Glib::RefPtr<Gdk::GLContext> &context) override;
    bool on_button_press_event(GdkEventButton *button_event) override;
    bool on_motion_notify_event(GdkEventMotion *motion_event) override;
    bool on_button_release_event(GdkEventButton *button_event) override;
    bool on_scroll_event(GdkEventScroll *scroll_event) override;

    Glib::RefPtr<Gtk::GestureDrag> gesture_drag;
    glm::vec2 gesture_drag_center_orig;
    glm::vec2 get_center_shift(const glm::vec2 &shift) const;
    void drag_gesture_begin_cb(GdkEventSequence *seq);
    void drag_gesture_update_cb(GdkEventSequence *seq);

    Glib::RefPtr<Gtk::GestureZoom> gesture_zoom;
    float gesture_zoom_cam_dist_orig = 1;
    void zoom_gesture_begin_cb(GdkEventSequence *seq);
    void zoom_gesture_update_cb(GdkEventSequence *seq);

    Glib::RefPtr<Gtk::GestureRotate> gesture_rotate;
    float gesture_rotate_cam_azimuth_orig = 0;
    float gesture_rotate_cam_elevation_orig = 0;
    glm::vec2 gesture_rotate_pos_orig;
    void rotate_gesture_begin_cb(GdkEventSequence *seq);
    void rotate_gesture_update_cb(GdkEventSequence *seq);
    void fix_cam_elevation();

    glm::vec2 pointer_pos_orig;
    float cam_azimuth_orig;
    float cam_elevation_orig;

    glm::vec2 center_orig;

    enum class PanMode { NONE, MOVE, ROTATE };
    PanMode pan_mode = PanMode::NONE;

    MSDAnimator zoom_animator;
    float zoom_animation_cam_dist_orig = 1;

    bool needs_resize = false;


    void prepare();

    void load_models_thread(std::map<std::string, std::string> model_filenames);


    Glib::Dispatcher models_loading_dispatcher;

    type_signal_models_loading s_signal_models_loading;
    unsigned int n_models_loading = 0;
    std::atomic<unsigned int> i_model_loading;

    std::thread model_load_thread;
    std::atomic<bool> stop_model_load_thread;
};
} // namespace horizon
