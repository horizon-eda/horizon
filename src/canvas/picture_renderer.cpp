#include "picture_renderer.hpp"
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

PictureRenderer::PictureRenderer(CanvasGL &c) : ca(c)
{
}

void PictureRenderer::realize()
{
    program = gl_create_program_from_resource(
            "/org/horizon-eda/horizon/canvas/shaders/"
            "picture-vertex.glsl",
            "/org/horizon-eda/horizon/canvas/shaders/"
            "picture-fragment.glsl",
            nullptr);
    vao = create_vao(program);

    GET_LOC(this, screenmat);
    GET_LOC(this, viewmat);
    GET_LOC(this, scale);
    GET_LOC(this, size);
    GET_LOC(this, shift);
    GET_LOC(this, angle);
    GET_LOC(this, tex);
    GET_LOC(this, opacity);
}

void PictureRenderer::render(bool on_top)
{
    glUseProgram(program);
    glBindVertexArray(vao);
    glUniformMatrix3fv(screenmat_loc, 1, GL_FALSE, glm::value_ptr(ca.screenmat));
    glUniformMatrix3fv(viewmat_loc, 1, GL_FALSE, glm::value_ptr(ca.viewmat));
    glUniform1f(scale_loc, ca.scale);

    glActiveTexture(GL_TEXTURE1);
    glUniform1i(tex_loc, 1);

    for (const auto &it : ca.pictures) {
        if (it.on_top == on_top) {
            const auto &tex = textures.at(it.data->uuid);
            glBindTexture(GL_TEXTURE_2D, tex.second);
            glUniform2f(shift_loc, it.x, it.y);
            glUniform2f(size_loc, tex.first->width * it.px_size / 2, tex.first->height * it.px_size / 2);
            glUniform1f(angle_loc, it.angle);
            glUniform1f(opacity_loc, it.opacity);

            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        }
    }


    glBindVertexArray(0);
    glUseProgram(0);
}

void PictureRenderer::cache_picture(std::shared_ptr<const PictureData> d)
{
    if (textures.count(d->uuid))
        return;
    GLuint tex = 0;
    glGenTextures(1, &tex);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, d->width, d->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, d->data.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    textures.emplace(std::piecewise_construct, std::forward_as_tuple(d->uuid), std::forward_as_tuple(d, tex));
}

void PictureRenderer::uncache_picture(const UUID &uu)
{
    glDeleteTextures(1, &textures.at(uu).second);
    textures.erase(uu);
}

void PictureRenderer::push()
{
    std::set<UUID> pics_to_evict, pics_needed;
    for (const auto &it : ca.pictures) {
        if (!textures.count(it.data->uuid))
            cache_picture(it.data);
        pics_needed.insert(it.data->uuid);
    }
    for (const auto &it : textures) {
        if (!pics_needed.count(it.first))
            pics_to_evict.insert(it.first);
    }
    for (const auto &it : pics_to_evict) {
        uncache_picture(it);
    }
}

} // namespace horizon
