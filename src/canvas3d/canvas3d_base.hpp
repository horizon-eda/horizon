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
#include <sigc++/sigc++.h>

namespace horizon {

class Canvas3DBase {
public:
    Canvas3DBase();
    friend class CoverRenderer;
    friend class WallRenderer;
    friend class FaceRenderer;
    friend class BackgroundRenderer;
    Color get_layer_color(int layer) const;

    enum class Projection { PERSP, ORTHO };

#define GET_SET_X(x_, t_, f_)                                                                                          \
    const auto &get_##x_() const                                                                                       \
    {                                                                                                                  \
        return x_;                                                                                                     \
    }                                                                                                                  \
    void set_##x_(const t_ &c)                                                                                         \
    {                                                                                                                  \
        x_ = c;                                                                                                        \
        redraw();                                                                                                      \
        f_                                                                                                             \
    }

#define GET_SET(x_, t_) GET_SET_X(x_, t_, )
#define GET_SET_PICK(x_, t_) GET_SET_X(x_, t_, invalidate_pick();)

    GET_SET(background_top_color, Color)
    GET_SET(background_bottom_color, Color)
    GET_SET_PICK(show_solder_mask, bool)
    GET_SET_PICK(show_silkscreen, bool)
    GET_SET_PICK(show_substrate, bool)
    GET_SET_PICK(show_models, bool)
    GET_SET_PICK(show_dnp_models, bool)
    GET_SET_PICK(show_solder_paste, bool)
    GET_SET(use_layer_colors, bool)
    GET_SET(solder_mask_color, Color)
    GET_SET(substrate_color, Color)
    GET_SET_PICK(explode, float)
    GET_SET_PICK(cam_distance, float)
    GET_SET_PICK(cam_fov, float)
    GET_SET_PICK(center, glm::vec2)
    GET_SET_PICK(projection, Projection)

#undef GET_SET
#undef GET_SET_X
#undef GET_SET_PICK

    const float &get_cam_elevation() const
    {
        return cam_elevation;
    }
    void set_cam_elevation(const float &ele);

    const float &get_cam_azimuth() const
    {
        return cam_azimuth;
    }
    void set_cam_azimuth(const float &az);


    class FaceVertex {
    public:
        FaceVertex(float ix, float iy, float iz, float inx, float iny, float inz, uint8_t ir, uint8_t ig, uint8_t ib)
            : x(ix), y(iy), z(iz), nx(inx), ny(iny), nz(inz), r(ir), g(ig), b(ib), _pad(0)
        {
        }
        float x;
        float y;
        float z;
        float nx;
        float ny;
        float nz;

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
    CanvasMesh ca;

    Appearance appearance;

    Color background_top_color;
    Color background_bottom_color;
    bool show_solder_mask = true;
    bool show_silkscreen = true;
    bool show_substrate = true;
    bool show_models = true;
    bool show_dnp_models = false;
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

    Projection projection = Projection::PERSP;


    int width = 100;
    int height = 100;

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
    virtual void redraw()
    {
    }
    void invalidate_pick();
    void prepare();
    void prepare_packages();

    unsigned int num_samples = 1;

    const class Board *brd = nullptr;

    std::set<UUID> packages_highlight;

    void load_3d_model(const std::string &filename, const std::string &filename_abs);

    std::map<std::string, std::string> get_model_filenames(class IPool &pool);

    std::mutex models_loading_mutex;

    void update_max_package_height();

    void queue_pick();
    typedef sigc::signal<void> type_signal_pick_ready;
    type_signal_pick_ready signal_pick_ready()
    {
        return s_signal_pick_ready;
    }
    UUID pick_package(unsigned int x, unsigned int y) const;

private:
    float get_layer_offset(int layer) const;
    float get_layer_thickness(int layer) const;
    bool layer_is_visible(int layer) const;

    std::pair<glm::vec3, glm::vec3> bbox;

    GLuint renderbuffer;
    GLuint fbo;
    GLuint depthrenderbuffer;
    GLuint pickrenderbuffer;

    GLuint fbo_downsampled;
    GLuint pickrenderbuffer_downsampled;

    enum class PickState { QUEUED, CURRENT, INVALID };
    PickState pick_state = PickState::INVALID;

    glm::mat4 viewmat;
    glm::mat4 projmat;
    glm::vec3 cam_normal;

    float package_height_max = 0;
    std::vector<FaceVertex> face_vertex_buffer;  // vertices of all models, sequentially
    std::vector<unsigned int> face_index_buffer; // indexes face_vertex_buffer to form triangles

    class ModelInfo {
    public:
        ModelInfo(size_t o, size_t n) : face_index_offset(o), count(n)
        {
        }
        const size_t face_index_offset; // offset in face_index_buffer
        const size_t count;             // number of items in face_index_buffer
        bool pushed = false;
    };
    std::map<std::string, ModelInfo> models; // key: filename

    std::vector<ModelTransform> package_transforms; // position and rotation of
                                                    // all board packages,
                                                    // grouped by package

    struct PackageInfo {
        size_t offset; // in package_transforms
        size_t n_packages;
        unsigned int pick_base;
        std::vector<UUID> pkg;
    };

    std::map<std::pair<std::string, bool>, PackageInfo> package_infos; // key: first: model filename second: nopopulate
    std::vector<uint16_t> pick_buf;

    float get_magic_number() const;

    type_signal_pick_ready s_signal_pick_ready;
};

} // namespace horizon
