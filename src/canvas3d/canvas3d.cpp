#include "canvas3d.hpp"
#include "board/board.hpp"
#include "board/board_layers.hpp"
#include "canvas/gl_util.hpp"
#include "common/hole.hpp"
#include "common/polygon.hpp"
#include "logger/logger.hpp"
#include "poly2tri/poly2tri.h"
#include "util/step_importer.hpp"
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

Canvas3D::Canvas3D()
    : Gtk::GLArea(), CanvasPatch::CanvasPatch(), cover_renderer(this), wall_renderer(this), face_renderer(this),
      background_renderer(this), center(0)
{
    set_required_version(4, 2);
    add_events(Gdk::BUTTON_PRESS_MASK | Gdk::BUTTON_RELEASE_MASK | Gdk::BUTTON_MOTION_MASK | Gdk::SCROLL_MASK
               | Gdk::SMOOTH_SCROLL_MASK);

    models_loading_dispatcher.connect([this] {
        package_height_max = 0;
        for (const auto &it : face_vertex_buffer) {
            package_height_max = std::max(it.z, package_height_max);
        }
        request_push();
        s_signal_models_loading.emit(false);
    });
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

bool Canvas3D::on_motion_notify_event(GdkEventMotion *motion_event)
{
    auto delta = glm::mat2(1, 0, 0, -1) * (glm::vec2(motion_event->x, motion_event->y) - pointer_pos_orig);
    if (pan_mode == PanMode::ROTATE) {
        cam_azimuth = cam_azimuth_orig - (delta.x / width) * 360;
        cam_elevation = cam_elevation_orig - (delta.y / height) * 90;
        while (cam_elevation >= 360)
            cam_elevation -= 360;
        while (cam_elevation < 0)
            cam_elevation += 360;
        if (cam_elevation > 180)
            cam_elevation -= 360;
        queue_draw();
    }
    else if (pan_mode == PanMode::MOVE) {
        center = center_orig
                 + glm::rotate(glm::mat2(1, 0, 0, sin(glm::radians(cam_elevation))) * delta * 0.1f * cam_distance
                                       / 105.f,
                               glm::radians(cam_azimuth - 90));
        queue_draw();
    }
    return Gtk::GLArea::on_motion_notify_event(motion_event);
}

bool Canvas3D::on_button_release_event(GdkEventButton *button_event)
{
    pan_mode = PanMode::NONE;
    return Gtk::GLArea::on_button_release_event(button_event);
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

static const float magic_number = 0.4143;

void Canvas3D::view_all()
{
    if (!brd)
        return;

    const auto &vertices = layers[BoardLayers::L_OUTLINE].walls;
    MinMaxAccumulator<float> acc_x, acc_y;

    for (const auto &it : vertices) {
        acc_x.accumulate(it.x);
        acc_y.accumulate(it.y);
    }

    float xmin = acc_x.get_min();
    float xmax = acc_x.get_max();
    float ymin = acc_y.get_min();
    float ymax = acc_y.get_max();

    float board_width = (xmax - xmin) / 1e6;
    float board_height = (ymax - ymin) / 1e6;

    if (board_height < 1 || board_width < 1)
        return;

    center = {(xmin + xmax) / 2e6, (ymin + ymax) / 2e6};


    cam_distance = std::max(board_width / width, board_height / height) / (2 * magic_number / height) * 1.1;
    cam_azimuth = 270;
    cam_elevation = 89.99;
    queue_draw();
}

void Canvas3D::request_push()
{
    needs_push = true;
    queue_draw();
}

void Canvas3D::push()
{
    cover_renderer.push();
    wall_renderer.push();
    face_renderer.push();
}

void Canvas3D::resize_buffers()
{
    GLint rb;
    GLint samples = gl_clamp_samples(num_samples);
#ifdef __APPLE__
    samples = 0;
#endif
    glGetIntegerv(GL_RENDERBUFFER_BINDING, &rb); // save rb
    glBindRenderbuffer(GL_RENDERBUFFER, renderbuffer);
    glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, GL_RGBA8, width * get_scale_factor(),
                                     height * get_scale_factor());
    glBindRenderbuffer(GL_RENDERBUFFER, depthrenderbuffer);
    glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, GL_DEPTH_COMPONENT, width * get_scale_factor(),
                                     height * get_scale_factor());
    glBindRenderbuffer(GL_RENDERBUFFER, rb);
}

