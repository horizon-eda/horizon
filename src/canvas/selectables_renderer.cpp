#include "selectables_renderer.hpp"
#include "selectables.hpp"
#include "gl_util.hpp"
#include <glm/gtc/type_ptr.hpp>
#include "canvas_gl.hpp"

namespace horizon {

static GLuint create_vao(GLuint program, GLuint &vbo_out)
{
    GLuint origin_index = glGetAttribLocation(program, "origin");
    GLuint box_center_index = glGetAttribLocation(program, "box_center");
    GLuint box_dim_index = glGetAttribLocation(program, "box_dim");
    GLuint angle_index = glGetAttribLocation(program, "angle");
    GLuint flags_index = glGetAttribLocation(program, "flags");
    GLuint vao, buffer;

    /* we need to create a VAO to store the other buffers */
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    /* this is the VBO that holds the vertex data */
    glGenBuffers(1, &buffer);
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    // data is buffered lateron

    GLfloat vertices[] = {
            //   Position
            7500000, 5000000, 2500000, 3000000, 10000000, 12500000,
            7500000, 2500000, 2500000, 3000000, 10000000, 12500000,
    };
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    /* enable and set the position attribute */
    glEnableVertexAttribArray(origin_index);
    glVertexAttribPointer(origin_index, 2, GL_FLOAT, GL_FALSE, sizeof(Selectable), (void *)offsetof(Selectable, x));
    glEnableVertexAttribArray(box_center_index);
    glVertexAttribPointer(box_center_index, 2, GL_FLOAT, GL_FALSE, sizeof(Selectable),
                          (void *)offsetof(Selectable, c_x));
    glEnableVertexAttribArray(box_dim_index);
    glVertexAttribPointer(box_dim_index, 2, GL_FLOAT, GL_FALSE, sizeof(Selectable),
                          (void *)offsetof(Selectable, width));
    glEnableVertexAttribArray(angle_index);
    glVertexAttribPointer(angle_index, 1, GL_FLOAT, GL_FALSE, sizeof(Selectable), (void *)offsetof(Selectable, angle));
    glEnableVertexAttribArray(flags_index);
    glVertexAttribIPointer(flags_index, 1, GL_UNSIGNED_BYTE, sizeof(Selectable), (void *)offsetof(Selectable, flags));
    /* enable and set the color attribute */
    /* reset the state; we will re-enable the VAO when needed */
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // glDeleteBuffers (1, &buffer);
    vbo_out = buffer;
    return vao;
}

SelectablesRenderer::SelectablesRenderer(CanvasGL *c, Selectables *s) : ca(c), sel(s)
{
}

void SelectablesRenderer::realize()
{
    program = gl_create_program_from_resource(
            "/org/horizon-eda/horizon/canvas/shaders/"
            "selectable-vertex.glsl",
            "/org/horizon-eda/horizon/canvas/shaders/"
            "selectable-fragment.glsl",
            "/org/horizon-eda/horizon/canvas/shaders/"
            "selectable-geometry.glsl");
    GL_CHECK_ERROR
    vao = create_vao(program, vbo);
    GL_CHECK_ERROR
    GET_LOC(this, screenmat);
    GET_LOC(this, viewmat);
    GET_LOC(this, scale);
    GET_LOC(this, color_always);
    GET_LOC(this, color_inner);
    GET_LOC(this, color_outer);
    GET_LOC(this, color_prelight);
    GL_CHECK_ERROR
}

void SelectablesRenderer::render()
{
    glUseProgram(program);
    glBindVertexArray(vao);
    glUniformMatrix3fv(screenmat_loc, 1, GL_FALSE, glm::value_ptr(ca->screenmat));
    glUniformMatrix3fv(viewmat_loc, 1, GL_FALSE, glm::value_ptr(ca->viewmat));
    glUniform1f(scale_loc, ca->scale);
    gl_color_to_uniform_3f(color_always_loc, ca->get_color(ColorP::SELECTABLE_ALWAYS));
    gl_color_to_uniform_3f(color_inner_loc, ca->get_color(ColorP::SELECTABLE_INNER));
    gl_color_to_uniform_3f(color_outer_loc, ca->get_color(ColorP::SELECTABLE_OUTER));
    gl_color_to_uniform_3f(color_prelight_loc, ca->get_color(ColorP::SELECTABLE_PRELIGHT));

    glDrawArrays(GL_POINTS, 0, sel->items.size());

    glBindVertexArray(0);
    glUseProgram(0);
}

void SelectablesRenderer::push()
{
    /*std::cout << "---" << std::endl;
    for(const auto it: items) {
            std::cout << it.x << " "<< it.y << " " << (int)it.flags <<
    std::endl;
    }
    std::cout << "---" << std::endl;*/
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Selectable) * sel->items.size(), sel->items.data(), GL_STREAM_DRAW);
}
} // namespace horizon
