#include "drag_selection.hpp"
#include "canvas_gl.hpp"
#include "clipper/clipper.hpp"
#include "common/layer_provider.hpp"
#include "common/object_descr.hpp"
#include "gl_util.hpp"
#include <glm/gtc/type_ptr.hpp>

namespace horizon {
DragSelection::DragSelection(class CanvasGL *c) : ca(c), active(0), box(this), line(this)
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
    glUniformMatrix3fv(screenmat_loc, 1, GL_FALSE, glm::value_ptr(parent->ca->screenmat));
    glUniformMatrix3fv(viewmat_loc, 1, GL_FALSE, glm::value_ptr(parent->ca->viewmat));
    glUniform1f(scale_loc, parent->ca->scale);
    glUniform2f(a_loc, sel_a.x, sel_a.y);
    glUniform2f(b_loc, sel_b.x, sel_b.y);
    glUniform1i(fill_loc, fill);
    auto co = ca->get_color(ColorP::SELECTION_BOX);
    gl_color_to_uniform_3f(color_loc, co);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glBindVertexArray(0);
    glUseProgram(0);
}

void DragSelection::Line::render()
{
    glUseProgram(program);
    glBindVertexArray(vao);
    glUniformMatrix3fv(screenmat_loc, 1, GL_FALSE, glm::value_ptr(parent->ca->screenmat));
    glUniformMatrix3fv(viewmat_loc, 1, GL_FALSE, glm::value_ptr(parent->ca->viewmat));
    glUniform1f(scale_loc, parent->ca->scale);
    auto co = ca->get_color(ColorP::SELECTION_LINE);
    gl_color_to_uniform_3f(color_loc, co);


    glDrawArrays(GL_LINE_STRIP, 0, vertices.size());

    glBindVertexArray(0);
    glUseProgram(0);
}

void DragSelection::Line::push()
{
    if (!vertices.size())
        return;
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    auto n_vertices = vertices.size();
    if (parent->ca->selection_tool == CanvasGL::SelectionTool::PAINT) {
        glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * n_vertices, vertices.data(), GL_STREAM_DRAW);
    }
    else {
        n_vertices += 1;
        glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * n_vertices, nullptr, GL_STREAM_DRAW);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(Vertex), vertices.data() + (vertices.size() - 1));
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
    if (ca->selection_tool == CanvasGL::SelectionTool::BOX)
        box.render();

    if (is_line_sel(ca->selection_tool))
        line.render();
}

void DragSelection::drag_begin(GdkEventButton *button_event)
{
    if (button_event->state & Gdk::SHIFT_MASK)
        return;
    if (!ca->selection_allowed)
        return;
    if (button_event->type == GDK_2BUTTON_PRESS) {
        active = 0;
        ca->drag_selection_inhibited = false;
        return;
    }
    gdouble x, y;
    gdk_event_get_coords((GdkEvent *)button_event, &x, &y);
    if (button_event->button == 1) { // inside of grid and middle mouse button
        active = 1;
        sel_o = Coordf(x, y);
        if (!is_line_sel(ca->selection_tool)) {
            box.sel_a = ca->screen2canvas(sel_o);
            box.sel_b = box.sel_a;
        }
        else {
            line.vertices.clear();
            auto c = ca->screen2canvas(sel_o);
            line.vertices.emplace_back(c.x, c.y);
            line.path.clear();
            line.path.emplace_back(c.x, c.y);
        }
        ca->queue_draw();
    }
}

void DragSelection::drag_move(GdkEventMotion *motion_event)
{
    gdouble x, y;
    gdk_event_get_coords((GdkEvent *)motion_event, &x, &y);
    if (ca->drag_selection_inhibited && active) {
        active = 0;
        return;
    }

    if (active == 1) {
        if (is_line_sel(ca->selection_tool)) {
            if (ABS(sel_o.x - x) > 10 || ABS(sel_o.y - y) > 10) {
                active = 2;
                ca->set_selection_mode(CanvasGL::SelectionMode::NORMAL);
            }
        }
        else {
            if (ABS(sel_o.x - x) > 10 && ABS(sel_o.y - y) > 10) {
                active = 2;
                ca->set_selection_mode(CanvasGL::SelectionMode::NORMAL);
            }
        }
    }
    else if (active == 2) {
        if (!is_line_sel(ca->selection_tool)) {
            box.sel_b = ca->screen2canvas(Coordf(x, y));
            box.update();
        }
        else {
            auto c = ca->screen2canvas(Coordf(x, y));
            line.vertices.emplace_back(c.x, c.y);
            line.path.emplace_back(c.x, c.y);
            line.update();
            ca->request_push(CanvasGL::PF_DRAG_SELECTION);
            ca->request_push(CanvasGL::PF_SELECTABLES);
        }
        ca->queue_draw();
    }
}

