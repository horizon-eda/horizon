#include "canvas3d_base.hpp"
#include "import_step/import.hpp"
#include "board/board_layers.hpp"
#include "board/board.hpp"
#include "canvas/gl_util.hpp"
#include "util/util.hpp"
#include "util/min_max_accumulator.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glibmm/miscutils.h>
#include "pool/pool_manager.hpp"
#include "pool/ipool.hpp"
#include "logger/log_util.hpp"

namespace horizon {

Canvas3DBase::Canvas3DBase()
    : center(0), cover_renderer(*this), wall_renderer(*this), face_renderer(*this), background_renderer(*this),
      point_renderer(*this)
{
}

const std::map<int, CanvasMesh::Layer3D> &Canvas3DBase::get_layers() const
{
    return ca.get_layers();
}

const CanvasMesh::Layer3D &Canvas3DBase::get_layer(int layer) const
{
    return get_layers().at(layer);
}

Color Canvas3DBase::get_layer_color(int layer) const
{
    if (CanvasMesh::layer_is_pth_barrel(layer) || BoardLayers::is_copper(layer)) { // pth or cu
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
        return silkscreen_color;

    if (CanvasMesh::layer_is_substrate(layer))
        return substrate_color;
    return {1, 0, 0};
}

float Canvas3DBase::get_layer_offset(int layer) const
{
    if (CanvasMesh::layer_is_pth_barrel(layer)) {
        const auto &span = get_layer(layer).span;
        return get_layer_offset(span.end());
    }

    else
        return get_layer(layer).offset + get_layer(layer).explode_mul * explode;
}

float Canvas3DBase::get_layer_thickness(int layer) const
{
    if (layer == BoardLayers::L_OUTLINE && explode == 0) {
        return get_layer(BoardLayers::BOTTOM_COPPER).offset + get_layer(BoardLayers::BOTTOM_COPPER).thickness;
    }
    else if (CanvasMesh::layer_is_pth_barrel(layer)) {
        const auto &span = get_layer(layer).span;
        return -(get_layer_offset(span.end()) - get_layer_offset(span.start()));
    }
    else {
        return get_layer(layer).thickness;
    }
}

bool Canvas3DBase::layer_is_visible(int layer) const
{
    if (CanvasMesh::layer_is_pth_barrel(layer)) {
        if (!show_copper)
            return false;
        // pth holes
        const auto &span = get_layer(layer).span;
        if (span == BoardLayers::layer_range_through) {
            // always show thru pths
        }
        else {
            // only show non-thru pths if substrate is hidden or explode
            return explode > 0 || !show_substrate;
        }
    }

    if (layer == BoardLayers::TOP_MASK || layer == BoardLayers::BOTTOM_MASK)
        return show_solder_mask;

    if (layer == BoardLayers::TOP_PASTE || layer == BoardLayers::BOTTOM_PASTE)
        return show_solder_paste;

    if (layer == BoardLayers::TOP_SILKSCREEN || layer == BoardLayers::BOTTOM_SILKSCREEN)
        return show_silkscreen;

    if (CanvasMesh::layer_is_substrate(layer)) {
        if (show_substrate) {
            if (brd->get_n_inner_layers()) {
                // has inner layers and potentially blind/buried vias
                if (explode > 0) {
                    // show inner layers and hide L_OUTLINE
                    return layer != BoardLayers::L_OUTLINE;
                }
                else {
                    // not exploded, we only need to render L_OUTLINE
                    return layer == BoardLayers::L_OUTLINE;
                }
            }
            else {
                // has no inner layers
                return true;
            }
        }
        else {
            return false;
        }
    }


    if (layer < BoardLayers::TOP_COPPER && layer > BoardLayers::BOTTOM_COPPER)
        return (show_substrate == false || explode > 0) && show_copper;

    if (BoardLayers::is_copper(layer))
        return show_copper;

    return true;
}

void Canvas3DBase::a_realize()
{
    cover_renderer.realize();
    wall_renderer.realize();
    face_renderer.realize();
    background_renderer.realize();
    point_renderer.realize();
    glEnable(GL_DEPTH_TEST);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE_MINUS_DST_ALPHA, GL_DST_ALPHA);

    GLint fb;
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &fb); // save fb

    glGenRenderbuffers(1, &renderbuffer);
    glGenRenderbuffers(1, &depthrenderbuffer);
    glGenRenderbuffers(1, &pickrenderbuffer);
    glGenRenderbuffers(1, &pickrenderbuffer_downsampled);

