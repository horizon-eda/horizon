#include "cursor_renderer.hpp"
#include "canvas_gl.hpp"
#include "gl_util.hpp"
#include "util/picture_util.hpp"
#include <glm/gtc/type_ptr.hpp>

namespace horizon {
static GLuint create_vao(GLuint program)
{
    GLuint vao, buffer;
    GLuint position_index = glGetAttribLocation(program, "position");


    /* we need to create a VAO to store the other buffers */
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    /* this is the VBO that holds the vertex data */
    glGenBuffers(1, &buffer);
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    // data is buffered lateron

    float vertices[] = {//   Position
                        -1, 1, 1, 1, -1, -1, 1, -1};
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    /* enable and set the position attribute */
    glEnableVertexAttribArray(position_index);
    glVertexAttribPointer(position_index, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, 0);

    /* enable and set the color attribute */
    /* reset the state; we will re-enable the VAO when needed */
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    return vao;
}

CursorRenderer::CursorRenderer(const CanvasGL &c) : ca(c)
{
}

void CursorRenderer::realize()
{
    program = gl_create_program_from_resource(
            "/org/horizon-eda/horizon/canvas/shaders/"
            "cursor-vertex.glsl",
            "/org/horizon-eda/horizon/canvas/shaders/"
            "cursor-fragment.glsl",
            nullptr);
    vao = create_vao(program);

    GET_LOC(this, screenmat);
    GET_LOC(this, size);
    GET_LOC(this, cursor_position);
    GET_LOC(this, tex);

}

void CursorRenderer::render()
{
    glUseProgram(program);
    glBindVertexArray(vao);
    glUniformMatrix3fv(screenmat_loc, 1, GL_FALSE, glm::value_ptr(ca.screenmat));
    glUniform2f(size_loc, 16, 16);


    glActiveTexture(GL_TEXTURE2);
    glUniform1i(tex_loc, 2);

    for (const auto x_offset : {0.f, ca.m_width, -ca.m_width}) {
        for (const auto y_offset : {0.f, ca.m_height, -ca.m_height}) {
            auto pos = ca.pan_cursor_pos + Coordf(x_offset, y_offset) - Coordf(8, 8);
            glUniform2f(cursor_position_loc, pos.x, pos.y);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        }
    }


    glBindVertexArray(0);
    glUseProgram(0);
}

void CursorRenderer::push()
{
}

} // namespace horizon
