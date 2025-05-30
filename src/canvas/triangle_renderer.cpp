#include "triangle_renderer.hpp"
#include "canvas_gl.hpp"
#include "gl_util.hpp"
#include <glm/gtc/type_ptr.hpp>
#include "bitmap_font_util.hpp"
#include <range/v3/view.hpp>
#include "common/layer_provider.hpp"

namespace horizon {

#define CHECK_ENUM_VAL(v, i) static_assert(static_cast<int>(v) == (i))
CHECK_ENUM_VAL(ColorP::AIRWIRE, 13);
#undef CHECK_ENUM_VAL

static GLuint create_vao(GLuint program, GLuint &vbo_out, GLuint &ebo_out)
{
    GLuint p0_index = 0;
    GLuint p1_index = 1;
    GLuint p2_index = 2;
    GLuint color_index = 3;
    GLuint lod_index = 4;
    GLuint color2_index = 5;
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
    glEnableVertexAttribArray(color_index);
    glVertexAttribIPointer(color_index, 1, GL_UNSIGNED_BYTE, sizeof(Triangle), (void *)offsetof(Triangle, color));
    glEnableVertexAttribArray(lod_index);
    glVertexAttribIPointer(lod_index, 1, GL_UNSIGNED_BYTE, sizeof(Triangle), (void *)offsetof(Triangle, lod));
    glEnableVertexAttribArray(color2_index);
    glVertexAttribIPointer(color2_index, 1, GL_UNSIGNED_BYTE, sizeof(Triangle), (void *)offsetof(Triangle, color2));

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

TriangleRenderer::TriangleRenderer(const CanvasGL &c, const std::map<int, vector_pair<Triangle, TriangleInfo>> &tris)
    : ca(c), triangles(tris)
{
}


void TriangleRenderer::realize()
{

    glGenTextures(1, &texture_glyph);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture_glyph);
    bitmap_font::load_texture();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    program_triangle = gl_create_program_from_resource("/org/horizon-eda/horizon/canvas/shaders/triangle-vertex.glsl",
                                                       "/org/horizon-eda/horizon/canvas/shaders/"
                                                       "triangle-triangle-fragment.glsl",
                                                       "/org/horizon-eda/horizon/canvas/shaders/"
                                                       "triangle-triangle-geometry.glsl");
    program_line0 = gl_create_program_from_resource("/org/horizon-eda/horizon/canvas/shaders/triangle-vertex.glsl",
                                                    "/org/horizon-eda/horizon/canvas/shaders/"
                                                    "triangle-line0-fragment.glsl",
                                                    "/org/horizon-eda/horizon/canvas/shaders/"
                                                    "triangle-line0-geometry.glsl");
    program_line_butt = gl_create_program_from_resource("/org/horizon-eda/horizon/canvas/shaders/triangle-vertex.glsl",
                                                        "/org/horizon-eda/horizon/canvas/shaders/"
                                                        "triangle-line-butt-fragment.glsl",
                                                        "/org/horizon-eda/horizon/canvas/shaders/"
                                                        "triangle-line-butt-geometry.glsl");
    program_line = gl_create_program_from_resource("/org/horizon-eda/horizon/canvas/shaders/triangle-vertex.glsl",
                                                   "/org/horizon-eda/horizon/canvas/shaders/"
                                                   "triangle-line-fragment.glsl",
                                                   "/org/horizon-eda/horizon/canvas/shaders/"
                                                   "triangle-line-geometry.glsl");
    program_glyph = gl_create_program_from_resource("/org/horizon-eda/horizon/canvas/shaders/triangle-vertex.glsl",
                                                    "/org/horizon-eda/horizon/canvas/shaders/"
                                                    "triangle-glyph-fragment.glsl",
                                                    "/org/horizon-eda/horizon/canvas/shaders/"
                                                    "triangle-glyph-geometry.glsl");
    program_circle = gl_create_program_from_resource("/org/horizon-eda/horizon/canvas/shaders/triangle-vertex.glsl",
                                                     "/org/horizon-eda/horizon/canvas/shaders/"
                                                     "triangle-circle-fragment.glsl",
                                                     "/org/horizon-eda/horizon/canvas/shaders/"
                                                     "triangle-circle-geometry.glsl");
    program_arc0 = gl_create_program_from_resource("/org/horizon-eda/horizon/canvas/shaders/triangle-vertex.glsl",
                                                   "/org/horizon-eda/horizon/canvas/shaders/"
                                                   "triangle-arc0-fragment.glsl",
                                                   "/org/horizon-eda/horizon/canvas/shaders/"
                                                   "triangle-arc0-geometry.glsl");
    program_arc = gl_create_program_from_resource("/org/horizon-eda/horizon/canvas/shaders/triangle-vertex.glsl",
                                                  "/org/horizon-eda/horizon/canvas/shaders/"
                                                  "triangle-arc-fragment.glsl",
                                                  "/org/horizon-eda/horizon/canvas/shaders/"
                                                  "triangle-arc-geometry.glsl");
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
    glUniformBlockBinding(program_glyph, block_index, binding_point_index);
    glUniformBlockBinding(program_circle, block_index, binding_point_index);
    glUniformBlockBinding(program_arc, block_index, binding_point_index);
    glUniformBlockBinding(program_arc0, block_index, binding_point_index);
    GL_CHECK_ERROR;
    vao = create_vao(program_line0, vbo, ebo);
    GL_CHECK_ERROR;
}


class UBOBufferTriangle {
public:
    static constexpr size_t ubo_size = 23; //  keep in sync with ubo.glsl
    static_assert(static_cast<int>(ColorP::N_COLORS) == ubo_size, "ubo size mismatch");
    std::array<std::array<float, 4>, ubo_size> colors;
    std::array<std::array<float, 4>, 256> colors2; // keep in sync with shader
    std::array<float, 12> screenmat;
    std::array<float, 12> viewmat;
    float alpha;
    float scale;
    std::array<float, 2> offset;
    float min_line_width;
    unsigned int layer_mode;
    unsigned int stencil_mode;
};

static std::array<float, 4> operator+(const std::array<float, 4> &a, float b)
{
    return {a.at(0) + b, a.at(1) + b, a.at(2) + b, a.at(3)};
}

static std::array<float, 4> operator+(const std::array<float, 4> &a, const std::array<float, 4> &b)
{
    return {a.at(0) + b.at(0), a.at(1) + b.at(1), a.at(2) + b.at(2), a.at(3)};
}

static std::array<float, 4> operator*(const std::array<float, 4> &a, float b)
{
    return {a.at(0) * b, a.at(1) * b, a.at(2) * b, a.at(3)};
}

std::array<float, 4> TriangleRenderer::apply_highlight(const Color &icolor, HighlightMode mode, int layer) const
{
    const std::array<float, 4> color = gl_array_from_color(icolor);
    if (layer >= 20000 && layer < 30000) { // is annotation
        if (!ca.annotations.at(layer).use_highlight)
            return color;
    }
    if (ca.layer_mode == CanvasGL::LayerMode::SHADOW_OTHER) {
        if (layer == ca.work_layer || ca.is_overlay_layer(layer, ca.work_layer)) {
            // it's okay continue as usual
        }
        else {
            if (mode == HighlightMode::ONLY)
                return color;
            else
                return gl_array_from_color(ca.appearance.colors.at(ColorP::SHADOW));
        }
    }
    if (!ca.highlight_enabled)
        return color;
    switch (ca.highlight_mode) {
    case CanvasGL::HighlightMode::HIGHLIGHT:
        if (mode == HighlightMode::ONLY)
            return color + ca.appearance.highlight_lighten;
        else
            return color;

    case CanvasGL::HighlightMode::DIM:
        if (mode == HighlightMode::ONLY)
            return color;
        else
            return (color * ca.appearance.highlight_dim)
                   + (gl_array_from_color(ca.appearance.colors.at(ColorP::BACKGROUND))
                      * (1 - ca.appearance.highlight_dim));

    case CanvasGL::HighlightMode::SHADOW:
        if (mode == HighlightMode::ONLY)
            return color;
        else
            return gl_array_from_color(ca.appearance.colors.at(ColorP::SHADOW));
    }
    return color;
}

void TriangleRenderer::render_layer_batch(int layer, HighlightMode highlight_mode, bool ignore_flip, const Batch &batch,
                                          bool use_stencil, bool stencil_mode)
{
    const auto &ld = ca.get_layer_display(layer);
    UBOBufferTriangle buf;

    buf.alpha = ca.property_layer_opacity() / 100;
    gl_mat3_to_array(buf.screenmat, ca.screenmat);
    if (ignore_flip)
        gl_mat3_to_array(buf.viewmat, ca.viewmat_noflip);
    else
        gl_mat3_to_array(buf.viewmat, ca.viewmat);

    buf.layer_mode = static_cast<unsigned int>(ld.mode);
    buf.scale = ca.scale;
    buf.offset[0] = ca.offset.x;
    buf.offset[1] = ca.offset.y;
    buf.min_line_width = ca.appearance.min_line_width;
    buf.stencil_mode = stencil_mode;

    for (const auto &[key, span] : batch) {
        bool skip = false;
        switch (key.type) {
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
            glUseProgram(program_glyph);
            break;

        case Type::CIRCLE:
            glUseProgram(program_circle);
            break;

        case Type::ARC0:
            glUseProgram(program_arc0);
            break;

        case Type::ARC:
            glUseProgram(program_arc);
            break;

        case Type::N_TYPES:
            throw std::runtime_error("N_TYPES is not a type");
        }
        switch (highlight_mode) {
        case HighlightMode::ONLY: // only highlighted, skip not highlighted
            if (!key.highlight) {
                skip = true;
            }
            break;
        case HighlightMode::SKIP: // only not highlighted, skip highlighted
            if (key.highlight) {
                skip = true;
            }
            break;
        }
        if (!skip) {
            for (size_t i = 0; i < buf.colors.size(); i++) {
                auto k = static_cast<ColorP>(i);
                if (ca.appearance.colors.count(k))
                    buf.colors[i] = apply_highlight(ca.appearance.colors.at(k), highlight_mode, layer);
            }
            auto lc = ca.get_layer_color(ca.layer_provider.get_color_layer(layer));
            buf.colors[static_cast<int>(ColorP::AIRWIRE_ROUTER)] =
                    gl_array_from_color(ca.appearance.colors.at(ColorP::AIRWIRE_ROUTER));
            buf.colors[static_cast<int>(ColorP::AIRWIRE)] =
                    gl_array_from_color(ca.appearance.colors.at(ColorP::AIRWIRE));
            buf.colors[static_cast<int>(ColorP::FROM_LAYER)] = apply_highlight(lc, highlight_mode, layer);
            buf.colors[static_cast<int>(ColorP::LAYER_HIGHLIGHT)] = apply_highlight(lc, highlight_mode, layer);
            buf.colors[static_cast<int>(ColorP::LAYER_HIGHLIGHT_LIGHTEN)] =
                    gl_array_from_color(lc) + ca.appearance.highlight_lighten;
            for (size_t i = 0; i < std::min(buf.colors2.size(), ca.colors2.size()); i++) {
                buf.colors2[i] = apply_highlight(ca.colors2[i].to_color(), highlight_mode, layer);
            }

            if (ld.mode == LayerDisplay::Mode::FILL_ONLY || (key.stencil && use_stencil))
                glStencilFunc(GL_GREATER, stencil, 0xff);
            else
                glStencilFunc(GL_ALWAYS, stencil, 0xff);


            glBindBuffer(GL_UNIFORM_BUFFER, ubo);
            glBufferData(GL_UNIFORM_BUFFER, sizeof(buf), &buf, GL_DYNAMIC_DRAW);
            glBindBuffer(GL_UNIFORM_BUFFER, 0);
            GL_CHECK_ERROR
            glDrawElements(GL_POINTS, span.count, GL_UNSIGNED_INT, (void *)(span.offset * sizeof(unsigned int)));
        }
    }
}

void TriangleRenderer::render_layer(int layer, HighlightMode highlight_mode, bool ignore_flip)
{
    GL_CHECK_ERROR
    if (layer_offsets.count(layer)) {
        const auto &ld = ca.get_layer_display(layer);

        const auto &batches = layer_offsets.at(layer);
        Batch batch_stencil, batch_no_stencil;
        for (const auto &[key, span] : batches) {
            if (key.stencil)
                batch_stencil.emplace_back(key, span);
            else
                batch_no_stencil.emplace_back(key, span);
        }
        if (ld.mode == LayerDisplay::Mode::FILL_ONLY) {
            render_layer_batch(layer, highlight_mode, ignore_flip, batch_stencil, false, false);
            render_layer_batch(layer, highlight_mode, ignore_flip, batch_no_stencil, false, false);
        }
        else {
            // draw stencil first
            render_layer_batch(layer, highlight_mode, ignore_flip, batch_stencil, true, true);
            render_layer_batch(layer, highlight_mode, ignore_flip, batch_stencil, true, false);

            render_layer_batch(layer, highlight_mode, ignore_flip, batch_no_stencil, false, false);
        }
    }
    // glDrawArrays(GL_POINTS, layer_offsets[layer], triangles[layer].size());
    stencil++;
}

void TriangleRenderer::render()
{
    glBindVertexArray(vao);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glActiveTexture(GL_TEXTURE0);

    GL_CHECK_ERROR

    std::vector<int> layers;
    layers.reserve(layer_offsets.size());
    for (const auto &it : layer_offsets) {
        if (ca.get_layer_display(it.first).visible)
            layers.push_back(it.first);
    }

    std::sort(layers.begin(), layers.end(), [this](auto la, auto lb) {
        return ca.layer_provider.get_layer_position(la) < ca.layer_provider.get_layer_position(lb);
    });

    if (ca.work_layer < 0) {
        std::reverse(layers.begin(), layers.end());
    }

    glClear(GL_STENCIL_BUFFER_BIT);
    glStencilOp(GL_KEEP, GL_REPLACE, GL_REPLACE);
    glEnable(GL_STENCIL_TEST);
    stencil = 1;

    static const std::vector<std::vector<HighlightMode>> modes_on_top = {{HighlightMode::SKIP}, {HighlightMode::ONLY}};
    static const std::vector<std::vector<HighlightMode>> modes_normal = {{HighlightMode::SKIP, HighlightMode::ONLY}};

    const auto &modes = ca.highlight_on_top ? modes_on_top : modes_normal;

    std::vector<std::pair<int, std::set<std::pair<int, bool>>>> normal_layers;
    normal_layers.reserve(layers.size());
    for (const auto layer : layers) {
        const auto &ld = ca.get_layer_display(layer);
        if (layer != ca.work_layer && layer < 10000 && ld.visible && !ca.layer_is_annotation(layer))
            normal_layers.push_back({layer, {}});
    }
    normal_layers.push_back({ca.work_layer, {}});

    for (const auto &[k, overlay_layer] : ca.overlay_layers) {
        const auto layer = k.first;
        const auto ignore_flip = k.second;
        auto f = std::find_if(normal_layers.rbegin(), normal_layers.rend(),
                              [layer](const auto &x) { return layer.overlaps(x.first); });
        if (f != normal_layers.rend()) {
            f->second.emplace(overlay_layer, ignore_flip);
        }
    }

    render_annotations(false); // annotation bottom
    for (const auto &highlight_modes : modes) {
        for (const auto &[layer, overlays] : normal_layers) {
            for (const auto highlight_mode : highlight_modes) {
                if (layer != ca.work_layer && ca.layer_mode == CanvasGL::LayerMode::WORK_ONLY
                    && highlight_mode == HighlightMode::SKIP)
                    continue;

                render_layer(layer, highlight_mode);
                for (const auto &[overlay, ignore_flip] : overlays) {
                    render_layer(overlay, highlight_mode, ignore_flip);
                }
            }
        }
        for (auto layer : layers) {
            const auto &ld = ca.get_layer_display(layer);
            if (layer >= 10000 && layer < Canvas::first_overlay_layer && ld.visible && !ca.layer_is_annotation(layer)) {
                for (const auto highlight_mode : highlight_modes) {
                    render_layer(layer, highlight_mode);
                }
            }
        }
    }
    render_annotations(true); // anotations top
    glDisable(GL_STENCIL_TEST);

    GL_CHECK_ERROR

    glBindVertexArray(0);
    glUseProgram(0);
    GL_CHECK_ERROR
}

void TriangleRenderer::render_annotations(bool top)
{
    for (const auto &it : ca.annotations) {
        if (ca.get_layer_display(it.first).visible && it.second.on_top == top) {
            render_layer(it.first, HighlightMode::SKIP);
            render_layer(it.first, HighlightMode::ONLY);
        }
    }
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
    for (const auto &[layer, tris] : triangles) {
        const auto &ld = ca.get_layer_display(layer);
        glBufferSubData(GL_ARRAY_BUFFER, ofs * sizeof(Triangle), tris.size() * sizeof(Triangle), tris.first.data());
        for (auto &x : type_indices) {
            x.clear();
        }
        unsigned int i = 0;
        for (const auto &[tri, tri_info] : tris) {
            const bool hidden = tri_info.flags & TriangleInfo::FLAG_HIDDEN;
            const bool type_visible = ld.types_visible & (1 << static_cast<int>(tri_info.type));
            if (!hidden && type_visible) {
                auto ty = Type::LINE;
                if (tri_info.flags & TriangleInfo::FLAG_GLYPH) {
                    ty = Type::GLYPH;
                }
                else if ((tri_info.flags & TriangleInfo::FLAG_ARC) && tri.y2 == 0) {
                    ty = Type::ARC0;
                }
                else if (tri_info.flags & TriangleInfo::FLAG_ARC) {
                    ty = Type::ARC;
                }
                else if (tri_info.flags & TriangleInfo::FLAG_BUTT) {
                    ty = Type::LINE_BUTT;
                }
                else if (!isnan(tri.y2)) {
                    ty = Type::TRIANGLE;
                }
                else if (isnan(tri.y2) && tri.x2 == 0) {
                    ty = Type::LINE0;
                }
                else if (isnan(tri.y1) && isnan(tri.x2) && isnan(tri.y2)) {
                    ty = Type::CIRCLE;
                }

                else if (isnan(tri.y2)) {
                    ty = Type::LINE;
                }
                else {
                    throw std::runtime_error("unknown triangle type");
                }
                const bool highlight = (tri_info.flags & TriangleInfo::FLAG_HIGHLIGHT)
                                       || (tri.color == static_cast<int>(ColorP::LAYER_HIGHLIGHT))
                                       || (tri.color == static_cast<int>(ColorP::LAYER_HIGHLIGHT_LIGHTEN));
                const bool do_stencil = tri_info.type == TriangleInfo::Type::PAD;
                const BatchKey key{ty, highlight, do_stencil};
                type_indices.at(key.hash()).push_back(i + ofs);
            }
            i++;
        }
        for (const auto [i, elems] : type_indices | ranges::views::enumerate) {
            if (elems.size() == 0)
                continue;
            const auto key = BatchKey::unhash(i);
            auto el_offset = elements.size();
            elements.insert(elements.end(), elems.begin(), elems.end());
            layer_offsets[layer][key] = {el_offset, elems.size()};
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
