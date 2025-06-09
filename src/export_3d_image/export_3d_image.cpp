#include "export_3d_image.hpp"
#include "canvas3d/canvas3d_base.hpp"
#include <cairomm/cairomm.h>
#include <epoxy/egl.h>

namespace horizon {

Image3DExporter::Image3DExporter(const class Board &abrd, class IPool &apool, unsigned int awidth, unsigned int aheight)
    : pool(apool)
{
    width = awidth;
    height = aheight;

    // based on
    // https://code.videolan.org/videolan/libplacebo/-/blob/00a1009a78434bbc43a1efc54f5915dd466706a4/src/tests/opengl_surfaceless.c#L98
    const char *extstr = eglQueryString(EGL_NO_DISPLAY, EGL_EXTENSIONS);
    if (!extstr || !strstr(extstr, "EGL_MESA_platform_surfaceless"))
        throw std::runtime_error("no EGL_MESA_platform_surfaceless");

    dpy = eglGetPlatformDisplayEXT(EGL_PLATFORM_SURFACELESS_MESA, (void *)EGL_DEFAULT_DISPLAY, NULL);
    if (!dpy)
        throw std::runtime_error("no dpy");

    EGLint major, minor;
    if (!eglInitialize(dpy, &major, &minor))
        throw std::runtime_error("error in eglInitialize");

    // clang-format off
    const int cfg_attribs[] = {
            EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
            EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT,
            EGL_ALPHA_SIZE, 8,
            EGL_NONE
    };
    // clang-format on

    EGLConfig config = 0;
    EGLint num_configs = 0;
    bool ok = eglChooseConfig(dpy, cfg_attribs, &config, 1, &num_configs);
    if (!ok || !num_configs)
        throw std::runtime_error("error in eglChooseConfig");

    if (!eglBindAPI(EGL_OPENGL_API))
        throw std::runtime_error("error in eglBindAPI");

    // clang-format off
    const int egl_attribs[] = {
        EGL_CONTEXT_MAJOR_VERSION, 4,
        EGL_CONTEXT_MINOR_VERSION, 6,
        EGL_CONTEXT_OPENGL_PROFILE_MASK, EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT,
        EGL_NONE
    };
    // clang-format on

    egl = eglCreateContext(dpy, config, EGL_NO_CONTEXT, egl_attribs);
    if (!egl)
        throw std::runtime_error("no context");


    const EGLint pbuffer_attribs[] = {EGL_WIDTH, width, EGL_HEIGHT, height, EGL_NONE};

    surf = eglCreatePbufferSurface(dpy, config, pbuffer_attribs);

    if (!eglMakeCurrent(dpy, surf, surf, egl))
        throw std::runtime_error("error in eglMakeCurrent");

    a_realize();

    brd = &abrd;
    ca.update(*brd);
    prepare();
    push();
}

void Image3DExporter::check_ctx()
{
    if (egl != eglGetCurrentContext()) {
        throw std::runtime_error("lost context");
    }
}

void Image3DExporter::load_3d_models()
{
    check_ctx();
    clear_3d_models();
    auto model_filenames = get_model_filenames(pool);
    for (const auto &it : model_filenames) {
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

    glReadBuffer(GL_COLOR_ATTACHMENT0);
    std::vector<uint8_t> buf(width * height * 4);
    glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, buf.data());

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
    eglDestroySurface(dpy, surf);
    eglDestroyContext(dpy, egl);
    eglTerminate(dpy);
}
} // namespace horizon
