#include "drag_selection.hpp"
#include "canvas_gl.hpp"
#include "clipper/clipper.hpp"
#include "common/layer_provider.hpp"
#include "common/object_descr.hpp"
#include "gl_util.hpp"
#include "util/util.hpp"
#include <glm/gtc/type_ptr.hpp>
#include <iostream>

namespace horizon {
DragSelection::DragSelection(class CanvasGL &c) : ca(c), active(0), box(ca), line(ca)
{
}

static GLuint create_vao_box(GLuint program)
{
    GLuint vao, buffer;

    /* we need to create a VAO to store the other buffers */
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    /* this is the VBO that holds the vertex data */
    glGenBuffers(1, &buffer);
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    // data is buffered lateron

    static const GLfloat vertices[] = {0};
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    /* enable and set the color attribute */
    /* reset the state; we will re-enable the VAO when needed */
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    glDeleteBuffers(1, &buffer);
    return vao;
}

void DragSelection::Box::realize()
{
    program = gl_create_program_from_resource(
            "/org/horizon-eda/horizon/canvas/shaders/"
            "selection-vertex.glsl",
            "/org/horizon-eda/horizon/canvas/shaders/"
            "selection-fragment.glsl",
            nullptr);
    vao = create_vao_box(program);

    GET_LOC(this, screenmat);
    GET_LOC(this, viewmat);
    GET_LOC(this, scale);
    GET_LOC(this, a);
    GET_LOC(this, b);
    GET_LOC(this, fill);
    GET_LOC(this, color);
}

void DragSelection::Line::create_vao()
{
    GLuint position_index = glGetAttribLocation(program, "position");
    GLuint buffer;

    /* we need to create a VAO to store the other buffers */
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    /* this is the VBO that holds the vertex data */
    glGenBuffers(1, &buffer);
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    // data is buffered lateron

    /* enable and set the color attribute */
    glEnableVertexAttribArray(position_index);
    glVertexAttribPointer(position_index, 2, GL_FLOAT, GL_FALSE, sizeof(Line::Vertex), 0);

    /* reset the state; we will re-enable the VAO when needed */
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    vbo = buffer;
}

void DragSelection::Line::realize()
{
    program = gl_create_program_from_resource(
            "/org/horizon-eda/horizon/canvas/shaders/"
            "selection-line-vertex.glsl",
            "/org/horizon-eda/horizon/canvas/shaders/"
            "selection-line-fragment.glsl",
            "/org/horizon-eda/horizon/canvas/shaders/"
            "selection-line-geometry.glsl");
    create_vao();

    GET_LOC(this, screenmat);
    GET_LOC(this, viewmat);
    GET_LOC(this, scale);
    GET_LOC(this, color);
}

void DragSelection::realize()
{
    box.realize();
    line.realize();
}

void DragSelection::Box::render()
{
    glUseProgram(program);
    glBindVertexArray(vao);
    glUniformMatrix3fv(screenmat_loc, 1, GL_FALSE, glm::value_ptr(ca.screenmat));
    glUniformMatrix3fv(viewmat_loc, 1, GL_FALSE, glm::value_ptr(ca.viewmat));
    glUniform1f(scale_loc, ca.scale);
    glUniform2f(a_loc, sel_a.x, sel_a.y);
    glUniform2f(b_loc, sel_b.x, sel_b.y);
    glUniform1i(fill_loc, fill);
    auto co = ca.get_color(ColorP::SELECTION_BOX);
    gl_color_to_uniform_3f(color_loc, co);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glBindVertexArray(0);
    glUseProgram(0);
}

void DragSelection::Line::render()
{
    if (n_vertices < 2)
        return;
    glUseProgram(program);
    glBindVertexArray(vao);
    glUniformMatrix3fv(screenmat_loc, 1, GL_FALSE, glm::value_ptr(ca.screenmat));
    glUniformMatrix3fv(viewmat_loc, 1, GL_FALSE, glm::value_ptr(ca.viewmat));
    glUniform1f(scale_loc, ca.scale);
    auto co = ca.get_color(ColorP::SELECTION_LINE);
    gl_color_to_uniform_3f(color_loc, co);


    glDrawArrays(GL_LINE_STRIP, 0, n_vertices);

    glBindVertexArray(0);
    glUseProgram(0);
}

void DragSelection::Line::push()
{
    if (vertices.size() < 2) {
        n_vertices = 0; // prevent rendering stale data
        return;
    }
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    n_vertices = vertices.size();
    if (ca.selection_tool == CanvasGL::SelectionTool::PAINT) {
        glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * n_vertices, vertices.data(), GL_STREAM_DRAW);
    }
    else {
        n_vertices += 1;
        glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * n_vertices, nullptr, GL_STREAM_DRAW);
        // buffer last vertex first to close lasso
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(Vertex), vertices.data() + (vertices.size() - 1));
        // buffer all vertices
        glBufferSubData(GL_ARRAY_BUFFER, sizeof(Vertex), vertices.size() * sizeof(Vertex), vertices.data());
    }
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void DragSelection::push()
{
    line.push();
}

