#pragma once
#include "util/gl_inc.h"
#include <unordered_map>
#include <cstddef>

namespace horizon {
class CoverRenderer {
public:
    CoverRenderer(class Canvas3DBase &c);
    void realize();
    void render();
    void push();

private:
    Canvas3DBase &ca;
    std::unordered_map<int, size_t> layer_offsets;
    size_t n_vertices = 0;
    void render(int layer);

    GLuint program;
    GLuint vao;
    GLuint vbo;

    GLuint view_loc;
    GLuint proj_loc;
    GLuint layer_offset_loc;
    GLuint layer_color_loc;
    GLuint cam_normal_loc;
    GLuint cam_pos_loc;
    GLuint light_pos_loc;
    GLuint light_color_loc;
    GLuint ambient_intensity_loc;
    GLuint specular_intensity_loc;
    GLuint specular_power_loc;
    GLuint diffuse_intensity_loc;
};
} // namespace horizon
