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
#include "util/warp_cursor.hpp"

#ifdef HAVE_SPNAV
#include <spnav.h>
#endif

namespace horizon {

static bool layer_counts(int layer)
{
    return layer < 10000 /* not cloned substrate */ || layer == 20000 /*pth*/;
}

Canvas3D::Canvas3D() : i_model_loading(0), stop_model_load_thread(false)
{
    add_events(Gdk::BUTTON_PRESS_MASK | Gdk::BUTTON_RELEASE_MASK | Gdk::BUTTON_MOTION_MASK | Gdk::SCROLL_MASK
               | Gdk::SMOOTH_SCROLL_MASK);

    models_loading_dispatcher.connect([this] {
        update_max_package_height();
        request_push();
        invalidate_pick();
        s_signal_models_loading.emit(i_model_loading, n_models_loading);
        if ((i_model_loading >= n_models_loading) && model_load_thread.joinable()) {
            model_load_thread.join();
        }
    });

    prepare_dispatcher.connect([this] {
        for (auto &[i, l] : ca.get_layers()) {
            if (l.done && layers_to_be_moved.count(i)) {
                layers_local.at(i).move_from(std::move(l));
                layers_to_be_moved.erase(i);
                request_push();
                invalidate_pick();
            }
        }
        {
            const size_t layers_total = std::count_if(layers_local.begin(), layers_local.end(),
                                                      [](auto &x) { return layer_counts(x.first); });
            const size_t layers_left = std::count_if(layers_to_be_moved.begin(), layers_to_be_moved.end(),
                                                     [](auto x) { return layer_counts(x); });
            s_signal_layers_loading.emit(layers_total - layers_left, layers_total);
        }
        if (layers_to_be_moved.size() == 0 && prepare_thread.joinable()) {
            prepare_thread.join();
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

    signal_pick_ready().connect([this] {
        const auto pick_result = pick_package_or_point(pick_x, pick_y);
        if (auto uu = std::get_if<UUID>(&pick_result)) {
            s_signal_package_select.emit(*uu);
        }
        else if (auto pt = std::get_if<glm::dvec3>(&pick_result)) {
            s_signal_point_select.emit(*pt);
        }
    });

#ifdef HAVE_SPNAV
    if (spnav_open() != -1) {
        have_spnav = true;
        if (auto fd = spnav_fd(); fd != -1) {
            auto chan = Glib::IOChannel::create_from_fd(fd);
            spnav_connection = Glib::signal_io().connect(
                    [this](Glib::IOCondition cond) {
                        if (cond & Glib::IO_HUP) {
                            Logger::log_warning("disconnected from spacenavd", Logger::Domain::CANVAS);
                            return false;
                        }
                        handle_spnav();
                        return true;
                    },
                    chan, Glib::IO_IN | Glib::IO_HUP);
        }
        else {
            Logger::log_warning("couldn't get fd from spacenavd", Logger::Domain::CANVAS);
        }
    }
#endif

    set_can_focus(true);
    animators.push_back(&azimuth_animator);
    animators.push_back(&elevation_animator);
    animators.push_back(&zoom_animator);
    animators.push_back(&cx_animator);
    animators.push_back(&cy_animator);
}

#ifdef HAVE_SPNAV
void Canvas3D::handle_spnav()
{
    spnav_event e;
    auto top = dynamic_cast<Gtk::Window *>(get_ancestor(GTK_TYPE_WINDOW));

    while (spnav_poll_event(&e)) {
        if (!top->is_active())
            continue;
        switch (e.type) {
        case SPNAV_EVENT_MOTION: {
            const auto thre = spacenav_prefs.threshold;
            const auto values = {e.motion.x, e.motion.y, e.motion.z, e.motion.rx, e.motion.rz};
            if (std::any_of(values.begin(), values.end(), [thre](auto x) { return std::abs(x) > thre; })) {
                start_anim();
                const auto center_shift =
                        get_center_shift(glm::vec2(e.motion.x, e.motion.y) * spacenav_prefs.pan_scale);
                cx_animator.target += center_shift.x;
                cy_animator.target += center_shift.y;
                zoom_animator.target += e.motion.z * spacenav_prefs.zoom_scale;
                azimuth_animator.target += e.motion.rz * spacenav_prefs.rotation_scale;
                elevation_animator.target += e.motion.rx * spacenav_prefs.rotation_scale;
            }
        } break;

        case SPNAV_EVENT_BUTTON: {
            if (e.button.press)
                s_signal_spacenav_button_press.emit(e.button.bnum);
        } break;
        }
    }
}
#endif

void Canvas3D::set_msd_params(const MSD::Params &params)
{
    for (auto anim : animators) {
        anim->set_params(params);
    }
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
        Canvas3DBase::view_all(); // don't use animation
        queue_draw();
        needs_view_all = false;
    }

    // chain up
    Gtk::GLArea::on_size_allocate(alloc);
}

bool Canvas3D::on_button_press_event(GdkEventButton *button_event)
{
    grab_focus();

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

bool Canvas3D::on_motion_notify_event(GdkEventMotion *motion_event)
{
    grab_focus();

    const auto delta = glm::mat2(1, 0, 0, -1) * (glm::vec2(motion_event->x, motion_event->y) - pointer_pos_orig);
    const auto warp_distance = warp_cursor((GdkEvent *)motion_event, *this);
    if (pan_mode == PanMode::ROTATE) {
        set_cam_azimuth(cam_azimuth_orig - (delta.x / width) * 360);
        set_cam_elevation(cam_elevation_orig - (delta.y / height) * 90);
    }
    else if (pan_mode == PanMode::MOVE) {
        set_center(center_orig + get_center_shift(delta));
    }
    pointer_pos_orig += glm::vec2(warp_distance.x, warp_distance.y);
    return Gtk::GLArea::on_motion_notify_event(motion_event);
}

bool Canvas3D::on_button_release_event(GdkEventButton *button_event)
{
    if (pan_mode == PanMode::ROTATE) {
        if (glm::length((glm::vec2(button_event->x, button_event->y) - pointer_pos_orig)) < 50) {
            pick_x = button_event->x;
            pick_y = button_event->y;
            queue_pick();
        }
    }
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
        set_center(gesture_drag_center_orig + get_center_shift({x, -y}));
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
    set_cam_distance(gesture_zoom_cam_dist_orig / delta);
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
    set_cam_azimuth(gesture_rotate_cam_azimuth_orig + glm::degrees(delta));
    double cx, cy;
    gesture_rotate->get_bounding_box_center(cx, cy);
    auto dy = cy - gesture_rotate_pos_orig.y;
    set_cam_elevation(gesture_rotate_cam_elevation_orig + (dy / height) * 180);
}

bool Canvas3D::on_scroll_event(GdkEventScroll *scroll_event)
{
    auto *dev = gdk_event_get_source_device((GdkEvent *)scroll_event);
    auto src = gdk_device_get_source(dev);
    if (src == GDK_SOURCE_TRACKPOINT || (src == GDK_SOURCE_TOUCHPAD && touchpad_pan)) {
        if (scroll_event->state & GDK_CONTROL_MASK) {
            pan_zoom(scroll_event);
        }
        else if (scroll_event->state & GDK_SHIFT_MASK) {
            pan_rotate(scroll_event);
        }
        else {
            pan_drag_move(scroll_event);
        }
    }
    else {
        pan_zoom(scroll_event);
    }

    return Gtk::GLArea::on_scroll_event(scroll_event);
}


void Canvas3D::pan_drag_move(GdkEventScroll *scroll_event)
{
    gdouble dx, dy;
    gdk_event_get_scroll_deltas((GdkEvent *)scroll_event, &dx, &dy);

    auto delta = glm::vec2(dx * -50, dy * 50);

    set_center(get_center() + get_center_shift(delta));
}

void Canvas3D::pan_rotate(GdkEventScroll *scroll_event)
{
    gdouble dx, dy;
    gdk_event_get_scroll_deltas((GdkEvent *)scroll_event, &dx, &dy);

    auto delta = -glm::vec2(dx, dy);

    set_cam_azimuth(get_cam_azimuth() - delta.x * 9);
    set_cam_elevation(get_cam_elevation() - delta.y * 9);
}


static const float zoom_base = 1.5;

static float cam_dist_to_anim(float d)
{
    return log(d) / log(zoom_base);
}

static float cam_dist_from_anim(float d)
{
    return pow(zoom_base, d);
}

void Canvas3D::pan_zoom(GdkEventScroll *scroll_event)
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
            return;
        start_anim();
        zoom_animator.target += inc;
    }
    else {
        set_cam_distance(cam_distance * pow(zoom_base, inc));
    }
}

int Canvas3D::animate_step(GdkFrameClock *frame_clock)
{
    bool stop = true;
    for (auto anim : animators) {
        if (anim->step(gdk_frame_clock_get_frame_time(frame_clock) / 1e6))
            stop = false;
    }

    set_cam_azimuth(azimuth_animator.get_s());
    set_cam_elevation(elevation_animator.get_s());
    set_cam_distance(cam_dist_from_anim(zoom_animator.get_s()));
    set_center({cx_animator.get_s(), cy_animator.get_s()});

    if (stop)
        return G_SOURCE_REMOVE;
    else
        return G_SOURCE_CONTINUE;
}

int Canvas3D::anim_tick_cb(GtkWidget *cwidget, GdkFrameClock *frame_clock, gpointer user_data)
{
    Gtk::Widget *widget = Glib::wrap(cwidget);
    auto canvas = dynamic_cast<Canvas3D *>(widget);
    return canvas->animate_step(frame_clock);
}


void Canvas3D::start_anim()
{
    const bool was_stopped = !std::any_of(animators.begin(), animators.end(), [](auto x) { return x->is_running(); });

    if (!azimuth_animator.is_running())
        azimuth_animator.start(cam_azimuth);

    if (!elevation_animator.is_running())
        elevation_animator.start(cam_elevation);

    if (!zoom_animator.is_running())
        zoom_animator.start(cam_dist_to_anim(cam_distance));

    if (!cx_animator.is_running())
        cx_animator.start(center.x);

    if (!cy_animator.is_running())
        cy_animator.start(center.y);

    if (was_stopped)
        gtk_widget_add_tick_callback(GTK_WIDGET(gobj()), &Canvas3D::anim_tick_cb, nullptr, nullptr);
}

void Canvas3D::animate_to_azimuth_elevation_abs(float az, float el)
{
    if (!smooth_zoom) {
        set_cam_azimuth(az);
        set_cam_elevation(el);
        return;
    }
    start_anim();

    azimuth_animator.target = az;
    elevation_animator.target = el;
}

void Canvas3D::animate_to_azimuth_elevation_rel(float az, float el)
{
    if (!smooth_zoom) {
        set_cam_azimuth(get_cam_azimuth() + az);
        set_cam_elevation(get_cam_elevation() + el);
        return;
    }
    start_anim();

    azimuth_animator.target += az;
    elevation_animator.target += el;
}

void Canvas3D::animate_zoom_step(int inc)
{
    if (!smooth_zoom) {
        set_cam_distance(get_cam_distance() * pow(zoom_base, inc));
        return;
    }
    start_anim();

    zoom_animator.target += inc;
}

void Canvas3D::animate_center_rel(const glm::vec2 &d)
{
    if (!smooth_zoom) {
        set_center(get_center() + d);
        return;
    }
    start_anim();
    cx_animator.target += d.x;
    cy_animator.target += d.y;
}

void Canvas3D::view_all()
{
    if (!smooth_zoom) {
        Canvas3DBase::view_all();
        queue_draw();
        return;
    }
    if (const auto p = get_view_all_params()) {
        start_anim();
        azimuth_animator.target = p->cam_azimuth;
        elevation_animator.target = p->cam_elevation;
        zoom_animator.target = cam_dist_to_anim(p->cam_distance);
        cx_animator.target = p->cx;
        cy_animator.target = p->cy;
    }
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

const std::map<int, CanvasMesh::Layer3D> &Canvas3D::get_layers() const
{
    return layers_local;
}

void Canvas3D::update(const Board &b)
{
    if (prepare_thread.joinable())
        return;
    needs_view_all = brd == nullptr;
    brd = &b;
    ca.update_only(*brd);
    for (const auto &[i, l] : ca.get_layers()) {
        layers_local[i].copy_sizes_from(l);
    }
    map_erase_if(layers_local, [this](auto &x) { return ca.get_layers().count(x.first) == 0; });
    prepare_packages();
    prepare();
    layers_to_be_moved.clear();
    for (auto &[i, l] : layers_local) {
        layers_to_be_moved.insert(i);
    }
    s_signal_layers_loading.emit(0, 1); // something to get it to show up
    prepare_thread = std::thread(&Canvas3D::prepare_thread_worker, this);
}

void Canvas3D::prepare_thread_worker()
{
    ca.prepare_only([this] { prepare_dispatcher.emit(); });
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

void Canvas3D::load_models_async(IPool &pool)
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
        invalidate_pick();
        needs_resize = false;
    }

    render();

    return Gtk::GLArea::on_render(context);
}

Canvas3D::~Canvas3D()
{
#ifdef HAVE_SPNAV
    if (have_spnav) {
        spnav_connection.disconnect();
        spnav_close();
    }
#endif

    stop_model_load_thread = true;
    if (model_load_thread.joinable()) {
        model_load_thread.join();
    }
    ca.cancel_prepare();
    if (prepare_thread.joinable()) {
        prepare_thread.join();
    }
}

void Canvas3D::redraw()
{
    queue_draw();
}

} // namespace horizon
