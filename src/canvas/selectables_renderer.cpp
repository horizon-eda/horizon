#include "selectables_renderer.hpp"
#include "selectables.hpp"
#include "gl_util.hpp"
#include <glm/gtc/type_ptr.hpp>
#include "canvas_gl.hpp"

namespace horizon {

static GLuint create_vao(GLuint program, GLuint &vbo_out, GLuint &ebo_out)
{
    GLuint origin_index = glGetAttribLocation(program, "origin");
    GLuint box_center_index = glGetAttribLocation(program, "box_center");
    GLuint box_dim_index = glGetAttribLocation(program, "box_dim");
    GLuint angle_index = glGetAttribLocation(program, "angle");
    GLuint flags_index = glGetAttribLocation(program, "flags");
    GLuint vao, buffer;

    glGenBuffers(1, &ebo_out);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_out);

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

SelectablesRenderer::SelectablesRenderer(const CanvasGL &c, const Selectables &s) : ca(c), sel(s)
{
}

class UBOBufferSelectables {
public:
    std::array<float, 4> color_inner;
    std::array<float, 4> color_outer;
    std::array<float, 4> color_always;
    std::array<float, 4> color_prelight;
    std::array<float, 12> screenmat;
    std::array<float, 12> viewmat;
    float scale;
    float min_size;
};

void SelectablesRenderer::realize()
{
    program = gl_create_program_from_resource(
            "/org/horizon-eda/horizon/canvas/shaders/"
            "selectable-vertex.glsl",
            "/org/horizon-eda/horizon/canvas/shaders/"
            "selectable-fragment.glsl",
            "/org/horizon-eda/horizon/canvas/shaders/"
            "selectable-geometry.glsl");
    program_arc = gl_create_program_from_resource(
            "/org/horizon-eda/horizon/canvas/shaders/"
            "selectable-vertex.glsl",
            "/org/horizon-eda/horizon/canvas/shaders/"
            "selectable-arc-fragment.glsl",
            "/org/horizon-eda/horizon/canvas/shaders/"
            "selectable-arc-geometry.glsl");
    GL_CHECK_ERROR
    vao = create_vao(program, vbo, ebo);
    GL_CHECK_ERROR

    glGenBuffers(1, &ubo);
    unsigned int block_index = glGetUniformBlockIndex(program, "ubo");
    GLuint binding_point_index = 1;
    glBindBufferBase(GL_UNIFORM_BUFFER, binding_point_index, ubo);
    glUniformBlockBinding(program, block_index, binding_point_index);
    glUniformBlockBinding(program_arc, block_index, binding_point_index);
    GL_CHECK_ERROR
}

void SelectablesRenderer::render()
{
    UBOBufferSelectables buf;
    gl_mat3_to_array(buf.screenmat, ca.screenmat);
    gl_mat3_to_array(buf.viewmat, ca.viewmat);
    buf.scale = ca.scale;
    buf.min_size = ca.appearance.min_selectable_size;
    buf.color_always = gl_array_from_color(ca.get_color(ColorP::SELECTABLE_ALWAYS));
    buf.color_inner = gl_array_from_color(ca.get_color(ColorP::SELECTABLE_INNER));
    buf.color_outer = gl_array_from_color(ca.get_color(ColorP::SELECTABLE_OUTER));
    buf.color_prelight = gl_array_from_color(ca.get_color(ColorP::SELECTABLE_PRELIGHT));

    glBindVertexArray(vao);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBindBuffer(GL_UNIFORM_BUFFER, ubo);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(buf), &buf, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    if (n_arc) {
        glUseProgram(program_arc);

        // glDrawArrays(GL_POINTS, 0, n_arcs);
        glDrawElements(GL_POINTS, n_arc, GL_UNSIGNED_INT, (void *)(0 * sizeof(unsigned int)));
    }

    if (n_box) {
        glUseProgram(program);

        // glDrawArrays(GL_POINTS, 0, n_arcs);
        glDrawElements(GL_POINTS, n_box, GL_UNSIGNED_INT, (void *)(n_arc * sizeof(unsigned int)));
    }


    glBindVertexArray(0);
    glUseProgram(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
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
    glBufferData(GL_ARRAY_BUFFER, sizeof(Selectable) * sel.items.size(), sel.items.data(), GL_STREAM_DRAW);

    std::vector<unsigned int> elements;
    elements.reserve(sel.items.size());
    for (unsigned int i = 0; i < sel.items.size(); i++) {
        if (sel.items.at(i).is_arc()
            && (sel.items.at(i).flags & (~static_cast<int>(Selectable::Flag::ARC_CENTER_IS_MIDPOINT))) != 0) {
            elements.push_back(i);
        }
    }
    n_arc = elements.size();
    for (unsigned int i = 0; i < sel.items.size(); i++) {
        if (!sel.items.at(i).is_arc() && sel.items.at(i).flags != 0) {
            elements.push_back(i);
        }
    }
    n_box = elements.size() - n_arc;

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * elements.size(), elements.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}
} // namespace horizon