void Canvas3D::on_realize()
{
    Gtk::GLArea::on_realize();
    make_current();
    cover_renderer.realize();
    wall_renderer.realize();
    face_renderer.realize();
    background_renderer.realize();
    glEnable(GL_DEPTH_TEST);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    GLint fb;
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &fb); // save fb

    glGenRenderbuffers(1, &renderbuffer);
    glGenRenderbuffers(1, &depthrenderbuffer);

    resize_buffers();

    GL_CHECK_ERROR

    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, renderbuffer);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthrenderbuffer);

    GL_CHECK_ERROR

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        Gtk::MessageDialog md("Error setting up framebuffer, will now exit", false /* use_markup */, Gtk::MESSAGE_ERROR,
                              Gtk::BUTTONS_OK);
        md.run();
        abort();
    }
    glBindFramebuffer(GL_FRAMEBUFFER, fb);

    GL_CHECK_ERROR
}

void Canvas3D::add_path(int layer, const ClipperLib::Path &path)
{
    if (path.size() >= 3) {
        layers[layer].walls.emplace_back(path.back().X, path.back().Y);
        for (size_t i = 0; i < path.size(); i++) {
            layers[layer].walls.emplace_back(path[i].X, path[i].Y);
        }
        layers[layer].walls.emplace_back(path[0].X, path[0].Y);
        layers[layer].walls.emplace_back(path[1].X, path[1].Y);
        layers[layer].walls.emplace_back(NAN, NAN);
    }
}

static void append_path(std::vector<p2t::Point> &store, std::vector<p2t::Point *> &out,
                        std::set<std::pair<ClipperLib::cInt, ClipperLib::cInt>> &point_set,
                        const ClipperLib::Path &path)
{
    for (const auto &it : path) {
        auto p = std::make_pair(it.X, it.Y);
        bool a = false;
        bool fixed = false;
        while (point_set.count(p)) {
            fixed = true;
            if (a)
                p.first++;
            else
                p.second++;
            a = !a;
        }
        if (fixed) {
            Logger::log_warning("fixed duplicate point", Logger::Domain::BOARD,
                                "at " + coord_to_string(Coordf(it.X, it.Y)));
        }
        point_set.insert(p);
        store.emplace_back(p.first, p.second);
        out.push_back(&store.back());
    }
}

void Canvas3D::polynode_to_tris(const ClipperLib::PolyNode *node, int layer)
{
    assert(node->IsHole() == false);

    std::vector<p2t::Point> point_store;
    size_t pts_total = node->Contour.size();
    for (const auto child : node->Childs)
        pts_total += child->Contour.size();
    point_store.reserve(pts_total); // important so that iterators won't get invalidated
    std::set<std::pair<ClipperLib::cInt, ClipperLib::cInt>> point_set;

    try {
        std::vector<p2t::Point *> contour;
        contour.reserve(node->Contour.size());
        append_path(point_store, contour, point_set, node->Contour);
        p2t::CDT cdt(contour);
        for (const auto child : node->Childs) {
            std::vector<p2t::Point *> hole;
            hole.reserve(child->Contour.size());
            append_path(point_store, hole, point_set, child->Contour);
            cdt.AddHole(hole);
        }
        cdt.Triangulate();
        auto tris = cdt.GetTriangles();

        for (const auto &tri : tris) {
            for (int i = 0; i < 3; i++) {
                auto p = tri->GetPoint(i);
                layers[layer].tris.emplace_back(p->x, p->y);
            }
        }
    }
    catch (const std::runtime_error &e) {
        Logger::log_critical("error triangulating layer " + brd->get_layers().at(layer).name, Logger::Domain::BOARD,
                             e.what());
    }
    catch (...) {
        Logger::log_critical("error triangulating layer" + brd->get_layers().at(layer).name, Logger::Domain::BOARD,
                             "unspecified error");
    }

    layers[layer].walls.reserve(pts_total);
    add_path(layer, node->Contour);
    for (auto child : node->Childs) {
        add_path(layer, child->Contour);
    }

    for (auto child : node->Childs) {
        assert(child->IsHole() == true);
        for (auto child2 : child->Childs) { // add fragments in holes
            polynode_to_tris(child2, layer);
        }
    }
}

void Canvas3D::update2(const Board &b)
{
    needs_view_all = brd == nullptr;
    brd = &b;
    update(*brd);
    prepare_packages();
    prepare();
}

