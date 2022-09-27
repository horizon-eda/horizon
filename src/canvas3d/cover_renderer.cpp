#include "cover_renderer.hpp"
#include "board/board_layers.hpp"
#include "canvas/gl_util.hpp"
#include "canvas3d_base.hpp"
#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace horizon {
CoverRenderer::CoverRenderer(Canvas3DBase &c) : ca(c)
{
}

static GLuint create_vao(GLuint program, GLuint &vbo_out)
{
    GLuint position_index = glGetAttribLocation(program, "position");
    GLuint vao, buffer;

    /* we need to create a VAO to store the other buffers */
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    /* this is the VBO that holds the vertex data */
    glGenBuffers(1, &buffer);
    glBindBuffer(GL_ARRAY_BUFFER, buffer);

    CanvasMesh::Layer3D::Vertex vertices[] = {
            //   Position
            {-5, -5}, {5, -5}, {-5, 5},

            {5, 5},   {5, -5}, {-5, 5},
    };
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    /* enable and set the position attribute */
    glEnableVertexAttribArray(position_index);
    glVertexAttribPointer(position_index, 2, GL_FLOAT, GL_FALSE, sizeof(CanvasMesh::Layer3D::Vertex), 0);

    /* enable and set the color attribute */
    /* reset the state; we will re-enable the VAO when needed */
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // glDeleteBuffers (1, &buffer);
    vbo_out = buffer;

    return vao;
}

void CoverRenderer::realize()
{
    program = gl_create_program_from_resource("/org/horizon-eda/horizon/canvas3d/shaders/cover-vertex.glsl",
                                              "/org/horizon-eda/horizon/canvas3d/shaders/"
                                              "cover-fragment.glsl",
                                              nullptr);
    vao = create_vao(program, vbo);

    GET_LOC(this, view);
    GET_LOC(this, proj);
    GET_LOC(this, layer_offset);
    GET_LOC(this, layer_color);
    GET_LOC(this, cam_normal);
    GET_LOC(this, cam_pos);
    GET_LOC(this, ambient_intensity);
    GET_LOC(this, specular_intensity);
    GET_LOC(this, specular_power);
    GET_LOC(this, diffuse_intensity);
    GET_LOC(this, light_pos);
    GET_LOC(this, light_color);
}

void CoverRenderer::push()
{
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    n_vertices = 0;
    for (const auto &it : ca.get_layers()) {
        n_vertices += it.second.tris.size();
    }
    glBufferData(GL_ARRAY_BUFFER, sizeof(CanvasMesh::Layer3D::Vertex) * n_vertices, nullptr, GL_STREAM_DRAW);
    GL_CHECK_ERROR
    size_t ofs = 0;
    layer_offsets.clear();
    for (const auto &it : ca.get_layers()) {
        glBufferSubData(GL_ARRAY_BUFFER, ofs * sizeof(CanvasMesh::Layer3D::Vertex),
                        it.second.tris.size() * sizeof(CanvasMesh::Layer3D::Vertex), it.second.tris.data());
        layer_offsets[it.first] = ofs;
        ofs += it.second.tris.size();
    }
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void CoverRenderer::render(int layer)
{
    const bool is_opaque = ca.get_layer(layer).alpha == 1;
    if (!is_opaque)
        glEnable(GL_BLEND);
    auto co = ca.get_layer_color(layer);
    gl_color_to_uniform_4f(layer_color_loc, co, ca.get_layer(layer).alpha);
    glUniform1f(layer_offset_loc, ca.get_layer_offset(layer));
    glUniform1f(ambient_intensity_loc, ca.get_layer(layer).ambient_intensity);
    glUniform1f(specular_intensity_loc, ca.get_layer(layer).specular_intensity);
    glUniform1f(specular_power_loc, ca.get_layer(layer).specular_power);
    glUniform1f(diffuse_intensity_loc, ca.get_layer(layer).diffuse_intensity);
    glDrawArrays(GL_TRIANGLES, layer_offsets[layer], ca.get_layer(layer).tris.size());
    if (is_opaque) {
        glUniform1f(layer_offset_loc, ca.get_layer_offset(layer) + ca.get_layer_thickness(layer));
        glDrawArrays(GL_TRIANGLES, layer_offsets[layer], ca.get_layer(layer).tris.size());
    }
    glDisable(GL_BLEND);
}

void CoverRenderer::render()
{
    glm::vec3 light_color(ca.light_color.r, ca.light_color.g, ca.light_color.b);
    glUseProgram(program);
    glBindVertexArray(vao);

    glUniformMatrix4fv(view_loc, 1, GL_FALSE, glm::value_ptr(ca.viewmat));
    glUniformMatrix4fv(proj_loc, 1, GL_FALSE, glm::value_ptr(ca.projmat));
    glUniform3fv(cam_normal_loc, 1, glm::value_ptr(ca.cam_normal));
    glUniform3fv(cam_pos_loc, 1, glm::value_ptr(ca.cam_position));
    glUniform3fv(light_pos_loc, 1, glm::value_ptr(ca.light_position));
    glUniform3fv(light_color_loc, 1, glm::value_ptr(light_color));

    for (const auto &it : layer_offsets) {
        if (ca.get_layer(it.first).alpha == 1 && ca.layer_is_visible(it.first))
            render(it.first);
    }
    for (const auto &it : layer_offsets) {
        if (ca.get_layer(it.first).alpha != 1 && ca.layer_is_visible(it.first))
            render(it.first);
    }
}
} // namespace horizon
