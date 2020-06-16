#pragma once
#include "clipper/clipper.hpp"
#include "common/common.hpp"
#include <epoxy/gl.h>
#include <gtkmm.h>

namespace horizon {
class DragSelection {
    friend class CanvasGL;
    friend class Box;

public:
    DragSelection(class CanvasGL &c);
    void realize();
    void render();
    void push();
    void drag_begin(GdkEventButton *button_event);
    void drag_end(GdkEventButton *button_event);
    void drag_move(GdkEventMotion *motion_event);

private:
    CanvasGL &ca;

    int active;
    Coordf sel_o;

    class Box {
    public:
        Box(class CanvasGL &c) : ca(c)
        {
        }
        void realize();
        void render();
        CanvasGL &ca;

        GLuint program;
        GLuint vao;
        GLuint vbo;

        GLuint screenmat_loc;
        GLuint viewmat_loc;
        GLuint scale_loc;
        GLuint a_loc;
        GLuint b_loc;
        GLuint fill_loc;
        GLuint color_loc;

        Coordf sel_a;
        Coordf sel_b;
        bool fill = true;

        void update();
    };
    Box box;

    class Line {
    public:
        Line(class CanvasGL &c) : ca(c)
        {
        }
        void realize();
        void render();
        void push();
        void create_vao();
        CanvasGL &ca;

        GLuint program;
        GLuint vao;
        GLuint vbo;

        GLuint screenmat_loc;
        GLuint viewmat_loc;
        GLuint scale_loc;
        GLuint color_loc;

        class Vertex {
        public:
            Vertex(float ix, float iy) : x(ix), y(iy)
            {
            }

            float x, y;
        };

        std::vector<Vertex> vertices;
        ClipperLib::Path path;

        void update();
    };
    Line line;
};
} // namespace horizon