static bool is_line_sel(CanvasGL::SelectionTool t)
{
    return t == CanvasGL::SelectionTool::PAINT || t == CanvasGL::SelectionTool::LASSO;
}

void DragSelection::render()
{
    if (active != 2)
        return;
    if (ca.selection_tool == CanvasGL::SelectionTool::BOX)
        box.render();

    if (is_line_sel(ca.selection_tool))
        line.render();
}

void DragSelection::drag_begin(GdkEventButton *button_event)
{
    if (button_event->state & Gdk::SHIFT_MASK)
        return;
    if (!ca.selection_allowed)
        return;
    if (button_event->type == GDK_2BUTTON_PRESS) {
        active = 0;
        ca.drag_selection_inhibited = false;
        return;
    }
    gdouble x, y;
    gdk_event_get_coords((GdkEvent *)button_event, &x, &y);
    if (button_event->button == 1) { // inside of grid and middle mouse button
        active = 1;
        sel_o = Coordf(x, y);
        if (!is_line_sel(ca.selection_tool)) {
            box.sel_a = ca.screen2canvas(sel_o);
            box.sel_b = box.sel_a;
        }
        else {
            line.vertices.clear();
            auto c = ca.screen2canvas(sel_o);
            line.vertices.emplace_back(c.x, c.y);
            line.path.clear();
            line.path.emplace_back(c.x, c.y);
            line.update();
        }
        ca.queue_draw();
    }
}

void DragSelection::drag_move(GdkEventMotion *motion_event)
{
    gdouble x, y;
    gdk_event_get_coords((GdkEvent *)motion_event, &x, &y);
    if (ca.drag_selection_inhibited && active) {
        active = 0;
        return;
    }

    if (active == 1) {
        if (is_line_sel(ca.selection_tool)) {
            if (ABS(sel_o.x - x) > 10 || ABS(sel_o.y - y) > 10) {
                active = 2;
                ca.set_selection_mode(CanvasGL::SelectionMode::NORMAL);
            }
        }
        else {
            if (ABS(sel_o.x - x) > 10 && ABS(sel_o.y - y) > 10) {
                active = 2;
                ca.set_selection_mode(CanvasGL::SelectionMode::NORMAL);
            }
        }
    }
    else if (active == 2) {
        if (!is_line_sel(ca.selection_tool)) {
            box.sel_b = ca.screen2canvas(Coordf(x, y));
            box.update();
        }
        else {
            auto c = ca.screen2canvas(Coordf(x, y));
            line.vertices.emplace_back(c.x, c.y);
            line.path.emplace_back(c.x, c.y);
            line.update();
        }
        ca.queue_draw();
    }
}

