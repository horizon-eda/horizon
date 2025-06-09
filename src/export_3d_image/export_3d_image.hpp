#pragma once
#include <string>
#include <vector>
#include "canvas3d/canvas3d_base.hpp"
#include <cairomm/cairomm.h>

namespace horizon {

class Image3DExporter : public Canvas3DBase {
public:
    Image3DExporter(const class Board &brd, class IPool &pool, unsigned int width, unsigned int height);

    void load_3d_models();
    Cairo::RefPtr<Cairo::Surface> render_to_surface();
    virtual ~Image3DExporter();

    const bool &get_render_background() const
    {
        return render_background;
    }
    void set_render_background(const bool &v)
    {
        render_background = v;
    }

private:
    class IPool &pool;
    void *egl = nullptr;  // to get around including epoxy here
    void *dpy = nullptr;  // to get around including epoxy here
    void *surf = nullptr; // to get around including epoxy here
    void check_ctx();
    bool render_background = false;
};

} // namespace horizon