void Canvas3D::prepare()
{
    if (!brd)
        return;
    layers.clear();

    float board_thickness = -((float)brd->stackup.at(0).thickness);
    int n_inner_layers = brd->get_n_inner_layers();
    for (const auto &it : brd->stackup) {
        board_thickness += it.second.thickness + it.second.substrate_thickness;
    }
    board_thickness /= 1e6;

    int layer = BoardLayers::TOP_COPPER;
    layers[layer].offset = 0;
    layers[layer].thickness = brd->stackup.at(0).thickness / 1e6;
    layers[layer].explode_mul = 1;
    prepare_layer(layer);

    layer = BoardLayers::BOTTOM_COPPER;
    layers[layer].offset = -board_thickness;
    layers[layer].thickness = +(brd->stackup.at(layer).thickness / 1e6);
    layers[layer].explode_mul = -2 * n_inner_layers - 1;
    prepare_layer(layer);

    {
        float offset = -(brd->stackup.at(0).substrate_thickness / 1e6);
        for (int i = 0; i < n_inner_layers; i++) {
            layer = -i - 1;
            layers[layer].offset = offset;
            layers[layer].thickness = -(brd->stackup.at(layer).thickness / 1e6);
            layers[layer].explode_mul = -1 - 2 * i;
            offset -= brd->stackup.at(layer).thickness / 1e6 + brd->stackup.at(layer).substrate_thickness / 1e6;
            prepare_layer(layer);
        }
    }

    layer = BoardLayers::L_OUTLINE;
    layers[layer].offset = 0;
    layers[layer].thickness = -(brd->stackup.at(0).substrate_thickness / 1e6);
    layers[layer].explode_mul = 0;
    prepare_layer(layer);

    float offset = -(brd->stackup.at(0).substrate_thickness / 1e6);
    for (int i = 0; i < n_inner_layers; i++) {
        int l = 10000 + i;
        offset -= brd->stackup.at(-i - 1).thickness / 1e6;
        layers[l] = layers[layer];
        layers[l].offset = offset;
        layers[l].thickness = -(brd->stackup.at(-i - 1).substrate_thickness / 1e6);
        layers[l].explode_mul = -2 - 2 * i;

        offset -= brd->stackup.at(-i - 1).substrate_thickness / 1e6;
    }

    layer = BoardLayers::TOP_MASK;
    layers[layer].offset = brd->stackup.at(0).thickness / 1e6 + 1e-3;
    layers[layer].thickness = 0.01;
    layers[layer].alpha = .8;
    layers[layer].explode_mul = 3;
    prepare_soldermask(layer);

    layer = BoardLayers::BOTTOM_MASK;
    layers[layer].offset = -board_thickness - 1e-3;
    layers[layer].thickness = 0.035;
    layers[layer].alpha = .8;
    layers[layer].explode_mul = -2 * n_inner_layers - 3;
    prepare_soldermask(layer);

    layer = BoardLayers::TOP_SILKSCREEN;
    layers[layer].offset = brd->stackup.at(0).thickness / 1e6 + 1e-3;
    layers[layer].thickness = 0.035;
    layers[layer].explode_mul = 4;
    prepare_layer(layer);

    layer = BoardLayers::BOTTOM_SILKSCREEN;
    layers[layer].offset = -board_thickness - .1e-3;
    layers[layer].thickness = -0.035;
    layers[layer].explode_mul = -2 * n_inner_layers - 4;
    prepare_layer(layer);

    layer = BoardLayers::TOP_PASTE;
    layers[layer].offset = brd->stackup.at(0).thickness / 1e6 + 1e-3;
    layers[layer].thickness = 0.035;
    layers[layer].explode_mul = 2;
    prepare_layer(layer);

    layer = BoardLayers::BOTTOM_PASTE;
    layers[layer].offset = -board_thickness;
    layers[layer].thickness = -0.035;
    layers[layer].explode_mul = -2 * n_inner_layers - 2;
    prepare_layer(layer);

    layer = 20000; // pth holes
    layers[layer].offset = 0;
    layers[layer].thickness = -board_thickness;
    for (const auto &it : patches) {
        if (it.first.layer == 10000 && it.first.type == PatchType::HOLE_PTH) {
            ClipperLib::ClipperOffset ofs;
            ofs.AddPaths(it.second, ClipperLib::jtRound, ClipperLib::etClosedPolygon);
            ClipperLib::Paths res;
            ofs.Execute(res, -.001_mm);
            for (const auto &path : res) {
                add_path(layer, path);
            }
        }
    }

    bbox.first = glm::vec3();
    bbox.second = glm::vec3();
    for (const auto &it : patches) {
        for (const auto &path : it.second) {
            for (const auto &p : path) {
                glm::vec3 q(p.X / 1e6, p.Y / 1e6, 0);
                bbox.first = glm::min(bbox.first, q);
                bbox.second = glm::max(bbox.second, q);
            }
        }
    }
    request_push();
}