void DragSelection::drag_end(GdkEventButton *button_event)
{
    if (button_event->button == 1) { // inside of grid and middle mouse button {
        const bool mod = button_event->state & Gdk::CONTROL_MASK;
        if (active == 2) {
            for (auto &it : ca.selectables.items) {
                if (it.get_flag(Selectable::Flag::PRELIGHT)) {
                    if (mod)
                        it.set_flag(Selectable::Flag::SELECTED, !it.get_flag(Selectable::Flag::SELECTED));
                    else
                        it.set_flag(Selectable::Flag::SELECTED, true);
                }
                else {
                    if (!mod)
                        it.set_flag(Selectable::Flag::SELECTED, false);
                }
                it.set_flag(Selectable::Flag::PRELIGHT, false);
            }
            ca.selectables.update_preview(ca.get_selection());
            ca.request_push(CanvasGL::PF_DRAG_SELECTION);
            ca.request_push(CanvasGL::PF_SELECTABLES);
            ca.s_signal_selection_changed.emit();
        }
        else if (active == 1) {
            std::cout << "click select" << std::endl;
            if (ca.selection_mode == CanvasGL::SelectionMode::HOVER) { // just select what was
                                                                       // selecte by hover select
                auto sel = ca.get_selection();
                if (sel.size()) {
                    ca.set_selection_mode(CanvasGL::SelectionMode::NORMAL);
                    ca.selectables.update_preview(sel);
                    ca.s_signal_selection_changed.emit();
                }
            }
            else {
                std::set<SelectableRef> selection;
                selection = ca.get_selection();
                gdouble x, y;
                gdk_event_get_coords((GdkEvent *)button_event, &x, &y);
                auto c = ca.screen2canvas({(float)x, (float)y});
                for (auto &it : ca.selectables.items) {
                    it.set_flag(Selectable::Flag::PRELIGHT, false);
                    it.set_flag(Selectable::Flag::SELECTED, false);
                }
                auto sel_from_canvas = ca.get_selection_at(Coordi(c.x, c.y));

                if (sel_from_canvas.size() > 1) {
                    ca.set_selection(selection, false);
                    for (const auto it : ca.clarify_menu->get_children()) {
                        ca.clarify_menu->remove(*it);
                    }
                    for (const auto sr : sel_from_canvas) {
                        auto text = ca.s_signal_request_display_name.emit(sr);
                        Gtk::MenuItem *la = nullptr;
                        if (mod) {
                            auto l = Gtk::manage(new Gtk::CheckMenuItem(text));
                            l->set_active(selection.count(sr));
                            la = l;
                        }
                        else {
                            la = Gtk::manage(new Gtk::MenuItem(text));
                        }
                        la->signal_select().connect([this, selection, sr, mod] {
                            auto sel = selection;
                            if (mod) {
                                if (sel.count(sr)) {
                                    sel.erase(sr);
                                }
                                else {
                                    sel.insert(sr);
                                }
                                ca.set_selection(sel);
                            }
                            else {
                                ca.set_selection({sr}, false);
                            }
                        });
                        la->signal_deselect().connect([this, selection, mod] {
                            if (mod) {
                                ca.set_selection(selection, false);
                            }
                            else {
                                ca.set_selection({}, false);
                            }
                        });
                        la->signal_activate().connect([this, sr, selection, mod] {
                            auto sel = selection;
                            if (mod) {
                                if (sel.count(sr)) {
                                    sel.erase(sr);
                                }
                                else {
                                    sel.insert(sr);
                                }
                                ca.set_selection(sel);
                            }
                            else {
                                ca.set_selection({sr}, true);
                            }
                        });
                        la->show();
                        ca.clarify_menu->append(*la);
                    }
#if GTK_CHECK_VERSION(3, 22, 0)
                    ca.clarify_menu->popup_at_pointer((GdkEvent *)button_event);
#else
                    ca.clarify_menu->popup(0, gtk_get_current_event_time());
#endif
                }
                else if (sel_from_canvas.size() == 1) {
                    auto sel = *sel_from_canvas.begin();
                    if (mod) {
                        if (selection.count(sel)) {
                            selection.erase(sel);
                        }
                        else {
                            selection.insert(sel);
                        }
                        ca.set_selection(selection);
                    }
                    else {
                        ca.set_selection({sel});
                    }
                }
                else if (sel_from_canvas.size() == 0) {
                    if (mod)
                        ca.set_selection(selection);
                    else
                        ca.set_selection({});
                }
            }
        }
        active = 0;
        ca.queue_draw();
        ca.drag_selection_inhibited = false;
    }
}

