#include "canvas3d_base.hpp"
#include "util/step_importer.hpp"
#include "board/board_layers.hpp"
#include "board/board.hpp"
#include "canvas/gl_util.hpp"
#include "util/min_max_accumulator.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glibmm/miscutils.h>
#include "pool/pool_manager.hpp"

namespace horizon {

Canvas3DBase::Canvas3DBase()
    : center(0), cover_renderer(*this), wall_renderer(*this), face_renderer(*this), background_renderer(*this)
{
}

Color Canvas3DBase::get_layer_color(int layer) const
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

float Canvas3DBase::get_layer_offset(int layer) const
{
    if (layer == 20000)
        return get_layer_offset(BoardLayers::TOP_COPPER);

    else
        return ca.get_layer(layer).offset + ca.get_layer(layer).explode_mul * explode;
}

float Canvas3DBase::get_layer_thickness(int layer) const
{
    if (layer == BoardLayers::L_OUTLINE && explode == 0) {
        return ca.get_layer(BoardLayers::BOTTOM_COPPER).offset + ca.get_layer(BoardLayers::BOTTOM_COPPER).thickness;
    }
    else if (layer == 20000) {
        return -(get_layer_offset(BoardLayers::TOP_COPPER) - get_layer_offset(BoardLayers::BOTTOM_COPPER));
    }
    else {
        return ca.get_layer(layer).thickness;
    }
}

bool Canvas3DBase::layer_is_visible(int layer) const
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

void Canvas3DBase::a_realize()
{
    cover_renderer.realize();
    wall_renderer.realize();
    face_renderer.realize();
    background_renderer.realize();
    glEnable(GL_DEPTH_TEST);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE_MINUS_DST_ALPHA, GL_DST_ALPHA);

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
    glBindRenderbuffer(GL_RENDERBUFFER, rb);
}

void Canvas3DBase::push()
{
    cover_renderer.push();
    wall_renderer.push();
    face_renderer.push();
}

float Canvas3DBase::get_magic_number() const
{
    return tan(0.5 * glm::radians(cam_fov));
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
    glBlitFramebuffer(0, 0, width * a_get_scale_factor(), height * a_get_scale_factor(), 0, 0,
                      width * a_get_scale_factor(), height * a_get_scale_factor(), GL_COLOR_BUFFER_BIT, GL_NEAREST);

    glBindFramebuffer(GL_FRAMEBUFFER, fb);
    GL_CHECK_ERROR
    glFlush();
}

void Canvas3DBase::view_all()
{
    if (!brd)
        return;

    const auto &vertices = ca.get_layer(BoardLayers::L_OUTLINE).walls;
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


    cam_distance = std::max(board_width / width, board_height / height) / (2 * get_magic_number() / height) * 1.1;
    cam_azimuth = 270;
    cam_elevation = 89.99;
}

void Canvas3DBase::prepare()
{
    bbox.first = glm::vec3();
    bbox.second = glm::vec3();
    for (const auto &it : ca.get_patches()) {
        for (const auto &path : it.second) {
            for (const auto &p : path) {
                glm::vec3 q(p.X / 1e6, p.Y / 1e6, 0);
                bbox.first = glm::min(bbox.first, q);
                bbox.second = glm::max(bbox.second, q);
            }
        }
    }
}

int Canvas3DBase::a_get_scale_factor() const
{
    return 1;
}

void Canvas3DBase::prepare_packages()
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


void Canvas3DBase::load_3d_model(const std::string &filename, const std::string &filename_abs)
{
    if (models.count(filename))
        return;

    auto faces = STEPImporter::import(filename_abs);
    {
        std::lock_guard<std::mutex> lock(models_loading_mutex);
        // canvas->face_vertex_buffer.reserve(faces.size());
        size_t vertex_offset = face_vertex_buffer.size();
        size_t first_index = face_index_buffer.size();
        for (const auto &face : faces) {
            for (const auto &v : face.vertices) {
                face_vertex_buffer.emplace_back(v.x, v.y, v.z, face.color.r * 255, face.color.g * 255,
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


std::map<std::string, std::string> Canvas3DBase::get_model_filenames(Pool &pool)
{
    std::map<std::string, std::string> model_filenames; // first: relative, second: absolute
    for (const auto &it : brd->packages) {
        auto model = it.second.package.get_model(it.second.model);
        if (model) {
            std::string model_filename;
            const Package *pool_package = nullptr;

            UUID this_pool_uuid;
            UUID pkg_pool_uuid;
            auto base_path = pool.get_base_path();
            const auto &pools = PoolManager::get().get_pools();
            if (pools.count(base_path))
                this_pool_uuid = pools.at(base_path).uuid;


            try {
                pool_package = pool.get_package(it.second.pool_package->uuid, &pkg_pool_uuid);
            }
            catch (const std::runtime_error &e) {
                // it's okay
            }
            if (it.second.pool_package == pool_package) {
                // package is from pool, ask pool for model filename (might come from cache)
                model_filename = pool.get_model_filename(it.second.pool_package->uuid, model->uuid);
            }
            else {
                // package is not from pool (from package editor, use filename relative to current pool)
                if (pkg_pool_uuid && pkg_pool_uuid != this_pool_uuid) { // pkg is open in RO mode from included pool
                    model_filename = pool.get_model_filename(it.second.pool_package->uuid, model->uuid);
                }
                else { // really editing package
                    model_filename = Glib::build_filename(pool.get_base_path(), model->filename);
                }
            }
            if (model_filename.size())
                model_filenames[model->filename] = model_filename;
        }
    }
    return model_filenames;
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

} // namespace horizon