void Canvas3D::prepare_soldermask(int layer)
{
    ClipperLib::Paths temp;
    {
        ClipperLib::Clipper cl;
        for (const auto &it : patches) {
            if (it.first.layer == BoardLayers::L_OUTLINE) { // add outline
                cl.AddPaths(it.second, ClipperLib::ptSubject, true);
            }
            else if (it.first.layer == layer) {
                cl.AddPaths(it.second, ClipperLib::ptClip, true);
            }
        }

        cl.Execute(ClipperLib::ctDifference, temp, ClipperLib::pftEvenOdd, ClipperLib::pftNonZero);
    }
    ClipperLib::PolyTree pt;
    ClipperLib::ClipperOffset cl;
    cl.AddPaths(temp, ClipperLib::jtSquare, ClipperLib::etClosedPolygon);
    cl.Execute(pt, -.001_mm);

    for (const auto node : pt.Childs) {
        polynode_to_tris(node, layer);
    }
}

void Canvas3D::prepare_layer(int layer)
{
    ClipperLib::Clipper cl;
    for (const auto &it : patches) {
        if (it.first.layer == layer) {
            cl.AddPaths(it.second, ClipperLib::ptSubject, true);
        }
    }
    ClipperLib::Paths result;
    auto pft = ClipperLib::pftNonZero;
    if (layer == BoardLayers::L_OUTLINE) {
        pft = ClipperLib::pftEvenOdd;
    }
    cl.Execute(ClipperLib::ctUnion, result, pft);

    ClipperLib::PolyTree pt;
    cl.Clear();
    cl.AddPaths(result, ClipperLib::ptSubject, true);
    for (const auto &it : patches) {
        if (it.first.layer == 10000
            && (it.first.type == PatchType::HOLE_NPTH || it.first.type == PatchType::HOLE_PTH)) {
            cl.AddPaths(it.second, ClipperLib::ptClip, true);
        }
    }
    cl.Execute(ClipperLib::ctDifference, pt, pft, ClipperLib::pftNonZero);

    for (const auto node : pt.Childs) {
        polynode_to_tris(node, layer);
    }
}

float Canvas3D::get_layer_offset(int layer)
{
    return layers[layer].offset + layers[layer].explode_mul * explode;
}

float Canvas3D::get_layer_thickness(int layer) const
{
    if (layer == BoardLayers::L_OUTLINE && explode == 0) {
        return layers.at(BoardLayers::BOTTOM_COPPER).offset + layers.at(BoardLayers::BOTTOM_COPPER).thickness;
    }
    else {
        return layers.at(layer).thickness;
    }
}

void Canvas3D::load_3d_model(const std::string &filename, const std::string &filename_abs)
{
    if (models.count(filename))
        return;

    auto faces = STEPImporter::import(filename_abs);
    // canvas->face_vertex_buffer.reserve(faces.size());
    size_t vertex_offset = face_vertex_buffer.size();
    size_t first_index = face_index_buffer.size();
    for (const auto &face : faces) {
        for (const auto &v : face.vertices) {
            face_vertex_buffer.emplace_back(v.x, v.y, v.z, face.color.r * 255, face.color.g * 255, face.color.b * 255);
        }
        for (const auto &tri : face.triangle_indices) {
            size_t a, b, c;
            std::tie(a, b, c) = tri;
            face_index_buffer.push_back(a + vertex_offset);
            face_index_buffer.push_back(b + vertex_offset);
            face_index_buffer.push_back(c + vertex_offset);
        }
        vertex_offset += face.vertices.size();
    }
    size_t last_index = face_index_buffer.size();
    models.emplace(std::piecewise_construct, std::forward_as_tuple(filename),
                   std::forward_as_tuple(first_index, last_index - first_index));
}

