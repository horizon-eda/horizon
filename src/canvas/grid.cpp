#include "grid.hpp"
#include "canvas_gl.hpp"
#include "gl_util.hpp"
#include <glm/gtc/type_ptr.hpp>

namespace horizon {
Grid::Grid(class CanvasGL *c) : ca(c), spacing(1.25_mm, 1.25_mm), mark_size(5)
{
}

static GLuint create_vao(GLuint program)
{
    GLuint position_index = glGetAttribLocation(program, "position");
    GLuint vao, buffer;

    /* we need to create a VAO to store the other buffers */
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    /* this is the VBO that holds the vertex data */
    glGenBuffers(1, &buffer);
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    // data is buffered lateron

    static const GLfloat vertices[] = {
            0, -1, 0, 1, -1, 0, 1, 0, .2, -.2, -.2, -.2, -.2, -.2, -.2, .2, -.2, .2, .2, .2, .2, .2, .2, -.2,
    };
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    /* enable and set the position attribute */
    glEnableVertexAttribArray(position_index);
    glVertexAttribPointer(position_index, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), 0);

    /* enable and set the color attribute */
    /* reset the state; we will re-enable the VAO when needed */
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    glDeleteBuffers(1, &buffer);
    return vao;
}

void Grid::realize()
{
    program = gl_create_program_from_resource("/org/horizon-eda/horizon/canvas/shaders/grid-vertex.glsl",
                                              "/org/horizon-eda/horizon/canvas/shaders/grid-fragment.glsl", nullptr);
    vao = create_vao(program);

    GET_LOC(this, screenmat);
    GET_LOC(this, viewmat);
    GET_LOC(this, grid_size);
    GET_LOC(this, grid_0);
    GET_LOC(this, grid_mod);
    GET_LOC(this, mark_size);
    GET_LOC(this, color);
}

void Grid::render()
{
    glUseProgram(program);
    glBindVertexArray(vao);
    glUniformMatrix3fv(screenmat_loc, 1, GL_FALSE, glm::value_ptr(ca->screenmat));
    glUniformMatrix3fv(viewmat_loc, 1, GL_FALSE, glm::value_ptr(ca->viewmat_noflip));
    glUniform1f(mark_size_loc, mark_size);
    auto color = ca->get_color(ColorP::GRID);
    glUniform4f(color_loc, color.r, color.g, color.b, ca->appearance.grid_opacity);

    Coordf sp = spacing;
    Coordf sp_px = sp * ca->scale;
    unsigned int newmul = 1;
    while (sp_px.x < 20 || sp_px.y < 20) {
        newmul *= 2;
        sp = spacing * newmul;
        sp_px = sp * ca->scale;
    }

    Coord<float> grid_0;
    grid_0.x = (round(((-ca->offset.x / ca->scale) - origin.x) / sp.x) - 1) * sp.x + origin.x;
    grid_0.y = (round(((-(ca->m_height - ca->offset.y) / ca->scale) - origin.y) / sp.y) - 1) * sp.y + origin.y;

    if (mul != newmul) {
        mul = newmul;
        ca->s_signal_grid_mul_changed.emit(mul);
    }

    glUniform2f(grid_size_loc, sp.x, sp.y);
    glUniform2f(grid_0_loc, grid_0.x, grid_0.y);

    auto spmin = std::min(sp.x, sp.y);
    glLineWidth(1 * ca->get_scale_factor());
    if (mark_size > 100) {
        glUniform1f(mark_size_loc, ca->m_height * 2);
        int n = (ca->m_width / ca->scale) / spmin + 4;
        glUniform1i(grid_mod_loc, n + 1);
        glDrawArraysInstanced(GL_LINES, 0, 2, n);

        glUniform1f(mark_size_loc, ca->m_width * 2);
        n = (ca->m_height / ca->scale) / spmin + 4;
        glUniform1i(grid_mod_loc, 1);
        glDrawArraysInstanced(GL_LINES, 2, 2, n);
    }
    else {
        int mod = (ca->m_width / ca->scale) / spmin + 4;
        glUniform1i(grid_mod_loc, mod);
        int n = mod * ((ca->m_height / ca->scale) / spmin + 4);
        glDrawArraysInstanced(GL_LINES, 0, 4, n);
    }

    // draw origin
    grid_0.x = 0;
    grid_0.y = 0;

    glUniform2f(grid_size_loc, 0, 0);
    glUniform2f(grid_0_loc, grid_0.x, grid_0.y);
    glUniform1i(grid_mod_loc, 1);
    glUniform1f(mark_size_loc, 15);
    auto origin_color = ca->get_color(ColorP::ORIGIN);
    gl_color_to_uniform_4f(color_loc, origin_color);

    glLineWidth(1 * ca->get_scale_factor());
    glDrawArraysInstanced(GL_LINES, 0, 4, 1);

    glBindVertexArray(0);
    glUseProgram(0);
}

void Grid::render_cursor(Coord<int64_t> &coord)
{
    glUseProgram(program);
    glBindVertexArray(vao);
    glUniformMatrix3fv(screenmat_loc, 1, GL_FALSE, glm::value_ptr(ca->screenmat));
    glUniformMatrix3fv(viewmat_loc, 1, GL_FALSE, glm::value_ptr(ca->viewmat));
    if (ca->cursor_size > 0)
        glUniform1f(mark_size_loc, ca->cursor_size);
    else
        glUniform1f(mark_size_loc, std::max(ca->m_width, ca->m_height));

    glUniform2f(grid_size_loc, 0, 0);
    glUniform2f(grid_0_loc, coord.x, coord.y);
    glUniform1i(grid_mod_loc, 1);

    auto bgcolor = ca->get_color(ColorP::BACKGROUND);
    glUniform4f(color_loc, bgcolor.r, bgcolor.g, bgcolor.b, 1);
    glLineWidth(4 * ca->get_scale_factor());
    glDrawArrays(GL_LINES, 0, 12);

    Color cursor_color;
    if (ca->target_current.is_valid()) {
        cursor_color = ca->get_color(ColorP::CURSOR_TARGET);
    }
    else {
        cursor_color = ca->get_color(ColorP::CURSOR_NORMAL);
    }
    glUniform4f(color_loc, cursor_color.r, cursor_color.g, cursor_color.b, 1);
    glLineWidth(1 * ca->get_scale_factor());
    glDrawArrays(GL_LINES, 0, 12);

    glBindVertexArray(0);
    glUseProgram(0);
}
} // namespace horizon
