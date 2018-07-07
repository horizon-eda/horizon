#include "triangle.hpp"
#include "canvas_gl.hpp"
#include "gl_util.hpp"
#include <glm/gtc/type_ptr.hpp>

namespace horizon {

static GLuint create_vao(GLuint program, GLuint &vbo_out)
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
    GLuint vao, buffer;

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

    return vao;
}

TriangleRenderer::TriangleRenderer(CanvasGL *c, std::unordered_map<int, std::vector<Triangle>> &tris)
    : ca(c), triangles(tris)
{
}

void TriangleRenderer::realize()
{
    program = gl_create_program_from_resource("/net/carrotIndustries/horizon/canvas/shaders/triangle-vertex.glsl",
                                              "/net/carrotIndustries/horizon/canvas/shaders/"
                                              "triangle-fragment.glsl",
                                              "/net/carrotIndustries/horizon/canvas/shaders/"
                                              "triangle-geometry.glsl");
    GL_CHECK_ERROR;
    glGenBuffers(1, &ubo);
    glBindBuffer(GL_UNIFORM_BUFFER, ubo);
    float testd[3] = {1, 1, 1};
    glBufferData(GL_UNIFORM_BUFFER, sizeof(testd), &testd, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    GL_CHECK_ERROR;
    unsigned int block_index = glGetUniformBlockIndex(program, "layer_setup");
    GLuint binding_point_index = 0;
    glBindBufferBase(GL_UNIFORM_BUFFER, binding_point_index, ubo);
    glUniformBlockBinding(program, block_index, binding_point_index);
    GL_CHECK_ERROR;
    vao = create_vao(program, vbo);
    GET_LOC(this, screenmat);
    GET_LOC(this, viewmat);
    GET_LOC(this, scale);
    GET_LOC(this, alpha);
    GET_LOC(this, layer_color);
    GET_LOC(this, layer_flags);
    GET_LOC(this, types_visible);
    GET_LOC(this, types_force_outline);
    GET_LOC(this, highlight_mode);
    GET_LOC(this, highlight_dim);
    GET_LOC(this, highlight_shadow);
    GET_LOC(this, highlight_lighten);
    GL_CHECK_ERROR;
}

void TriangleRenderer::render_layer(int layer)
{
    const auto &ld = ca->layer_display.at(layer);
    glUniform3f(layer_color_loc, ld.color.r, ld.color.g, ld.color.b);
    glUniform1i(layer_flags_loc, static_cast<int>(ld.mode));
    glUniform1ui(types_visible_loc, ld.types_visible);
    glUniform1ui(types_force_outline_loc, ld.types_force_outline);
    if (ld.mode == LayerDisplay::Mode::FILL_ONLY)
        glStencilFunc(GL_GREATER, stencil, 0xff);
    else
        glStencilFunc(GL_ALWAYS, stencil, 0xff);
    glDrawArrays(GL_POINTS, layer_offsets[layer], triangles[layer].size());
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
    GL_CHECK_ERROR
    glUseProgram(program);
    glBindVertexArray(vao);
    glUniformMatrix3fv(screenmat_loc, 1, GL_FALSE, glm::value_ptr(ca->screenmat));
    glUniformMatrix3fv(viewmat_loc, 1, GL_FALSE, glm::value_ptr(ca->viewmat));
    glUniform1f(scale_loc, ca->scale);
    glUniform1f(alpha_loc, ca->property_layer_opacity() / 100);
    glUniform1i(highlight_mode_loc, ca->highlight_enabled ? static_cast<int>(ca->highlight_mode) : 0);
    glUniform1f(highlight_dim_loc, ca->highlight_dim);
    glUniform1f(highlight_shadow_loc, ca->highlight_shadow);
    glUniform1f(highlight_lighten_loc, ca->highlight_lighten);

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
    glBindBuffer(GL_UNIFORM_BUFFER, ubo);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(ca->layer_setup), &ca->layer_setup, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
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
    for (const auto &it : triangles) {
        glBufferSubData(GL_ARRAY_BUFFER, ofs * sizeof(Triangle), it.second.size() * sizeof(Triangle), it.second.data());
        layer_offsets[it.first] = ofs;
        ofs += it.second.size();
    }
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    GL_CHECK_ERROR
}
} // namespace horizon