static ClipperLib::IntPoint to_pt(const Coordf &p)
{
    return ClipperLib::IntPoint(p.x, p.y);
}

static ClipperLib::Path path_from_sel(const Selectable &sel)
{
    ClipperLib::Path path;
    if (!sel.is_arc()) {
        path.reserve(4);
        auto corners = sel.get_corners();
        for (const auto &c : corners) {
            path.emplace_back(c.x, c.y);
        }
    }
    else {
        unsigned int steps = 15;
        const float a0 = sel.width;
        const float dphi = sel.height;
        const float r0 = sel.c_x - 100;
        const float r1 = sel.c_y + 100;
        const auto c = sel.get_arc_center();

        path.reserve((steps + 1) * 2);
        for (unsigned int i = 0; i <= steps; i++) {
            const float a = a0 + (dphi / steps) * i;
            const auto p = c + Coordf::euler(r0, a);
            path.emplace_back(p.x, p.y);
        }
        for (unsigned int i = 0; i <= steps; i++) {
            const float a = a0 + (dphi / steps) * (steps - i);
            const auto p = c + Coordf::euler(r1, a);
            path.emplace_back(p.x, p.y);
        }
    }
    return path;
}

void DragSelection::Box::update()
{
    const auto sel_center = (sel_a + sel_b) / 2;
    const auto sel_a_screen = ca.canvas2screen(sel_a);
    const auto sel_b_screen = ca.canvas2screen(sel_b);
    const auto sel_width = std::abs(sel_b_screen.x - sel_a_screen.x) / ca.scale;
    const auto sel_height = std::abs(sel_b_screen.y - sel_a_screen.y) / ca.scale;
    const auto sel_angle = (ca.flip_view ? -1 : 1) * ca.view_angle;
    auto in_box = [sel_center, sel_width, sel_height, sel_angle](Coordf p) {
        p -= sel_center;
        p = p.rotate(sel_angle);
        return std::abs(p.x) < sel_width / 2 && std::abs(p.y) < sel_height / 2;
    };
    auto sq = ca.selection_qualifier;

    if (sq == CanvasGL::SelectionQualifier::AUTO) {
        if (sel_a_screen.x < sel_b_screen.x)
            sq = CanvasGL::SelectionQualifier::INCLUDE_BOX;
        else
            sq = CanvasGL::SelectionQualifier::TOUCH_BOX;
    }

    ClipperLib::Path clbox(4);
    if (sq == CanvasGL::SelectionQualifier::TOUCH_BOX) {
        const auto sz1 = Coordf(sel_width / 2, sel_height / 2).rotate(-sel_angle);
        const auto sz2 = Coordf(sel_width / 2, sel_height / -2).rotate(-sel_angle);
        clbox.at(0) = to_pt(sel_center + sz1);
        clbox.at(1) = to_pt(sel_center + sz2);
        clbox.at(2) = to_pt(sel_center - sz1);
        clbox.at(3) = to_pt(sel_center - sz2);
    }

    unsigned int i = 0;
    for (auto &it : ca.selectables.items) {
        it.set_flag(Selectable::Flag::PRELIGHT, false);
        if (ca.selection_filter.can_select(ca.selectables.items_ref[i])) {


            if (sq == CanvasGL::SelectionQualifier::INCLUDE_ORIGIN) {
                if (in_box({it.x, it.y})) {
                    it.set_flag(Selectable::Flag::PRELIGHT, true);
                }
                fill = true;
            }
            else if (sq == CanvasGL::SelectionQualifier::INCLUDE_BOX) {
                auto corners = path_from_sel(it);
                if (std::all_of(corners.begin(), corners.end(),
                                [in_box](const auto &a) { return in_box(Coordf(a.X, a.Y)); })) {
                    it.set_flag(Selectable::Flag::PRELIGHT, true);
                }
                fill = false;
            }
            else if (sq == CanvasGL::SelectionQualifier::TOUCH_BOX) {
                // possible optimisation: don't use clipper
                ClipperLib::Path sel = path_from_sel(it);

                ClipperLib::Clipper clipper;
                clipper.AddPath(clbox, ClipperLib::ptSubject, true);
                clipper.AddPath(sel, ClipperLib::ptClip, true);

                ClipperLib::Paths isect;
                clipper.Execute(ClipperLib::ctIntersection, isect);

                if (isect.size()) {
                    it.set_flag(Selectable::Flag::PRELIGHT, true);
                }
                fill = true;
            }
        }
        i++;
    }
    ca.request_push(CanvasGL::PF_DRAG_SELECTION);
    ca.request_push(CanvasGL::PF_SELECTABLES);
}

