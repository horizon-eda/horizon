#pragma once
#include <glm/glm.hpp>
#include "common/common.hpp"
#include "canvas_mesh.hpp"
#include "canvas/appearance.hpp"
#include "util/uuid.hpp"
#include <mutex>
#include "cover.hpp"
#include "face.hpp"
#include "wall.hpp"
#include "background.hpp"

namespace horizon {

class Canvas3DBase {
public:
    Canvas3DBase();
    friend class CoverRenderer;
    friend class WallRenderer;
    friend class FaceRenderer;
    friend class BackgroundRenderer;
    CanvasMesh ca;
    Color background_top_color;
    Color background_bottom_color;
    Color get_layer_color(int layer) const;

    bool show_solder_mask = true;
    bool show_silkscreen = true;
    bool show_substrate = true;
    bool show_models = true;
    bool show_solder_paste = true;
    bool use_layer_colors = false;
    Color solder_mask_color = {0, .5, 0};
    Color substrate_color = {.2, .15, 0};
    float explode = 0;
    float highlight_intensity = .5;

    float cam_azimuth = 90;
    float cam_elevation = 45;
    float cam_distance = 20;
    float cam_fov = 45;
    glm::vec2 center;


    enum class Projection { PERSP, ORTHO };
    Projection projection = Projection::PERSP;

    class FaceVertex {
    public:
        FaceVertex(float ix, float iy, float iz, uint8_t ir, uint8_t ig, uint8_t ib)
            : x(ix), y(iy), z(iz), r(ir), g(ig), b(ib), _pad(0)
        {
        }
        float x;
        float y;
        float z;

        uint8_t r;
        uint8_t g;
        uint8_t b;
        uint8_t _pad;
    } __attribute__((packed));

    class ModelTransform {
    public:
        ModelTransform(float ix, float iy, float a, bool flip, bool highlight)
            : x(ix), y(iy), angle(a), flags(flip | (highlight << 1))
        {
        }
        float x;
        float y;
        uint16_t angle;
        uint16_t flags;

        float model_x = 0;
        float model_y = 0;
        float model_z = 0;
        uint16_t model_roll = 0;
        uint16_t model_pitch = 0;
        uint16_t model_yaw = 0;
    } __attribute__((packed));
    void view_all();
    void clear_3d_models();

protected:
    Appearance appearance;

    float width = 100;
    float height = 100;

    CoverRenderer cover_renderer;
    WallRenderer wall_renderer;
    FaceRenderer face_renderer;
    BackgroundRenderer background_renderer;

    void a_realize();
    void resize_buffers();
    void push();
    enum class RenderBackground { YES, NO };
    void render(RenderBackground mode = RenderBackground::YES);
    virtual int a_get_scale_factor() const;
    void prepare();
    void prepare_packages();

    unsigned int num_samples = 1;

    const class Board *brd = nullptr;

    std::set<UUID> packages_highlight;

    void load_3d_model(const std::string &filename, const std::string &filename_abs);

    std::map<std::string, std::string> get_model_filenames(class Pool &pool);

    std::mutex models_loading_mutex;

    void update_max_package_height();

private:
    float get_layer_offset(int layer) const;
    float get_layer_thickness(int layer) const;
    bool layer_is_visible(int layer) const;

    std::pair<glm::vec3, glm::vec3> bbox;

    GLuint renderbuffer;
    GLuint fbo;
    GLuint depthrenderbuffer;

    glm::mat4 viewmat;
    glm::mat4 projmat;
    glm::vec3 cam_normal;

    float package_height_max = 0;
    std::vector<FaceVertex> face_vertex_buffer;              // vertices of all models, sequentially
    std::vector<unsigned int> face_index_buffer;             // indexes face_vertex_buffer to form triangles
    std::map<std::string, std::pair<size_t, size_t>> models; // key: filename value: first: offset in face_index_buffer
                                                             // second: no of indexes

    std::vector<ModelTransform> package_transforms; // position and rotation of
                                                    // all board packages,
                                                    // grouped by package
    std::map<std::string, std::pair<size_t, size_t>>
            package_transform_idxs; // key: model filename: value: first: offset
                                    // in package_transforms second: no of items

    float get_magic_number() const;
};

} // namespace horizon
