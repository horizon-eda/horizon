#include "point_renderer.hpp"
#include "board/board_layers.hpp"
#include "canvas/gl_util.hpp"
#include "canvas3d_base.hpp"
#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace horizon {
PointRenderer::PointRenderer(Canvas3DBase &c) : ca(c)
{
}

GLuint PointRenderer::create_vao(GLuint program, GLuint &vbo_out)
{
    GLuint position_index = glGetAttribLocation(program, "position");
    GLuint vao, buffer;

    /* we need to create a VAO to store the other buffers */
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    /* this is the VBO that holds the vertex data */
    glGenBuffers(1, &buffer);
    glBindBuffer(GL_ARRAY_BUFFER, buffer);

    Point3D vertices[] = {//   Position
                          {0, 0, 0},
                          {0, 0, 10},
                          {10, 10, 10}};
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    /* enable and set the position attribute */
    glEnableVertexAttribArray(position_index);
    glVertexAttribPointer(position_index, 3, GL_DOUBLE, GL_FALSE, sizeof(Point3D), 0);

    /* enable and set the color attribute */
    /* reset the state; we will re-enable the VAO when needed */
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // glDeleteBuffers (1, &buffer);
    vbo_out = buffer;

    return vao;
}

void PointRenderer::realize()
{
    program = gl_create_program_from_resource("/org/horizon-eda/horizon/canvas3d/shaders/point-vertex.glsl",
                                              "/org/horizon-eda/horizon/canvas3d/shaders/"
                                              "point-fragment.glsl",
                                              nullptr);
    vao = create_vao(program, vbo);

    GET_LOC(this, view);
    GET_LOC(this, proj);
    GET_LOC(this, model);
    GET_LOC(this, z_offset);
    GET_LOC(this, pick_base);
}

void PointRenderer::push()
{
    if (ca.models_loading_mutex.try_lock()) {
        ca.n_points = ca.points.size();
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(Point3D) * ca.n_points, ca.points.data(), GL_STATIC_DRAW);

        ca.models_loading_mutex.unlock();
    }
}

void PointRenderer::render()
{
    if (!ca.n_points)
        return;
    glUseProgram(program);
    glBindVertexArray(vao);

    glUniformMatrix4fv(view_loc, 1, GL_FALSE, glm::value_ptr(ca.viewmat));
    glUniformMatrix4fv(proj_loc, 1, GL_FALSE, glm::value_ptr(ca.projmat));
    glm::mat4 model_mat = ca.point_mat;
    glUniformMatrix4fv(model_loc, 1, GL_FALSE, glm::value_ptr(model_mat));
    glUniform1ui(pick_base_loc, ca.point_pick_base);

    glUniform1f(z_offset_loc, ca.get_layer(BoardLayers::TOP_COPPER).offset + 5 * ca.explode
                                      + ca.get_layer(BoardLayers::TOP_COPPER).thickness);

    glPointSize(10);
    glDrawArrays(GL_POINTS, 0, ca.n_points);
}
} // namespace horizon