void DragSelection::drag_end(GdkEventButton *button_event)
{
    if (button_event->button == 1) { // inside of grid and middle mouse button {
        bool toggle = button_event->state & Gdk::CONTROL_MASK;
        if (active == 2) {
            for (auto &it : ca->selectables.items) {
                if (it.get_flag(Selectable::Flag::PRELIGHT)) {
                    if (toggle)
                        it.set_flag(Selectable::Flag::SELECTED, !it.get_flag(Selectable::Flag::SELECTED));
                    else
                        it.set_flag(Selectable::Flag::SELECTED, true);
                }
                else {
                    if (!toggle)
                        it.set_flag(Selectable::Flag::SELECTED, false);
                }
                it.set_flag(Selectable::Flag::PRELIGHT, false);
            }
            ca->request_push(CanvasGL::PF_DRAG_SELECTION);
            ca->request_push(CanvasGL::PF_SELECTABLES);
            ca->s_signal_selection_changed.emit();
        }
        else if (active == 1) {
            std::cout << "click select" << std::endl;
            if (ca->selection_mode == CanvasGL::SelectionMode::HOVER) { // just select what was
                                                                        // selecte by hover select
                ca->set_selection_mode(CanvasGL::SelectionMode::NORMAL);
                ca->s_signal_selection_changed.emit();
            }
            else {
                std::set<SelectableRef> selection;
                selection = ca->get_selection();
                gdouble x, y;
                gdk_event_get_coords((GdkEvent *)button_event, &x, &y);
                auto c = ca->screen2canvas({(float)x, (float)y});
                std::vector<unsigned int> in_selection;
                {
                    unsigned int i = 0;
                    for (auto &it : ca->selectables.items) {
                        it.set_flag(Selectable::Flag::PRELIGHT, false);
                        it.set_flag(Selectable::Flag::SELECTED, false);
                        if (it.inside(c, 10 / ca->scale)
                            && ca->selection_filter.can_select(ca->selectables.items_ref[i])) {
                            in_selection.push_back(i);
                        }
                        i++;
                    }
                }
                if (in_selection.size() > 1) {
                    ca->set_selection(selection, false);
                    for (const auto it : ca->clarify_menu->get_children()) {
                        ca->clarify_menu->remove(*it);
                    }
                    for (auto i : in_selection) {
                        const auto sr = ca->selectables.items_ref[i];

                        std::string text = object_descriptions.at(sr.type).name;
                        auto display_name = ca->s_signal_request_display_name.emit(sr.type, sr.uuid);
                        if (display_name.size()) {
                            text += " " + display_name;
                        }
                        auto layers = ca->layer_provider->get_layers();
                        if (layers.count(sr.layer)) {
                            text += " (" + layers.at(sr.layer).name + ")";
                        }
                        Gtk::MenuItem *la = nullptr;
                        if (toggle) {
                            auto l = Gtk::manage(new Gtk::CheckMenuItem(text));
                            l->set_active(selection.count(sr));
                            la = l;
                        }
                        else {
                            la = Gtk::manage(new Gtk::MenuItem(text));
                        }
                        la->signal_select().connect([this, selection, sr, toggle] {
                            auto sel = selection;
                            if (toggle) {
                                if (sel.count(sr)) {
                                    sel.erase(sr);
                                }
                                else {
                                    sel.insert(sr);
                                }
                                ca->set_selection(sel);
                            }
                            else {
                                ca->set_selection({sr}, false);
                            }
#ifdef G_OS_WIN32 // work around a bug(?) in intel(?) GPU drivers on windows
                            Glib::signal_idle().connect_once([this] { ca->queue_draw(); });
#endif
                        });
                        la->signal_deselect().connect([this, selection, toggle] {
                            if (toggle) {
                                ca->set_selection(selection, false);
                            }
                            else {
                                ca->set_selection({}, false);
                            }
#ifdef G_OS_WIN32 // work around a bug(?) in intel(?) GPU drivers on windows
                            Glib::signal_idle().connect_once([this] { ca->queue_draw(); });
#endif
                        });
                        la->signal_activate().connect([this, sr, selection, toggle] {
                            auto sel = selection;
                            if (toggle) {
                                if (sel.count(sr)) {
                                    sel.erase(sr);
                                }
                                else {
                                    sel.insert(sr);
                                }
                                ca->set_selection(sel);
                            }
                            else {
                                ca->set_selection({sr}, true);
                            }
                        });
                        la->show();
                        ca->clarify_menu->append(*la);
                    }
#if GTK_CHECK_VERSION(3, 22, 0)
                    ca->clarify_menu->popup_at_pointer((GdkEvent *)button_event);
#else
                    ca->clarify_menu->popup(0, gtk_get_current_event_time());
#endif
                }
                else if (in_selection.size() == 1) {
                    auto sel = ca->selectables.items_ref[in_selection.front()];
                    if (toggle) {
                        if (selection.count(sel)) {
                            selection.erase(sel);
                        }
                        else {
                            selection.insert(sel);
                        }
                        ca->set_selection(selection);
                    }
                    else {
                        ca->set_selection({sel});
                    }
                }
                else if (in_selection.size() == 0) {
                    if (toggle)
                        ca->set_selection(selection);
                    else
                        ca->set_selection({});
                }
            }
        }
        active = 0;
        ca->queue_draw();
        ca->drag_selection_inhibited = false;
    }
}

