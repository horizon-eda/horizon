#pragma once
#include <string>
#include <vector>
#include "canvas3d/canvas3d_base.hpp"
#include <cairomm/cairomm.h>

namespace horizon {

class Image3DExporter : public Canvas3DBase {
public:
    Image3DExporter(const class Board &brd, class Pool &pool, unsigned int width, unsigned int height);

    void load_3d_models();
    Cairo::RefPtr<Cairo::Surface> render_to_surface();
    virtual ~Image3DExporter();
    bool render_background = false;

private:
    class Pool &pool;
    void *ctx = nullptr; // to get around including osmesa here
    std::vector<unsigned char> buffer;
    void check_ctx();
};

} // namespace horizon
