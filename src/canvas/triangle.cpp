#include "triangle.hpp"
#include "canvas_gl.hpp"
#include "gl_util.hpp"
#include <glm/gtc/type_ptr.hpp>

namespace horizon {

static GLuint create_vao(GLuint program, GLuint &vbo_out, GLuint &ebo_out)
{
    auto err = glGetError();
    if (err != GL_NO_ERROR) {
        std::cout << "gl error t " << err << std::endl;
    }
    GLuint p0_index = glGetAttribLocation(program, "p0");
    GLuint p1_index = glGetAttribLocation(program, "p1");
    GLuint p2_index = glGetAttribLocation(program, "p2");
    GLuint type_index = glGetAttribLocation(program, "type");
    GLuint color_index = glGetAttribLocation(program, "color");
    GLuint lod_index = glGetAttribLocation(program, "lod");
    GLuint flags_index = glGetAttribLocation(program, "flags");
    GL_CHECK_ERROR;
    GLuint vao, buffer, ebuffer;

    glGenBuffers(1, &ebuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebuffer);

    /* we need to create a VAO to store the other buffers */
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    /* this is the VBO that holds the vertex data */
    glGenBuffers(1, &buffer);
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    // data is buffered lateron

    GLfloat vertices[] = {//   Position
                          0, 0, 7500000, 5000000, 2500000, -2500000, 1, 0, 1};
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    /* enable and set the position attribute */
    glEnableVertexAttribArray(p0_index);
    glVertexAttribPointer(p0_index, 2, GL_FLOAT, GL_FALSE, sizeof(Triangle), (void *)offsetof(Triangle, x0));
    glEnableVertexAttribArray(p1_index);
    glVertexAttribPointer(p1_index, 2, GL_FLOAT, GL_FALSE, sizeof(Triangle), (void *)offsetof(Triangle, x1));
    glEnableVertexAttribArray(p2_index);
    glVertexAttribPointer(p2_index, 2, GL_FLOAT, GL_FALSE, sizeof(Triangle), (void *)offsetof(Triangle, x2));
    /*glEnableVertexAttribArray (oid_index);
    glVertexAttribIPointer (oid_index, 1, GL_UNSIGNED_INT,
                                             sizeof(Triangle),
                                             (void*)offsetof(Triangle, oid));*/
    glEnableVertexAttribArray(type_index);
    glVertexAttribIPointer(type_index, 1, GL_UNSIGNED_BYTE, sizeof(Triangle), (void *)offsetof(Triangle, type));
    glEnableVertexAttribArray(color_index);
    glVertexAttribIPointer(color_index, 1, GL_UNSIGNED_BYTE, sizeof(Triangle), (void *)offsetof(Triangle, color));
    glEnableVertexAttribArray(lod_index);
    glVertexAttribIPointer(lod_index, 1, GL_UNSIGNED_BYTE, sizeof(Triangle), (void *)offsetof(Triangle, lod));

    glEnableVertexAttribArray(flags_index);
    glVertexAttribIPointer(flags_index, 1, GL_UNSIGNED_BYTE, sizeof(Triangle), (void *)offsetof(Triangle, flags));

    GL_CHECK_ERROR;

    /* enable and set the color attribute */
    /* reset the state; we will re-enable the VAO when needed */
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // glDeleteBuffers (1, &buffer);
    vbo_out = buffer;
    ebo_out = ebuffer;

    return vao;
}

TriangleRenderer::TriangleRenderer(CanvasGL *c, std::unordered_map<int, std::vector<Triangle>> &tris)
    : ca(c), triangles(tris)
{
}

void TriangleRenderer::realize()
{
    program_triangle =
            gl_create_program_from_resource("/net/carrotIndustries/horizon/canvas/shaders/triangle-vertex.glsl",
                                            "/net/carrotIndustries/horizon/canvas/shaders/"
                                            "triangle-triangle-fragment.glsl",
                                            "/net/carrotIndustries/horizon/canvas/shaders/"
                                            "triangle-triangle-geometry.glsl");
    program_line0 = gl_create_program_from_resource("/net/carrotIndustries/horizon/canvas/shaders/triangle-vertex.glsl",
                                                    "/net/carrotIndustries/horizon/canvas/shaders/"
                                                    "triangle-line0-fragment.glsl",
                                                    "/net/carrotIndustries/horizon/canvas/shaders/"
                                                    "triangle-line0-geometry.glsl");
    program_line_butt =
            gl_create_program_from_resource("/net/carrotIndustries/horizon/canvas/shaders/triangle-vertex.glsl",
                                            "/net/carrotIndustries/horizon/canvas/shaders/"
                                            "triangle-line-butt-fragment.glsl",
                                            "/net/carrotIndustries/horizon/canvas/shaders/"
                                            "triangle-line-butt-geometry.glsl");
    program_line = gl_create_program_from_resource("/net/carrotIndustries/horizon/canvas/shaders/triangle-vertex.glsl",
                                                   "/net/carrotIndustries/horizon/canvas/shaders/"
                                                   "triangle-line-fragment.glsl",
                                                   "/net/carrotIndustries/horizon/canvas/shaders/"
                                                   "triangle-line-geometry.glsl");
    GL_CHECK_ERROR;
    glGenBuffers(1, &ubo);
    glBindBuffer(GL_UNIFORM_BUFFER, ubo);
    float testd[3] = {1, 1, 1};
    glBufferData(GL_UNIFORM_BUFFER, sizeof(testd), &testd, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    GL_CHECK_ERROR;
    unsigned int block_index = glGetUniformBlockIndex(program_line, "layer_setup");
    GLuint binding_point_index = 0;
    glBindBufferBase(GL_UNIFORM_BUFFER, binding_point_index, ubo);
    glUniformBlockBinding(program_triangle, block_index, binding_point_index);
    glUniformBlockBinding(program_line0, block_index, binding_point_index);
    glUniformBlockBinding(program_line_butt, block_index, binding_point_index);
    glUniformBlockBinding(program_line, block_index, binding_point_index);
    GL_CHECK_ERROR;
    vao = create_vao(program_line, vbo, ebo);
    GL_CHECK_ERROR;
}


class UBOBuffer {
public:
    std::array<std::array<float, 4>, 14> colors; // 14==ColorP::N_COLORS
    std::array<float, 12> screenmat;
    std::array<float, 12> viewmat;
    std::array<float, 3> layer_color;
    float alpha;
    float scale;
    unsigned int types_visible;
    unsigned int types_force_outline;
    int layer_flags;
    int highlight_mode;
    float highlight_dim;
    float highlight_shadow;
    float highlight_lighten;
};

static void mat3_to_array(std::array<float, 12> &dest, const glm::mat3 &src)
{
    for (int r = 0; r < 3; r++) {
        for (int c = 0; c < 3; c++) {
            dest[r * 4 + c] = src[r][c];
        }
    }
}

void TriangleRenderer::render_layer(int layer)
{
    const auto &ld = ca->layer_display.at(layer);

    GL_CHECK_ERROR
    glBindBuffer(GL_UNIFORM_BUFFER, ubo);
    UBOBuffer buf;
    buf.colors = ca->layer_setup.colors;
    buf.alpha = ca->property_layer_opacity() / 100;
    mat3_to_array(buf.screenmat, ca->screenmat);
    mat3_to_array(buf.viewmat, ca->viewmat);
    buf.layer_color[0] = ld.color.r;
    buf.layer_color[1] = ld.color.g;
    buf.layer_color[2] = ld.color.b;
    buf.layer_flags = static_cast<int>(ld.mode);
    buf.types_visible = ld.types_visible;
    buf.types_force_outline = ld.types_force_outline;
    buf.scale = ca->scale;
    buf.highlight_mode = ca->highlight_enabled ? static_cast<int>(ca->highlight_mode) : 0;
    buf.highlight_dim = ca->highlight_dim;
    buf.highlight_shadow = ca->highlight_shadow;
    buf.highlight_lighten = ca->highlight_lighten;


    glBufferData(GL_UNIFORM_BUFFER, sizeof(buf), &buf, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    GL_CHECK_ERROR

    if (ld.mode == LayerDisplay::Mode::FILL_ONLY)
        glStencilFunc(GL_GREATER, stencil, 0xff);
    else
        glStencilFunc(GL_ALWAYS, stencil, 0xff);

    for (const auto &it : layer_offsets[layer]) {
        bool skip = false;
        switch (it.first) {
        case Type::TRIANGLE:
            glUseProgram(program_triangle);
            if (ld.mode == LayerDisplay::Mode::OUTLINE)
                skip = true;
            break;

        case Type::LINE0:
            glUseProgram(program_line0);
            break;

        case Type::LINE_BUTT:
            glUseProgram(program_line_butt);
            break;

        case Type::LINE:
            glUseProgram(program_line);
            break;

        case Type::GLYPH:
            break;
        }
        if (!skip)
            glDrawElements(GL_POINTS, it.second.second, GL_UNSIGNED_INT,
                           (void *)(it.second.first * sizeof(unsigned int)));
    }
    // glDrawArrays(GL_POINTS, layer_offsets[layer], triangles[layer].size());
    stencil++;
}

void TriangleRenderer::render_layer_with_overlay(int layer)
{
    render_layer(layer);
    if (ca->overlay_layers.count(layer))
        render_layer(ca->overlay_layers.at(layer));
}


void TriangleRenderer::render()
{
    glBindVertexArray(vao);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);

    GL_CHECK_ERROR

    std::vector<int> layers;
    layers.reserve(layer_offsets.size());
    for (const auto &it : layer_offsets) {
        if (ca->layer_display.count(it.first))
            layers.push_back(it.first);
    }
    std::sort(layers.begin(), layers.end());
    if (ca->work_layer < 0) {
        std::reverse(layers.begin(), layers.end());
    }

    glClear(GL_STENCIL_BUFFER_BIT);
    glStencilOp(GL_KEEP, GL_REPLACE, GL_REPLACE);
    glEnable(GL_STENCIL_TEST);
    stencil = 1;
    for (auto layer : layers) {
        const auto &ld = ca->layer_display.at(layer);
        if (layer != ca->work_layer && layer < 10000 && ld.visible) {
            render_layer_with_overlay(layer);
        }
    }
    if (ca->layer_display.count(ca->work_layer))
        render_layer_with_overlay(ca->work_layer);
    for (auto layer : layers) {
        const auto &ld = ca->layer_display.at(layer);
        if (layer >= 10000 && layer < 30000 && ld.visible) {
            render_layer(layer);
        }
    }
    glDisable(GL_STENCIL_TEST);

    GL_CHECK_ERROR

    glBindVertexArray(0);
    glUseProgram(0);
    GL_CHECK_ERROR
}


void TriangleRenderer::push()
{
    GL_CHECK_ERROR
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    n_tris = 0;
    for (const auto &it : triangles) {
        n_tris += it.second.size();
    }
    glBufferData(GL_ARRAY_BUFFER, sizeof(Triangle) * n_tris, nullptr, GL_STREAM_DRAW);
    GL_CHECK_ERROR
    size_t ofs = 0;
    layer_offsets.clear();
    std::vector<unsigned int> elements;
    for (const auto &it : triangles) {
        const auto &tris = it.second;
        glBufferSubData(GL_ARRAY_BUFFER, ofs * sizeof(Triangle), tris.size() * sizeof(Triangle), tris.data());
        std::map<Type, std::vector<unsigned int>> type_indices;
        unsigned int i = 0;
        for (const auto &tri : tris) {
            auto ty = Type::LINE;
            if (!isnan(tri.y2)) {
                ty = Type::TRIANGLE;
            }
            else if (isnan(tri.y2) && tri.x2 == 0) {
                ty = Type::LINE0;
            }
            else if (isnan(tri.y2) && (tri.flags & Triangle::FLAG_BUTT)) {
                ty = Type::LINE_BUTT;
            }
            else if (isnan(tri.y2)) {
                ty = Type::LINE;
            }
            else {
                throw std::runtime_error("unknown triangle type");
            }
            type_indices[ty].push_back(i + ofs);
            i++;
        }
        for (const auto &it2 : type_indices) {
            auto el_offset = elements.size();
            elements.insert(elements.end(), it2.second.begin(), it2.second.end());
            layer_offsets[it.first][it2.first] = std::make_pair(el_offset, it2.second.size());
        }
        ofs += tris.size();
    }
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    GL_CHECK_ERROR
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * elements.size(), elements.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    GL_CHECK_ERROR
}
} // namespace horizon