void DragSelection::Line::update()
{
    unsigned int i = 0;
    for (auto &it : ca.selectables.items) {
        it.set_flag(Selectable::Flag::PRELIGHT, false);
        if (ca.selection_filter.can_select(ca.selectables.items_ref[i])) {
            if (ca.selection_tool == CanvasGL::SelectionTool::PAINT) {
                ClipperLib::Path sel = path_from_sel(it);

                ClipperLib::Clipper clipper;
                clipper.AddPath(sel, ClipperLib::ptClip, true);
                clipper.AddPath(path, ClipperLib::ptSubject, false);

                ClipperLib::PolyTree isect;
                clipper.Execute(ClipperLib::ctIntersection, isect);

                if (isect.ChildCount()) {
                    it.set_flag(Selectable::Flag::PRELIGHT, true);
                }
            }

            else if (ca.selection_tool == CanvasGL::SelectionTool::LASSO) {
                if (ca.selection_qualifier == CanvasGL::SelectionQualifier::INCLUDE_ORIGIN) {
                    ClipperLib::IntPoint pt(it.x, it.y);
                    if (ClipperLib::PointInPolygon(pt, path) != 0) {
                        it.set_flag(Selectable::Flag::PRELIGHT, true);
                    }
                }
                else if (ca.selection_qualifier == CanvasGL::SelectionQualifier::INCLUDE_BOX
                         || ca.selection_qualifier == CanvasGL::SelectionQualifier::TOUCH_BOX) {
                    ClipperLib::Path sel = path_from_sel(it);

                    ClipperLib::Clipper clipper;
                    clipper.AddPath(sel, ClipperLib::ptSubject, true);
                    clipper.AddPath(path, ClipperLib::ptClip, true);

                    ClipperLib::Paths isect;
                    if (ca.selection_qualifier == CanvasGL::SelectionQualifier::INCLUDE_BOX) {
                        clipper.Execute(ClipperLib::ctDifference, isect); // isect = sel-path

                        if (isect.size() == 0) {
                            it.set_flag(Selectable::Flag::PRELIGHT, true);
                        }
                    }
                    else { // touch box
                        clipper.Execute(ClipperLib::ctIntersection, isect);

                        if (isect.size()) {
                            it.set_flag(Selectable::Flag::PRELIGHT, true);
                        }
                    }
                }
            }
        }
        i++;
    }
    ca.request_push(CanvasGL::PF_DRAG_SELECTION);
    ca.request_push(CanvasGL::PF_SELECTABLES);
}
} // namespace horizon
