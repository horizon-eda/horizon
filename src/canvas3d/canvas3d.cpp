#include "canvas3d.hpp"
#include "board/board.hpp"
#include "board/board_layers.hpp"
#include "canvas/gl_util.hpp"
#include "common/hole.hpp"
#include "common/polygon.hpp"
#include "logger/logger.hpp"
#include "poly2tri/poly2tri.h"
#include "util/util.hpp"
#include "pool/pool_manager.hpp"
#include "util/min_max_accumulator.hpp"
#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <iostream>
#include <thread>

namespace horizon {

Canvas3D::Canvas3D() : i_model_loading(0), stop_model_load_thread(false)
{
    add_events(Gdk::BUTTON_PRESS_MASK | Gdk::BUTTON_RELEASE_MASK | Gdk::BUTTON_MOTION_MASK | Gdk::SCROLL_MASK
               | Gdk::SMOOTH_SCROLL_MASK);

    models_loading_dispatcher.connect([this] {
        update_max_package_height();
        request_push();
        s_signal_models_loading.emit(i_model_loading, n_models_loading);
        if ((i_model_loading >= n_models_loading) && model_load_thread.joinable()) {
            model_load_thread.join();
        }
    });

    gesture_drag = Gtk::GestureDrag::create(*this);
    gesture_drag->signal_begin().connect(sigc::mem_fun(*this, &Canvas3D::drag_gesture_begin_cb));
    gesture_drag->signal_update().connect(sigc::mem_fun(*this, &Canvas3D::drag_gesture_update_cb));
    gesture_drag->set_propagation_phase(Gtk::PHASE_CAPTURE);
    gesture_drag->set_touch_only(true);

    gesture_zoom = Gtk::GestureZoom::create(*this);
    gesture_zoom->signal_begin().connect(sigc::mem_fun(*this, &Canvas3D::zoom_gesture_begin_cb));
    gesture_zoom->signal_update().connect(sigc::mem_fun(*this, &Canvas3D::zoom_gesture_update_cb));
    gesture_zoom->set_propagation_phase(Gtk::PHASE_CAPTURE);

    gesture_rotate = Gtk::GestureRotate::create(*this);
    gesture_rotate->signal_begin().connect(sigc::mem_fun(*this, &Canvas3D::rotate_gesture_begin_cb));
    gesture_rotate->signal_update().connect(sigc::mem_fun(*this, &Canvas3D::rotate_gesture_update_cb));
    gesture_rotate->set_propagation_phase(Gtk::PHASE_CAPTURE);
}

glm::vec2 Canvas3D::get_center_shift(const glm::vec2 &shift) const
{
    return glm::rotate(glm::mat2(1, 0, 0, sin(glm::radians(cam_elevation))) * shift * 0.1218f * cam_distance / 105.f,
                       glm::radians(cam_azimuth - 90));
}


void Canvas3D::set_appearance(const Appearance &a)
{
    appearance = a;
    queue_draw();
}

void Canvas3D::on_size_allocate(Gtk::Allocation &alloc)
{
    width = alloc.get_width();
    height = alloc.get_height();
    needs_resize = true;
    if (needs_view_all) {
        view_all();
        needs_view_all = false;
    }

    // chain up
    Gtk::GLArea::on_size_allocate(alloc);
}

bool Canvas3D::on_button_press_event(GdkEventButton *button_event)
{
    if (button_event->button == 2 || (button_event->button == 1 && (button_event->state & Gdk::SHIFT_MASK))) {
        pan_mode = PanMode::MOVE;
        pointer_pos_orig = {button_event->x, button_event->y};
        center_orig = center;
    }
    else if (button_event->button == 1) {
        pan_mode = PanMode::ROTATE;
        pointer_pos_orig = {button_event->x, button_event->y};
        cam_elevation_orig = cam_elevation;
        cam_azimuth_orig = cam_azimuth;
    }
    return Gtk::GLArea::on_button_press_event(button_event);
}

void Canvas3D::fix_cam_elevation()
{
    while (cam_elevation >= 360)
        cam_elevation -= 360;
    while (cam_elevation < 0)
        cam_elevation += 360;
    if (cam_elevation > 180)
        cam_elevation -= 360;
}

bool Canvas3D::on_motion_notify_event(GdkEventMotion *motion_event)
{
    auto delta = glm::mat2(1, 0, 0, -1) * (glm::vec2(motion_event->x, motion_event->y) - pointer_pos_orig);
    if (pan_mode == PanMode::ROTATE) {
        cam_azimuth = cam_azimuth_orig - (delta.x / width) * 360;
        cam_elevation = cam_elevation_orig - (delta.y / height) * 90;
        fix_cam_elevation();
        queue_draw();
    }
    else if (pan_mode == PanMode::MOVE) {
        center = center_orig + get_center_shift(delta);
        queue_draw();
    }
    return Gtk::GLArea::on_motion_notify_event(motion_event);
}

bool Canvas3D::on_button_release_event(GdkEventButton *button_event)
{
    pan_mode = PanMode::NONE;
    return Gtk::GLArea::on_button_release_event(button_event);
}

void Canvas3D::drag_gesture_begin_cb(GdkEventSequence *seq)
{
    if (pan_mode != PanMode::NONE) {
        gesture_drag->set_state(Gtk::EVENT_SEQUENCE_DENIED);
    }
    else {
        gesture_drag_center_orig = center;
        gesture_drag->set_state(Gtk::EVENT_SEQUENCE_CLAIMED);
    }
}
void Canvas3D::drag_gesture_update_cb(GdkEventSequence *seq)
{
    double x, y;
    if (gesture_drag->get_offset(x, y)) {
        center = gesture_drag_center_orig + get_center_shift({x, -y});
        queue_draw();
    }
}

void Canvas3D::zoom_gesture_begin_cb(GdkEventSequence *seq)
{
    if (pan_mode != PanMode::NONE) {
        gesture_zoom->set_state(Gtk::EVENT_SEQUENCE_DENIED);
        return;
    }
    gesture_zoom_cam_dist_orig = cam_distance;
    gesture_zoom->set_state(Gtk::EVENT_SEQUENCE_CLAIMED);
}

void Canvas3D::zoom_gesture_update_cb(GdkEventSequence *seq)
{
    auto delta = gesture_zoom->get_scale_delta();
    cam_distance = gesture_zoom_cam_dist_orig / delta;
    queue_draw();
}

void Canvas3D::rotate_gesture_begin_cb(GdkEventSequence *seq)
{
    if (pan_mode != PanMode::NONE) {
        gesture_zoom->set_state(Gtk::EVENT_SEQUENCE_DENIED);
        return;
    }
    gesture_rotate_cam_azimuth_orig = cam_azimuth;
    gesture_rotate_cam_elevation_orig = cam_elevation;
    double cx, cy;
    gesture_rotate->get_bounding_box_center(cx, cy);
    gesture_rotate_pos_orig = glm::vec2(cx, cy);
    gesture_zoom->set_state(Gtk::EVENT_SEQUENCE_CLAIMED);
}

void Canvas3D::rotate_gesture_update_cb(GdkEventSequence *seq)
{
    auto delta = gesture_rotate->get_angle_delta();
    if (cam_elevation < 0)
        delta *= -1;
    cam_azimuth = gesture_rotate_cam_azimuth_orig + glm::degrees(delta);
    inc_cam_azimuth(0);
    double cx, cy;
    gesture_rotate->get_bounding_box_center(cx, cy);
    auto dy = cy - gesture_rotate_pos_orig.y;
    cam_elevation = gesture_rotate_cam_elevation_orig + (dy / height) * 180;
    fix_cam_elevation();
    queue_draw();
}

int Canvas3D::_animate_step(GdkFrameClock *frame_clock)
{
    auto r = zoom_animator.step(gdk_frame_clock_get_frame_time(frame_clock) / 1e6);
    if (!r) { // should stop
        return G_SOURCE_REMOVE;
    }
    auto s = zoom_animator.get_s();

    cam_distance = zoom_animation_cam_dist_orig * pow(1.5, s);
    queue_draw();

    if (std::abs((s - zoom_animator.target) / std::max(std::abs(zoom_animator.target), 1.f)) < .005) {
        cam_distance = zoom_animation_cam_dist_orig * pow(1.5, zoom_animator.target);
        queue_draw();
        zoom_animator.stop();
        return G_SOURCE_REMOVE;
    }

    return G_SOURCE_CONTINUE;
}

static int tick_cb(GtkWidget *cwidget, GdkFrameClock *frame_clock, gpointer user_data)
{
    Gtk::Widget *widget = Glib::wrap(cwidget);
    auto canvas = dynamic_cast<Canvas3D *>(widget);
    return canvas->_animate_step(frame_clock);
}


bool Canvas3D::on_scroll_event(GdkEventScroll *scroll_event)
{

    float inc = 0;
    float factor = 1;
    if (scroll_event->state & Gdk::SHIFT_MASK)
        factor = .5;
    if (scroll_event->direction == GDK_SCROLL_UP) {
        inc = -1;
    }
    else if (scroll_event->direction == GDK_SCROLL_DOWN) {
        inc = 1;
    }
    else if (scroll_event->direction == GDK_SCROLL_SMOOTH) {
        gdouble sx, sy;
        gdk_event_get_scroll_deltas((GdkEvent *)scroll_event, &sx, &sy);
        inc = sy;
    }
    inc *= factor;
    if (smooth_zoom) {
        if (inc == 0)
            return Gtk::GLArea::on_scroll_event(scroll_event);
        if (!zoom_animator.is_running()) {
            zoom_animator.start();
            zoom_animation_cam_dist_orig = cam_distance;
            gtk_widget_add_tick_callback(GTK_WIDGET(gobj()), &tick_cb, nullptr, nullptr);
        }
        zoom_animator.target += inc;
    }
    else {
        cam_distance *= pow(1.5, inc);
        queue_draw();
    }


    return Gtk::GLArea::on_scroll_event(scroll_event);
}

void Canvas3D::inc_cam_azimuth(float v)
{
    cam_azimuth += v;
    while (cam_azimuth < 0)
        cam_azimuth += 360;

    while (cam_azimuth > 360)
        cam_azimuth -= 360;
    queue_draw();
}

void Canvas3D::view_all()
{
    Canvas3DBase::view_all();
    queue_draw();
}

void Canvas3D::request_push()
{
    needs_push = true;
    queue_draw();
}

int Canvas3D::a_get_scale_factor() const
{
    return get_scale_factor();
}

void Canvas3D::on_realize()
{
    Gtk::GLArea::on_realize();
    make_current();
    a_realize();
}

void Canvas3D::update(const Board &b)
{
    needs_view_all = brd == nullptr;
    brd = &b;
    ca.update(*brd);
    prepare_packages();
    prepare();
}

void Canvas3D::prepare()
{
    Canvas3DBase::prepare();
    request_push();
}

void Canvas3D::load_models_thread(std::map<std::string, std::string> model_filenames)
{
    class FilenameItem {
    public:
        FilenameItem(const std::string &fn, const std::string &fn_abs, goffset sz)
            : filename(fn), filename_abs(fn_abs), size(sz)
        {
        }
        std::string filename;
        std::string filename_abs;
        goffset size;
    };
    std::vector<FilenameItem> model_filenames_sorted;
    model_filenames_sorted.reserve(model_filenames.size());
    std::transform(model_filenames.begin(), model_filenames.end(), std::back_inserter(model_filenames_sorted),
                   [](const auto &x) {
                       auto fi = Gio::File::create_for_path(x.second);
                       auto sz = fi->query_info(G_FILE_ATTRIBUTE_STANDARD_SIZE)->get_size();
                       return FilenameItem(x.first, x.second, sz);
                   });
    std::sort(model_filenames_sorted.begin(), model_filenames_sorted.end(),
              [](const auto &a, const auto &b) { return a.size < b.size; });

    for (const auto &it : model_filenames_sorted) {
        load_3d_model(it.filename, it.filename_abs);
        i_model_loading++;
        models_loading_dispatcher.emit();
        if (stop_model_load_thread)
            return;
    }
}

void Canvas3D::load_models_async(Pool &pool)
{
    std::map<std::string, std::string> model_filenames = get_model_filenames(pool); // first: relative, second: absolute

    map_erase_if(model_filenames,
                 [](const auto &it) { return !Glib::file_test(it.second, Glib::FILE_TEST_IS_REGULAR); });

    if (model_filenames.size() == 0)
        return;

    if (model_load_thread.joinable())
        return;

    n_models_loading = model_filenames.size();
    i_model_loading = 0;
    s_signal_models_loading.emit(i_model_loading, n_models_loading);
    model_load_thread = std::thread(&Canvas3D::load_models_thread, this, model_filenames);
}

void Canvas3D::update_packages()
{
    prepare_packages();
    request_push();
}

void Canvas3D::set_highlights(const std::set<UUID> &pkgs)
{
    packages_highlight = pkgs;
    update_packages();
}

void Canvas3D::set_msaa(unsigned int samples)
{
    num_samples = samples;
    needs_resize = true;
    queue_draw();
}

bool Canvas3D::on_render(const Glib::RefPtr<Gdk::GLContext> &context)
{
    if (needs_push) {
        push();
        needs_push = false;
    }
    if (needs_resize) {
        resize_buffers();
        needs_resize = false;
    }

    render();

    return Gtk::GLArea::on_render(context);
}

Canvas3D::~Canvas3D()
{
    stop_model_load_thread = true;
    if (model_load_thread.joinable()) {
        model_load_thread.join();
    }
}

} // namespace horizon
