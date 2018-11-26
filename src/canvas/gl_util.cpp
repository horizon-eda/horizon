#include "gl_util.hpp"
#include <giomm/resource.h>
#include <gtkmm.h>
#include <iostream>
#include "common/common.hpp"
#include "logger/logger.hpp"
#ifdef G_OS_WIN32
#include <windows.h>
#endif

namespace horizon {
/* Create and compile a shader */
static GLuint create_shader(int type, const char *src)
{
    auto shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    int status;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (status == GL_FALSE) {
        int log_len;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_len);

        std::string log_space(log_len + 1, ' ');
        glGetShaderInfoLog(shader, log_len, nullptr, (GLchar *)log_space.c_str());

        std::cerr << "Compile failure in " << (type == GL_VERTEX_SHADER ? "vertex" : "fragment")
                  << " shader: " << log_space << std::endl;

        glDeleteShader(shader);

        return 0;
    }

    return shader;
}

static GLuint create_shader_from_resource(int type, const char *resource)
{
    auto shader_bytes = Gio::Resource::lookup_data_global(resource);
    gsize shader_size{shader_bytes->get_size()};
    return create_shader(type, (const char *)shader_bytes->get_data(shader_size));
}

GLuint gl_create_program_from_resource(const char *vertex_resource, const char *fragment_resource,
                                       const char *geometry_resource)
{
    GLuint vertex, fragment, geometry = 0;
    GLuint program = 0;
    int status;
    vertex = create_shader_from_resource(GL_VERTEX_SHADER, vertex_resource);

    if (vertex == 0) {
        return 0;
    }

    fragment = create_shader_from_resource(GL_FRAGMENT_SHADER, fragment_resource);

    if (fragment == 0) {
        glDeleteShader(vertex);
        return 0;
    }

    if (geometry_resource) {
        geometry = create_shader_from_resource(GL_GEOMETRY_SHADER, geometry_resource);
        if (geometry == 0) {
            glDeleteShader(vertex);
            glDeleteShader(fragment);
        }
    }

    program = glCreateProgram();
    glAttachShader(program, vertex);
    glAttachShader(program, fragment);
    if (geometry) {
        glAttachShader(program, geometry);
    }

    glLinkProgram(program);

    glGetProgramiv(program, GL_LINK_STATUS, &status);
    if (status == GL_FALSE) {
        int log_len;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &log_len);

        std::string log_space(log_len + 1, ' ');
        glGetProgramInfoLog(program, log_len, nullptr, (GLchar *)log_space.c_str());

        std::cerr << "Linking failure: " << log_space << std::endl;

        glDeleteProgram(program);
        program = 0;

        goto out;
    }

    glDetachShader(program, vertex);
    glDetachShader(program, fragment);
    if (geometry)
        glDetachShader(program, geometry);

out:
    glDeleteShader(vertex);
    glDeleteShader(fragment);
    if (geometry)
        glDeleteShader(geometry);

    return program;
}

void gl_show_error(const std::string &s)
{
#ifdef G_OS_WIN32
    MessageBox(NULL, (s + "\nProgram will abort").c_str(), "OpenGL Error", MB_ICONERROR);
#else
    std::cout << s << std::endl;
#endif
}

void gl_color_to_uniform_3f(GLuint loc, const Color &c)
{
    glUniform3f(loc, c.r, c.g, c.b);
}
void gl_color_to_uniform_4f(GLuint loc, const Color &c, float alpha)
{
    glUniform4f(loc, c.r, c.g, c.b, alpha);
}
GLint gl_clamp_samples(GLint samples_req)
{
    GLint color_samples;
    GLint depth_samples;
    glGetIntegerv(GL_MAX_COLOR_TEXTURE_SAMPLES, &color_samples);
    glGetIntegerv(GL_MAX_DEPTH_TEXTURE_SAMPLES, &depth_samples);
    GLint samples = std::min(samples_req, std::min(color_samples, depth_samples));
    if (samples != samples_req) {
        Logger::log_warning("unsupported MSAA", Logger::Domain::CANVAS,
                            "requested:" + std::to_string(samples_req) + " actual:" + std::to_string(samples));
    }
    return samples;
}


} // namespace horizon
