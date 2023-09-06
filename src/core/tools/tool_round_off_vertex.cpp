#include "tool_round_off_vertex.hpp"
#include "common/polygon.hpp"
#include "imp/imp_interface.hpp"
#include "util/geom_util.hpp"
#include "document/idocument.hpp"
#include "util/selection_util.hpp"
#include "clipper/clipper.hpp"
#include "dialogs/enter_datum_window.hpp"

namespace horizon {

bool ToolRoundOffVertex::can_begin()
{
    return (sel_count_type(selection, ObjectType::POLYGON_VERTEX) == 1
            || sel_count_type(selection, ObjectType::POLYGON_ARC_CENTER) == 1);
}

int ToolRoundOffVertex::wrap_index(int i) const
{
    while (i < 0) {
        i += poly->vertices.size();
    }
    while (i >= (int)poly->vertices.size())
        i -= poly->vertices.size();
    return i;
}

ToolResponse ToolRoundOffVertex::begin(const ToolArgs &args)
{
    int vertex_idx = 0;
    editing = sel_has_type(selection, ObjectType::POLYGON_ARC_CENTER);
    {
        ObjectType type = editing ? ObjectType::POLYGON_ARC_CENTER : ObjectType::POLYGON_VERTEX;
        auto x = sel_find_one(selection, type);
        poly = doc.r->get_polygon(x.uuid);
        vertex_idx = x.vertex;
    }

    auto v_next = wrap_index(vertex_idx + 1);
    auto v_prev = wrap_index(vertex_idx - 1);


    if (editing) {
        auto v_inserted = v_next; // Inserted vertex
        v_next = wrap_index(vertex_idx + 2);

        vxp = &poly->vertices.at(vertex_idx);
        vxn = &poly->vertices.at(v_inserted);

        if (!revert_arc(p0, vxp->position, vxn->position, vxp->arc_center)) {
            imp->tool_bar_flash("can't undo arc");
            return ToolResponse::end();
        }
    }
    else {
        if ((poly->vertices.at(vertex_idx).type == Polygon::Vertex::Type::ARC)
            || (poly->vertices.at(v_prev).type == Polygon::Vertex::Type::ARC)) {
            imp->tool_bar_flash("can't round off arc");
            return ToolResponse::end();
        }
        p0 = poly->vertices.at(vertex_idx).position;
    }

    selection.clear();

    Coordd p1 = poly->vertices.at(v_next).position;
    Coordd p2 = poly->vertices.at(v_prev).position;

    vn = (p1 - p0).normalize();
    vp = (p2 - p0).normalize();
    vh = (vn + vp).normalize();

    delta_max = std::min((p1 - p0).mag(), (p2 - p0).mag());
    alpha = acos(vh.dot(vp));
    if (isnan(alpha) || (alpha > .99 * (M_PI / 2))) {
        imp->tool_bar_flash("can't round off collinear edges");
        return ToolResponse::end();
    }
    r_max = tan(alpha) * delta_max;

    if (!editing) {
        const bool rev = vn.cross(vp) < 0;

        if (v_next == 0) {
            poly->vertices.emplace_back(Coordi());
            vxn = &poly->vertices.back();
        }
        else {
            vxn = &*poly->vertices.emplace(poly->vertices.begin() + v_next, Coordi());
        }
        vxp = &poly->vertices.at(vertex_idx);
        vxp->type = Polygon::Vertex::Type::ARC;
        vxp->arc_reverse = rev;
    }

    imp->set_snap_filter({{ObjectType::POLYGON, poly->uuid}});

    update_cursor(args.coords);

    plane_init(*poly);

    imp->tool_bar_set_actions({
            {InToolActionID::LMB, "set radius"},
            {InToolActionID::RMB},
            {InToolActionID::FLIP_ARC},
            {InToolActionID::ENTER_DATUM, "enter radius"},
    });

    return ToolResponse();
}

void ToolRoundOffVertex::update_poly(double r)
{
    r = std::min(r_max, r);
    auto delta = r / tan(alpha);
    auto u = r / sin(alpha);
    vxp->position = (p0 + vp * delta).to_coordi();
    vxp->arc_center = (p0 + vh * u).to_coordi();
    vxn->position = (p0 + vn * delta).to_coordi();
}

void ToolRoundOffVertex::update_cursor(const Coordi &c)
{
    auto vm = Coordd(c) - p0;
    auto u = std::max(vm.mag() * vh.dot(vm.normalize()), 0.);
    auto r = u * sin(alpha);
    r = std::min(r_max, r);
    radius_current = r;
    update_poly(r);
    update_tip();
}

void ToolRoundOffVertex::update_tip()
{
    imp->tool_bar_set_tip("Current radius:" + dim_to_string(radius_current, false));
}

ToolResponse ToolRoundOffVertex::update(const ToolArgs &args)
{
    if (args.type == ToolEventType::MOVE) {
        if (imp->dialogs.get_nonmodal() == nullptr)
            update_cursor(args.coords);
    }
    else if (args.type == ToolEventType::ACTION) {
        switch (args.action) {
        case InToolActionID::LMB:
            if (commit())
                return ToolResponse::commit();
            else
                return ToolResponse::revert();
        case InToolActionID::RMB:
        case InToolActionID::CANCEL:
            return ToolResponse::revert();

        case InToolActionID::FLIP_ARC:
            vxp->arc_reverse = !vxp->arc_reverse;
            break;

        case InToolActionID::ENTER_DATUM: {
            auto win = imp->dialogs.show_enter_datum_window("Enter arc radius", radius_current);
            win->set_range(0, r_max);
        } break;

        default:;
        }
    }
    else if (args.type == ToolEventType::DATA) {
        if (auto data = dynamic_cast<const ToolDataWindow *>(args.data.get())) {
            if (data->event == ToolDataWindow::Event::UPDATE) {
                if (auto d = dynamic_cast<const ToolDataEnterDatumWindow *>(args.data.get())) {
                    radius_current = d->value;
                    update_poly(radius_current);
                    update_tip();
                }
            }
            else if (data->event == ToolDataWindow::Event::OK) {
                imp->dialogs.close_nonmodal();
                if (commit())
                    return ToolResponse::commit();
                else
                    return ToolResponse::revert();
            }
        }
    }
    return ToolResponse();
}

bool ToolRoundOffVertex::commit()
{
    if (radius_current < 1) {
        // Tiny radius, just ignore it
        if (!editing)
            return false; // Not a modification so just revert the action

        // Undo change to original vertex
        vxp->arc_center = Coordi();
        vxp->arc_reverse = false;
        vxp->position = p0.to_coordi();
        vxp->type = Polygon::Vertex::Type::LINE;

        // Remove inserted vertex
        vxn->remove = true;
        poly->vertices.erase(
                std::remove_if(poly->vertices.begin(), poly->vertices.end(), [](const auto &x) { return x.remove; }),
                poly->vertices.end());
    }
    return plane_finish();
}

} // namespace horizon