void Canvas3D::load_models_thread(std::map<std::string, std::string> model_filenames)
{
    std::lock_guard<std::mutex> lock(models_loading_mutex);
    for (const auto &it : model_filenames) {
        load_3d_model(it.first, it.second);
    }
    models_loading_dispatcher.emit();
}

void Canvas3D::load_models_async(Pool *pool)
{
    std::map<std::string, std::string> model_filenames; // first: relative, second: absolute
    for (const auto &it : brd->packages) {
        auto model = it.second.package.get_model(it.second.model);
        if (model) {
            std::string model_filename;
            const Package *pool_package = nullptr;

            UUID this_pool_uuid;
            UUID pkg_pool_uuid;
            auto base_path = pool->get_base_path();
            const auto &pools = PoolManager::get().get_pools();
            if (pools.count(base_path))
                this_pool_uuid = pools.at(base_path).uuid;


            try {
                pool_package = pool->get_package(it.second.pool_package->uuid, &pkg_pool_uuid);
            }
            catch (const std::runtime_error &e) {
                // it's okay
            }
            if (it.second.pool_package == pool_package) {
                // package is from pool, ask pool for model filename (might come from cache)
                model_filename = pool->get_model_filename(it.second.pool_package->uuid, model->uuid);
            }
            else {
                // package is not from pool (from package editor, use filename relative to current pool)
                if (pkg_pool_uuid && pkg_pool_uuid != this_pool_uuid) { // pkg is open in RO mode from included pool
                    model_filename = pool->get_model_filename(it.second.pool_package->uuid, model->uuid);
                }
                else { // really editing package
                    model_filename = Glib::build_filename(pool->get_base_path(), model->filename);
                }
            }
            if (model_filename.size())
                model_filenames[model->filename] = model_filename;
        }
    }
    s_signal_models_loading.emit(true);
    std::thread thr(&Canvas3D::load_models_thread, this, model_filenames);

    thr.detach();
}

