#include "export_3d_image.hpp"
#include "canvas3d/canvas3d_base.hpp"
#include "GL/osmesa.h"
#include <cairomm/cairomm.h>

namespace horizon {

Image3DExporter::Image3DExporter(const class Board &abrd, class IPool &apool, unsigned int awidth, unsigned int aheight)
    : pool(apool)
{
    width = awidth;
    height = aheight;

    {
        std::vector<int> attribs;
        attribs.push_back(OSMESA_DEPTH_BITS);
        attribs.push_back(16);

        attribs.push_back(OSMESA_PROFILE);
        attribs.push_back(OSMESA_CORE_PROFILE);

        attribs.push_back(OSMESA_CONTEXT_MAJOR_VERSION);
        attribs.push_back(3);

        attribs.push_back(0);
        attribs.push_back(0);
        ctx = OSMesaCreateContextAttribs(attribs.data(), NULL);
    }
    if (!ctx) {
        throw std::runtime_error("couldn't create context");
    }
    buffer.resize(width * height * 4);
    if (!OSMesaMakeCurrent(static_cast<OSMesaContext>(ctx), buffer.data(), GL_UNSIGNED_BYTE, width, height)) {
        throw std::runtime_error("couldn't make current");
    }
    a_realize();

    brd = &abrd;
    ca.update(*brd);
    prepare();
    push();
}

void Image3DExporter::check_ctx()
{
    if (ctx != OSMesaGetCurrentContext()) {
        throw std::runtime_error("lost context");
    }
}

void Image3DExporter::load_3d_models()
{
    check_ctx();
    clear_3d_models();
    auto model_filenames = get_model_filenames(pool);
    for (const auto &it : model_filenames) {
        std::cout << "load " << it.first << std::endl;
        load_3d_model(it.first, it.second);
    }
    update_max_package_height();
    prepare_packages();
    face_renderer.push();
}


Cairo::RefPtr<Cairo::Surface> Image3DExporter::render_to_surface()
{
    check_ctx();
    render(render_background ? RenderBackground::YES : RenderBackground::NO);
    glFinish();

    auto buf = buffer.data();
    unsigned int iwidth = width;
    unsigned int iheight = height;
    auto surf = Cairo::ImageSurface::create(Cairo::FORMAT_ARGB32, iwidth, iheight);
    unsigned char *data = surf->get_data();
    for (size_t y = 0; y < iheight; y++) {
        auto offset = (iheight - y - 1) * surf->get_stride();
        for (size_t x = 0; x < iwidth; x++) {
            auto p = &data[x * 4 + offset];
            auto pb = &buf[(y * iwidth + x) * 4];
            p[0] = pb[2];
            p[1] = pb[1];
            p[2] = pb[0];
            p[3] = pb[3];
        }
    }
    surf->mark_dirty();
    return surf;
}

Image3DExporter::~Image3DExporter()
{
    OSMesaDestroyContext(static_cast<OSMesaContext>(ctx));
}
} // namespace horizon