void DragSelection::Box::update()
{
    float xmin = std::min(sel_a.x, sel_b.x);
    float xmax = std::max(sel_a.x, sel_b.x);
    float ymin = std::min(sel_a.y, sel_b.y);
    float ymax = std::max(sel_a.y, sel_b.y);
    unsigned int i = 0;
    for (auto &it : ca->selectables.items) {
        it.set_flag(Selectable::Flag::PRELIGHT, false);
        if (ca->selection_filter.can_select(ca->selectables.items_ref[i])) {
            auto sq = ca->selection_qualifier;

            if (sq == CanvasGL::SelectionQualifier::AUTO) {
                if (sel_a.x < sel_b.x)
                    sq = CanvasGL::SelectionQualifier::INCLUDE_BOX;
                else
                    sq = CanvasGL::SelectionQualifier::TOUCH_BOX;
            }

            if (sq == CanvasGL::SelectionQualifier::INCLUDE_ORIGIN) {
                if (it.x > xmin && it.x < xmax && it.y > ymin && it.y < ymax) {
                    it.set_flag(Selectable::Flag::PRELIGHT, true);
                }
                fill = true;
            }
            else if (sq == CanvasGL::SelectionQualifier::INCLUDE_BOX) {
                auto corners = it.get_corners();
                if (std::all_of(corners.begin(), corners.end(), [xmin, xmax, ymin, ymax](const auto &a) {
                        return a.x > xmin && a.x < xmax && a.y > ymin && a.y < ymax;
                    })) {
                    it.set_flag(Selectable::Flag::PRELIGHT, true);
                }
                fill = false;
            }
            else if (sq == CanvasGL::SelectionQualifier::TOUCH_BOX) {
                // possible optimisation: don't use clipper
                ClipperLib::Path clbox(4);
                clbox.at(0) = ClipperLib::IntPoint(xmin, ymin);
                clbox.at(1) = ClipperLib::IntPoint(xmin, ymax);
                clbox.at(2) = ClipperLib::IntPoint(xmax, ymax);
                clbox.at(3) = ClipperLib::IntPoint(xmax, ymin);

                ClipperLib::Path sel(4);
                auto corners = it.get_corners();
                for (size_t j = 0; j < 4; j++) {
                    sel.at(j) = ClipperLib::IntPoint(corners[j].x, corners[j].y);
                }

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
    ca->request_push(CanvasGL::PF_DRAG_SELECTION);
    ca->request_push(CanvasGL::PF_SELECTABLES);
}

void DragSelection::Line::update()
{
    unsigned int i = 0;
    for (auto &it : ca->selectables.items) {
        it.set_flag(Selectable::Flag::PRELIGHT, false);
        if (ca->selection_filter.can_select(ca->selectables.items_ref[i])) {
            if (ca->selection_tool == CanvasGL::SelectionTool::PAINT) {
                ClipperLib::Path sel(4);
                auto corners = it.get_corners();
                for (size_t j = 0; j < 4; j++) {
                    sel.at(j) = ClipperLib::IntPoint(corners[j].x, corners[j].y);
                }

                ClipperLib::Clipper clipper;
                clipper.AddPath(sel, ClipperLib::ptClip, true);
                clipper.AddPath(path, ClipperLib::ptSubject, false);

                ClipperLib::PolyTree isect;
                clipper.Execute(ClipperLib::ctIntersection, isect);

                if (isect.ChildCount()) {
                    it.set_flag(Selectable::Flag::PRELIGHT, true);
                }
            }

            else if (ca->selection_tool == CanvasGL::SelectionTool::LASSO) {
                if (ca->selection_qualifier == CanvasGL::SelectionQualifier::INCLUDE_ORIGIN) {
                    ClipperLib::IntPoint pt(it.x, it.y);
                    if (ClipperLib::PointInPolygon(pt, path) != 0) {
                        it.set_flag(Selectable::Flag::PRELIGHT, true);
                    }
                }
                else if (ca->selection_qualifier == CanvasGL::SelectionQualifier::INCLUDE_BOX
                         || ca->selection_qualifier == CanvasGL::SelectionQualifier::TOUCH_BOX) {
                    ClipperLib::Path sel(4);
                    auto corners = it.get_corners();
                    for (size_t j = 0; j < 4; j++) {
                        sel.at(j) = ClipperLib::IntPoint(corners[j].x, corners[j].y);
                    }

                    ClipperLib::Clipper clipper;
                    clipper.AddPath(sel, ClipperLib::ptSubject, true);
                    clipper.AddPath(path, ClipperLib::ptClip, true);

                    ClipperLib::Paths isect;
                    if (ca->selection_qualifier == CanvasGL::SelectionQualifier::INCLUDE_BOX) {
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
    ca->request_push(CanvasGL::PF_DRAG_SELECTION);
    ca->request_push(CanvasGL::PF_SELECTABLES);
}
} // namespace horizon