void Canvas3D::clear_3d_models()
{
    face_vertex_buffer.clear();
    face_index_buffer.clear();
    models.clear();
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

void Canvas3D::prepare_packages()
{
    if (!brd)
        return;
    package_transform_idxs.clear();
    package_transforms.clear();
    std::map<std::string, std::set<const BoardPackage *>> pkg_map;
    for (const auto &it : brd->packages) {
        auto model = it.second.package.get_model(it.second.model);
        if (model)
            pkg_map[model->filename].insert(&it.second);
    }

    for (const auto &it_pkg : pkg_map) {
        size_t size_before = package_transforms.size();
        for (const auto &it_brd_pkg : it_pkg.second) {
            const auto &pl = it_brd_pkg->placement;
            const auto &pkg = it_brd_pkg->package;
            package_transforms.emplace_back(pl.shift.x / 1e6, pl.shift.y / 1e6, pl.get_angle(), it_brd_pkg->flip,
                                            packages_highlight.count(it_brd_pkg->uuid));
            auto &tr = package_transforms.back();
            const auto model = pkg.get_model(it_brd_pkg->model);
            if (model) {
                tr.model_x = model->x / 1e6;
                tr.model_y = model->y / 1e6;
                tr.model_z = model->z / 1e6;
                tr.model_roll = model->roll;
                tr.model_pitch = model->pitch;
                tr.model_yaw = model->yaw;
            }
        }
        size_t size_after = package_transforms.size();
        package_transform_idxs[it_pkg.first] = {size_before, size_after - size_before};
    }
}

void Canvas3D::set_msaa(unsigned int samples)
{
    num_samples = samples;
    needs_resize = true;
    queue_draw();
}

bool Canvas3D::layer_is_visible(int layer) const
{
    if (layer == 20000) // pth holes
        return true;

    if (layer == BoardLayers::TOP_MASK || layer == BoardLayers::BOTTOM_MASK)
        return show_solder_mask;

    if (layer == BoardLayers::TOP_PASTE || layer == BoardLayers::BOTTOM_PASTE)
        return show_solder_paste;

    if (layer == BoardLayers::TOP_SILKSCREEN || layer == BoardLayers::BOTTOM_SILKSCREEN)
        return show_silkscreen;

    if (layer == BoardLayers::L_OUTLINE || layer >= 10000) {
        if (show_substrate) {
            if (layer == BoardLayers::L_OUTLINE)
                return true;
            else {
                return explode > 0;
            }
        }
        else {
            return false;
        }
    }
    if (layer < BoardLayers::TOP_COPPER && layer > BoardLayers::BOTTOM_COPPER)
        return show_substrate == false || explode > 0;

    return true;
}

Color Canvas3D::get_layer_color(int layer) const
{
    if (layer == 20000 || BoardLayers::is_copper(layer)) { // pth or cu
        if (use_layer_colors && appearance.layer_colors.count(layer)) {
            return appearance.layer_colors.at(layer);
        }
        return {1, .8, 0};
    }

    if (layer == BoardLayers::TOP_MASK || layer == BoardLayers::BOTTOM_MASK)
        return solder_mask_color;

    if (layer == BoardLayers::TOP_PASTE || layer == BoardLayers::BOTTOM_PASTE)
        return {.7, .7, .7};

    if (layer == BoardLayers::TOP_SILKSCREEN || layer == BoardLayers::BOTTOM_SILKSCREEN)
        return {1, 1, 1};

    if (layer == BoardLayers::L_OUTLINE || layer >= 10000)
        return substrate_color;
    return {1, 0, 0};
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

    GLint fb;
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &fb); // save fb

    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    glClearColor(.5, .5, .5, 1.0);
    glClearDepth(10);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    GL_CHECK_ERROR

    glDisable(GL_DEPTH_TEST);
    background_renderer.render();
    glEnable(GL_DEPTH_TEST);

    layers[20000].offset = get_layer_offset(BoardLayers::TOP_COPPER);
    layers[20000].thickness =
            -(get_layer_offset(BoardLayers::TOP_COPPER) - get_layer_offset(BoardLayers::BOTTOM_COPPER));

    float r = cam_distance;
    float phi = glm::radians(cam_azimuth);
    float theta = glm::radians(90 - cam_elevation);
    auto cam_offset = glm::vec3(r * sin(theta) * cos(phi), r * sin(theta) * sin(phi), r * cos(theta));
    auto cam_pos = cam_offset + glm::vec3(center, 0);

    glm::vec3 right(sin(phi - 3.14f / 2.0f), cos(phi - 3.14f / 2.0f), 0);

    viewmat = glm::lookAt(cam_pos, glm::vec3(center, 0), glm::vec3(0, 0, std::abs(cam_elevation) < 90 ? 1 : -1));

    float cam_dist_min = std::max(std::abs(cam_pos.z) - (10 + explode * (brd->get_n_inner_layers() * 2 + 3)), 1.0f);
    float cam_dist_max = 0;

    float zmin = -10 - explode * (brd->get_n_inner_layers() * 2 + 3 + package_height_max);
    float zmax = 10 + explode * 2 + package_height_max;
    std::array<glm::vec3, 8> bbs = {
            glm::vec3(bbox.first.x, bbox.first.y, zmin),  glm::vec3(bbox.first.x, bbox.second.y, zmin),
            glm::vec3(bbox.second.x, bbox.first.y, zmin), glm::vec3(bbox.second.x, bbox.second.y, zmin),
            glm::vec3(bbox.first.x, bbox.first.y, zmax),  glm::vec3(bbox.first.x, bbox.second.y, zmax),
            glm::vec3(bbox.second.x, bbox.first.y, zmax), glm::vec3(bbox.second.x, bbox.second.y, zmax)};

    for (const auto &bb : bbs) {
        float dist = glm::length(bb - cam_pos);
        cam_dist_max = std::max(dist, cam_dist_max);
        cam_dist_min = std::min(dist, cam_dist_min);
    }
    float m = magic_number / height * cam_distance;
    float d = cam_dist_max * 2;
    if (projection == Projection::PERSP) {
        projmat = glm::perspective(glm::radians(cam_fov), width / height, cam_dist_min / 2, cam_dist_max * 2);
    }
    else {
        projmat = glm::ortho(-width * m, width * m, -height * m, height * m, -d, d);
    }

    cam_normal = glm::normalize(cam_offset);
    wall_renderer.render();

    if (show_models)
        face_renderer.render();

    cover_renderer.render();

    GL_CHECK_ERROR

    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fb);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo);
    glBlitFramebuffer(0, 0, width * get_scale_factor(), height * get_scale_factor(), 0, 0, width * get_scale_factor(),
                      height * get_scale_factor(), GL_COLOR_BUFFER_BIT, GL_NEAREST);

    glBindFramebuffer(GL_FRAMEBUFFER, fb);
    GL_CHECK_ERROR
    glFlush();

    return Gtk::GLArea::on_render(context);
}
} // namespace horizon
