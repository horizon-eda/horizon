#pragma once
#include "common/common.hpp"
#include "util/uuid.hpp"
#include "color_palette.hpp"
#include "util/gl_inc.h"
#include <memory>
#include "util/picture_data.hpp"

namespace horizon {

class PictureRenderer {
    friend class CanvasGL;

public:
    PictureRenderer(class CanvasGL &c);
    void realize();
    void render(bool on_top);
    void push();

private:
    CanvasGL &ca;

    GLuint program;
    GLuint vao;
    GLuint vbo;

    GLuint screenmat_loc;
    GLuint viewmat_loc;
    GLuint scale_loc;
    GLuint size_loc;
    GLuint shift_loc;
    GLuint angle_loc;
    GLuint tex_loc;
    GLuint opacity_loc;

    std::map<UUID, std::pair<std::shared_ptr<const PictureData>, GLuint>> textures;
    void cache_picture(std::shared_ptr<const PictureData> d);
    void uncache_picture(const UUID &uu);
};
} // namespace horizon
