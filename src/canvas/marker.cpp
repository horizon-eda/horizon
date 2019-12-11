#include "marker.hpp"
#include "canvas_gl.hpp"
#include "gl_util.hpp"
#include <algorithm>
#include <glm/gtc/type_ptr.hpp>

namespace horizon {

static GLuint create_vao(GLuint program, GLuint &vbo_out)
{
    auto err = glGetError();
    if (err != GL_NO_ERROR) {
        std::cout << "gl error a " << err << std::endl;
    }
    GLuint position_index = glGetAttribLocation(program, "position");
    GLuint color_index = glGetAttribLocation(program, "color");
    GLuint flags_index = glGetAttribLocation(program, "flags");

    GLuint vao, buffer;

    /* we need to create a VAO to store the other buffers */
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    /* this is the VBO that holds the vertex data */
    glGenBuffers(1, &buffer);
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    // data is buffered lateron

    GLfloat vertices[] = {//   Position
                          0, 0, 1, 0, 1, 0};
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    /* enable and set the position attribute */
    glEnableVertexAttribArray(position_index);
    glVertexAttribPointer(position_index, 2, GL_FLOAT, GL_FALSE, sizeof(Marker), (void *)offsetof(Marker, x));

    glEnableVertexAttribArray(color_index);
    glVertexAttribPointer(color_index, 3, GL_FLOAT, GL_FALSE, sizeof(Marker), (void *)offsetof(Marker, r));

    glEnableVertexAttribArray(flags_index);
    glVertexAttribIPointer(flags_index, 1, GL_UNSIGNED_BYTE, sizeof(Marker), (void *)offsetof(Marker, flags));

    /* enable and set the color attribute */
    /* reset the state; we will re-enable the VAO when needed */
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // glDeleteBuffers (1, &buffer);
    vbo_out = buffer;

    return vao;
}

Markers::Markers(CanvasGL *c) : ca(c)
{
    std::fill(domains_visible.begin(), domains_visible.end(), false);
}

void Markers::set_domain_visible(MarkerDomain dom, bool vis)
{
    domains_visible.at(static_cast<int>(dom)) = vis;
    ca->update_markers();
}

std::deque<MarkerRef> &Markers::get_domain(MarkerDomain dom)
{
    return domains.at(static_cast<int>(dom));
}

void Markers::update()
{
    ca->update_markers();
    ca->request_push(CanvasGL::PF_MARKER);
}

MarkerRenderer::MarkerRenderer(CanvasGL *c, Markers &ma) : ca(c), markers_ref(ma)
{
}

void MarkerRenderer::realize()
{
    program = gl_create_program_from_resource("/org/horizon-eda/horizon/canvas/shaders/marker-vertex.glsl",
                                              "/org/horizon-eda/horizon/canvas/shaders/marker-fragment.glsl",
                                              "/org/horizon-eda/horizon/canvas/shaders/"
                                              "marker-geometry.glsl");
    vao = create_vao(program, vbo);
    GET_LOC(this, screenmat);
    GET_LOC(this, viewmat);
    GET_LOC(this, alpha);
    GET_LOC(this, border_color);
}

void MarkerRenderer::render()
{
    glUseProgram(program);
    glBindVertexArray(vao);
    glUniformMatrix3fv(screenmat_loc, 1, GL_FALSE, glm::value_ptr(ca->screenmat));
    glUniformMatrix3fv(viewmat_loc, 1, GL_FALSE, glm::value_ptr(ca->viewmat));
    glUniform1f(alpha_loc, ca->property_layer_opacity() / 100);
    gl_color_to_uniform_3f(border_color_loc, ca->get_color(ColorP::MARKER_BORDER));
    glDrawArrays(GL_POINTS, 0, markers.size());

    glBindVertexArray(0);
    glUseProgram(0);
}

void MarkerRenderer::update()
{
    markers.clear();
    int i = 0;
    for (const auto &dom : markers_ref.domains) {
        if (markers_ref.domains_visible.at(i)) {
            for (const auto &mkr : dom) {
                if (mkr.sheet == ca->sheet_current_uuid || mkr.sheet == UUID()) {
                    uint8_t flags = 0;
                    if (mkr.size == MarkerRef::Size::SMALL)
                        flags = 1;
                    markers.emplace_back(mkr.position, mkr.color, flags);
                }
            }
        }

        i++;
    }
}

void MarkerRenderer::push()
{
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Marker) * markers.size(), markers.data(), GL_STREAM_DRAW);
}
} // namespace horizon