    resize_buffers();

    GL_CHECK_ERROR

    glGenFramebuffers(1, &fbo_downsampled);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_downsampled);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, pickrenderbuffer_downsampled);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        // Gtk::MessageDialog md("Error setting up framebuffer, will now exit", false /* use_markup */,
        // Gtk::MESSAGE_ERROR,
        //                      Gtk::BUTTONS_OK);
        // md.run();
        abort();
    }

    GL_CHECK_ERROR

    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, renderbuffer);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_RENDERBUFFER, pickrenderbuffer);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthrenderbuffer);

    GL_CHECK_ERROR

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        // Gtk::MessageDialog md("Error setting up framebuffer, will now exit", false /* use_markup */,
        // Gtk::MESSAGE_ERROR,
        //                      Gtk::BUTTONS_OK);
        // md.run();
        abort();
    }
    glBindFramebuffer(GL_FRAMEBUFFER, fb);

    GL_CHECK_ERROR
}


void Canvas3DBase::resize_buffers()
{
    GLint rb;
    GLint samples = gl_clamp_samples(num_samples);
    glGetIntegerv(GL_RENDERBUFFER_BINDING, &rb); // save rb
    glBindRenderbuffer(GL_RENDERBUFFER, renderbuffer);
    glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, GL_RGBA8, width * a_get_scale_factor(),
                                     height * a_get_scale_factor());
    glBindRenderbuffer(GL_RENDERBUFFER, depthrenderbuffer);
    glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, GL_DEPTH_COMPONENT, width * a_get_scale_factor(),
                                     height * a_get_scale_factor());
    glBindRenderbuffer(GL_RENDERBUFFER, pickrenderbuffer);
    glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, GL_R16UI, width * a_get_scale_factor(),
                                     height * a_get_scale_factor());

    glBindRenderbuffer(GL_RENDERBUFFER, pickrenderbuffer_downsampled);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_R16UI, width * a_get_scale_factor(), height * a_get_scale_factor());

    glBindRenderbuffer(GL_RENDERBUFFER, rb);
}

void Canvas3DBase::push()
{
    cover_renderer.push();
    wall_renderer.push();
    face_renderer.push();
    point_renderer.push();
}

float Canvas3DBase::get_magic_number() const
{
    return tan(0.5 * glm::radians(cam_fov));
}

static float fix_cam_elevation(float cam_elevation)
{
    while (cam_elevation >= 360)
        cam_elevation -= 360;
    while (cam_elevation < 0)
        cam_elevation += 360;
    if (cam_elevation > 180)
        cam_elevation -= 360;
    return cam_elevation;
}

void Canvas3DBase::set_cam_elevation(const float &ele)
{
    cam_elevation = fix_cam_elevation(ele);
    redraw();
    invalidate_pick();
    s_signal_view_changed.emit();
}

void Canvas3DBase::set_cam_azimuth(const float &az)
{
    cam_azimuth = az;
    while (cam_azimuth < 0)
        cam_azimuth += 360;

    while (cam_azimuth > 360)
        cam_azimuth -= 360;
    redraw();
    invalidate_pick();
    s_signal_view_changed.emit();
}

