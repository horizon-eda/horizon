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
#include "spacenav_prefs.hpp"
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
    bool touchpad_pan = false;
    SpacenavPrefs spacenav_prefs;

    void request_push();
    void update(const class Board &brd);
    void update_packages();
    void set_highlights(const std::set<UUID> &pkgs);

    void animate_to_azimuth_elevation_abs(float az, float el);
    void animate_to_azimuth_elevation_rel(float az, float el);
    void animate_zoom_step(int inc);
    void animate_center_rel(const glm::vec2 &d);
    void set_appearance(const Appearance &a);

    void set_msaa(unsigned int samples);

    void load_models_async(class IPool &pool);

    void view_all();

    void set_msd_params(const MSD::Params &params);

    glm::vec2 get_center_shift(const glm::vec2 &shift) const;

    typedef sigc::signal<void, unsigned int, unsigned int> type_signal_models_loading;
    type_signal_models_loading signal_models_loading()
    {
        return s_signal_models_loading;
    }
    type_signal_models_loading signal_layers_loading()
    {
        return s_signal_layers_loading;
    }

    typedef sigc::signal<void, UUID> type_signal_package_select;
    type_signal_package_select signal_package_select()
    {
        return s_signal_package_select;
    }

    typedef sigc::signal<void, glm::dvec3> type_signal_point_select;
    type_signal_point_select signal_point_select()
    {
        return s_signal_point_select;
    }

    typedef sigc::signal<void, unsigned int> type_signal_spacenav_button_press;
    type_signal_spacenav_button_press signal_spacenav_button_press()
    {
        return s_signal_spacenav_button_press;
    }

    bool get_spacenav_available() const
    {
        return have_spnav;
    }

    ~Canvas3D();

private:
    bool needs_push = false;
    bool needs_view_all = false;
    int a_get_scale_factor() const override;
    void redraw() override;

    void on_size_allocate(Gtk::Allocation &alloc) override;
    void on_realize() override;
    bool on_render(const Glib::RefPtr<Gdk::GLContext> &context) override;
    bool on_button_press_event(GdkEventButton *button_event) override;
    bool on_motion_notify_event(GdkEventMotion *motion_event) override;
    bool on_button_release_event(GdkEventButton *button_event) override;
    bool on_scroll_event(GdkEventScroll *scroll_event) override;

    void pan_zoom(GdkEventScroll *scroll_event);
    void pan_drag_move(GdkEventScroll *scroll_event);
    void pan_rotate(GdkEventScroll *scroll_event);

    Glib::RefPtr<Gtk::GestureDrag> gesture_drag;
    glm::vec2 gesture_drag_center_orig;
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

    glm::vec2 pointer_pos_orig;
    float cam_azimuth_orig;
    float cam_elevation_orig;

    glm::vec2 center_orig;

    enum class PanMode { NONE, MOVE, ROTATE };
    PanMode pan_mode = PanMode::NONE;


    bool needs_resize = false;

    MSDAnimator azimuth_animator;
    MSDAnimator elevation_animator;
    MSDAnimator zoom_animator;
    MSDAnimator cx_animator;
    MSDAnimator cy_animator;

    std::vector<MSDAnimator *> animators;
    int animate_step(GdkFrameClock *frame_clock);
    static int anim_tick_cb(GtkWidget *cwidget, GdkFrameClock *frame_clock, gpointer user_data);
    void start_anim();

    void prepare();

    void load_models_thread(std::map<std::string, std::string> model_filenames);


    Glib::Dispatcher models_loading_dispatcher;

    type_signal_models_loading s_signal_models_loading;
    type_signal_models_loading s_signal_layers_loading;
    unsigned int n_models_loading = 0;
    std::atomic<unsigned int> i_model_loading;

    std::thread model_load_thread;
    std::atomic<bool> stop_model_load_thread;

    type_signal_package_select s_signal_package_select;
    type_signal_point_select s_signal_point_select;
    int pick_x, pick_y;

    std::map<int, CanvasMesh::Layer3D> layers_local;
    std::set<int> layers_to_be_moved;
    const std::map<int, CanvasMesh::Layer3D> &get_layers() const override;

    Glib::Dispatcher prepare_dispatcher;
    std::thread prepare_thread;
    void prepare_thread_worker();

    bool have_spnav = false;
    void handle_spnav();
    sigc::connection spnav_connection;

    type_signal_spacenav_button_press s_signal_spacenav_button_press;
};
} // namespace horizon