void Canvas3DBase::render(RenderBackground mode)
{

    GLint fb;
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &fb); // save fb

    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    glClearColor(0, 0, 0, 0);
    glClearDepth(10);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    GL_CHECK_ERROR
    {
        const std::array<GLenum, 2> bufs = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
        glDrawBuffers(bufs.size(), bufs.data());
    }

    if (mode == RenderBackground::YES) {
        glDisable(GL_DEPTH_TEST);
        background_renderer.render();
        glEnable(GL_DEPTH_TEST);
    }

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
    float m = get_magic_number() / height * cam_distance;
    float d = cam_dist_max * 2;
    if (projection == Projection::PERSP) {
        projmat = glm::perspective(glm::radians(cam_fov), (float)width / height, cam_dist_min / 2, cam_dist_max * 2);
    }
    else {
        projmat = glm::ortho(-width * m, width * m, -height * m, height * m, -d, d);
    }

    cam_normal = glm::normalize(cam_offset);
    wall_renderer.render();

    if (show_models)
        face_renderer.render();

    if (show_points)
        point_renderer.render();

    cover_renderer.render();

    GL_CHECK_ERROR

    if (pick_state == PickState::QUEUED) {
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo_downsampled);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo);
        glDrawBuffer(GL_COLOR_ATTACHMENT0);
        glReadBuffer(GL_COLOR_ATTACHMENT1);
        glBlitFramebuffer(0, 0, width * a_get_scale_factor(), height * a_get_scale_factor(), 0, 0,
                          width * a_get_scale_factor(), height * a_get_scale_factor(), GL_COLOR_BUFFER_BIT, GL_NEAREST);

        glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo_downsampled);
        glReadBuffer(GL_COLOR_ATTACHMENT0);
        pick_buf.resize(width * a_get_scale_factor() * height * a_get_scale_factor());
        GL_CHECK_ERROR

        glPixelStorei(GL_PACK_ALIGNMENT, 2);
        glReadPixels(0, 0, width * a_get_scale_factor(), height * a_get_scale_factor(), GL_RED_INTEGER,
                     GL_UNSIGNED_SHORT, pick_buf.data());

        GL_CHECK_ERROR
        pick_state = PickState::CURRENT;
        s_signal_pick_ready.emit();
    }

    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fb);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo);
    glDrawBuffer(fb ? GL_COLOR_ATTACHMENT0 : GL_FRONT);
    glReadBuffer(GL_COLOR_ATTACHMENT0);
    glBlitFramebuffer(0, 0, width * a_get_scale_factor(), height * a_get_scale_factor(), 0, 0,
                      width * a_get_scale_factor(), height * a_get_scale_factor(), GL_COLOR_BUFFER_BIT, GL_NEAREST);

    glBindFramebuffer(GL_FRAMEBUFFER, fb);
    GL_CHECK_ERROR
    glFlush();
}

std::optional<Canvas3DBase::ViewParams> Canvas3DBase::get_view_all_params() const
{
    if (!brd)
        return {};
    ViewParams r;

    auto bb = ca.get_bbox();

    float xmin = bb.first.x;
    float xmax = bb.second.x;
    float ymin = bb.first.y;
    float ymax = bb.second.y;

    float board_width = (xmax - xmin) / 1e6;
    float board_height = (ymax - ymin) / 1e6;

    if (board_height < 1 || board_width < 1)
        return {};

    r.cx = (xmin + xmax) / 2e6;
    r.cy = (ymin + ymax) / 2e6;

    r.cam_distance = std::max(board_width / width, board_height / height) / (2 * get_magic_number() / height) * 1.1;
    r.cam_azimuth = 270;
    r.cam_elevation = 89.99;

    return r;
}

void Canvas3DBase::view_all()
{
    if (const auto p = get_view_all_params()) {
        set_center({p->cx, p->cy});
        set_cam_distance(p->cam_distance);
        set_cam_azimuth(p->cam_azimuth);
        set_cam_elevation(p->cam_elevation);
    }
}

void Canvas3DBase::prepare()
{
    const auto bb = ca.get_bbox();
    bbox.first = glm::vec3(bb.first.x / 1e6, bb.first.y / 1e6, 0);
    bbox.second = glm::vec3(bb.second.x / 1e6, bb.second.y / 1e6, 0);
}

int Canvas3DBase::a_get_scale_factor() const
{
    return 1;
}

void Canvas3DBase::prepare_packages()
{
    if (!brd)
        return;
    package_infos.clear();
    package_transforms.clear();
    std::map<std::pair<std::string, bool>, std::set<const BoardPackage *>> pkg_map;
    for (const auto &it : brd->packages) {
        auto model = it.second.package.get_model(it.second.model);
        if (model)
            pkg_map[{model->filename, it.second.component ? it.second.component->nopopulate : false}].insert(
                    &it.second);
    }

    unsigned int pick_base = 2; // 0: background, 1: PCB, 2+: models
    for (const auto &it_pkg : pkg_map) {
        size_t size_before = package_transforms.size();
        std::vector<UUID> pkgs;
        for (const auto &it_brd_pkg : it_pkg.second) {
            pkgs.push_back(it_brd_pkg->uuid);
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
        package_infos[it_pkg.first] = {size_before, size_after - size_before, pick_base, pkgs};
        pick_base += pkgs.size();
    }
    point_pick_base = pick_base;
}

STEPImporter::Faces Canvas3DBase::import_step(const std::string &filename_rel, const std::string &filename_abs)
{
    const auto result = STEPImporter::import(filename_abs);
    return result.faces;
}

void Canvas3DBase::load_3d_model(const std::string &filename, const std::string &filename_abs)
{
    if (models.count(filename))
        return;

    const auto faces = import_step(filename, filename_abs);
    {
        std::lock_guard<std::mutex> lock(models_loading_mutex);
        // canvas->face_vertex_buffer.reserve(faces.size());
        size_t vertex_offset = face_vertex_buffer.size();
        size_t first_index = face_index_buffer.size();
        for (const auto &face : faces) {
            for (size_t i = 0; i < face.vertices.size(); i++) {
                const auto &v = face.vertices.at(i);
                const auto &n = face.normals.at(i);
                face_vertex_buffer.emplace_back(v.x, v.y, v.z, n.x, n.y, n.z, face.color.r * 255, face.color.g * 255,
                                                face.color.b * 255);
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
}

bool Canvas3DBase::model_is_loaded(const std::string &filename)
{
    std::lock_guard<std::mutex> lock(models_loading_mutex);
    return models.count(filename);
}

void Canvas3DBase::set_point_transform(const glm::dmat4 &mat)
{
    point_mat = mat;
    invalidate_pick();
    redraw();
}

void Canvas3DBase::set_points(const std::vector<Point3D> &pts)
{
    points = pts;
}

std::map<std::string, std::string> Canvas3DBase::get_model_filenames(IPool &pool)
{
    std::map<std::string, std::string> model_filenames; // first: relative, second: absolute
    for (const auto &it : brd->packages) {
        const auto model_filename = get_model_filename(it.second, pool);
        if (model_filename.has_value())
            model_filenames[model_filename->first] = model_filename->second;
    }
    return model_filenames;
}

std::optional<std::pair<std::string, std::string>> Canvas3DBase::get_model_filename(const BoardPackage &pkg,
                                                                                    IPool &pool)
{
    auto model = pkg.package.get_model(pkg.model);
    if (model)
        return std::make_pair(model->filename, pool.get_model_filename(pkg.package.uuid, model->uuid));
    else
        return {};
}

void Canvas3DBase::clear_3d_models()
{
    face_vertex_buffer.clear();
    face_index_buffer.clear();
    models.clear();
}

void Canvas3DBase::update_max_package_height()
{
    package_height_max = 0;
    for (const auto &it : face_vertex_buffer) {
        package_height_max = std::max(it.z, package_height_max);
    }
}

void Canvas3DBase::invalidate_pick()
{
    pick_state = PickState::INVALID;
}

void Canvas3DBase::queue_pick()
{
    if (pick_state == PickState::INVALID) {
        pick_state = PickState::QUEUED;
        redraw();
    }
    else if (pick_state == PickState::CURRENT) {
        s_signal_pick_ready.emit();
    }
}

std::variant<UUID, glm::dvec3> Canvas3DBase::pick_package_or_point(unsigned int x, unsigned int y) const
{
    if (pick_state != PickState::CURRENT) {
        Logger::log_warning("can't with non-current pick state");
        return {};
    }
    x *= a_get_scale_factor();
    y *= a_get_scale_factor();
    const int idx = ((height * a_get_scale_factor()) - y - 1) * width * a_get_scale_factor() + x;
    const auto pick = pick_buf.at(idx);
    if (pick >= point_pick_base) {
        const auto &pt = points.at(pick - point_pick_base);
        const auto p4 = point_mat * glm::dvec4(pt.x, pt.y, pt.z, 1);
        return glm::dvec3(p4.x, p4.y, p4.z);
    }
    for (const auto &[k, v] : package_infos) {
        if (pick >= v.pick_base && pick < (v.pick_base + v.n_packages)) {
            return v.pkg.at(pick - v.pick_base);
        }
    }
    return {};
}

static void acc(float &l, float &h, float v)
{
    l = std::min(l, v);
    h = std::max(h, v);
}

Canvas3DBase::BBox Canvas3DBase::get_model_bbox(const std::string &filename) const
{
    Canvas3DBase::BBox bb;
    bool first = true;
    if (models.count(filename)) {
        const auto &m = models.at(filename);
        for (size_t i = m.face_index_offset; i < m.face_index_offset + m.count; i++) {
            const auto idx = face_index_buffer.at(i);
            const auto &v = face_vertex_buffer.at(idx);
            if (first) {
                bb.xh = v.x;
                bb.xl = v.x;
                bb.yh = v.y;
                bb.yl = v.y;
                bb.zh = v.z;
                bb.zl = v.z;
            }
            else {
                acc(bb.xl, bb.xh, v.x);
                acc(bb.yl, bb.yh, v.y);
                acc(bb.zl, bb.zh, v.z);
            }
            first = false;
        }
    }
    else {
        bb = {0};
    }
    return bb;
}

} // namespace horizon
