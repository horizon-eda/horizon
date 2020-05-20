#include "tool_round_off_vertex.hpp"
#include "common/polygon.hpp"
#include "imp/imp_interface.hpp"
#include "util/util.hpp"
#include "document/idocument.hpp"
#include <gdk/gdkkeysyms.h>
#include "util/selection_util.hpp"
#include "clipper/clipper.hpp"
#include "dialogs/enter_datum_window.hpp"

namespace horizon {

ToolRoundOffVertex::ToolRoundOffVertex(IDocument *c, ToolID tid) : ToolBase(c, tid)
{
}

bool ToolRoundOffVertex::can_begin()
{
    return sel_count_type(selection, ObjectType::POLYGON_VERTEX) == 1;
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

using Coordd = Coord<double>;

static Coordd normalize(const Coordd &c)
{
    return c / (sqrt(c.mag_sq()));
}

static Coordi to_coordi(const Coordd &c)
{
    return Coordi(c.x, c.y);
}

ToolResponse ToolRoundOffVertex::begin(const ToolArgs &args)
{
    int vertex_idx = 0;
    {
        auto x = sel_find_one(selection, ObjectType::POLYGON_VERTEX);
        poly = doc.r->get_polygon(x.uuid);
        vertex_idx = x.vertex;
    }

    auto v_next = wrap_index(vertex_idx + 1);
    auto v_prev = wrap_index(vertex_idx - 1);

    if ((poly->vertices.at(vertex_idx).type == Polygon::Vertex::Type::ARC)
        || (poly->vertices.at(v_prev).type == Polygon::Vertex::Type::ARC)) {
        imp->tool_bar_flash("can't round off arc");
        return ToolResponse::end();
    }

    selection.clear();

    p0 = poly->vertices.at(vertex_idx).position;
    vn = normalize(Coordd(poly->vertices.at(v_next).position) - p0);
    vp = normalize(Coordd(poly->vertices.at(v_prev).position) - p0);
    vh = normalize(vn + vp);

    delta_max = sqrt(std::min((poly->vertices.at(v_next).position - poly->vertices.at(vertex_idx).position).mag_sq(),
                              (poly->vertices.at(v_prev).position - poly->vertices.at(vertex_idx).position).mag_sq()));
    alpha = acos(vh.dot(vp));
    if (isnan(alpha) || (alpha > .99 * (M_PI / 2))) {
        imp->tool_bar_flash("can't round off collinear edges");
        return ToolResponse::end();
    }
    r_max = tan(alpha) * delta_max;

    bool rev = false;
    {
        ClipperLib::Path path;
        path.reserve(poly->vertices.size());
        std::transform(poly->vertices.begin(), poly->vertices.end(), std::back_inserter(path),
                       [](const Polygon::Vertex &v) { return ClipperLib::IntPoint(v.position.x, v.position.y); });
        rev = (!ClipperLib::Orientation(path));
    }


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

    imp->set_snap_filter({{ObjectType::POLYGON, poly->uuid}});

    update_cursor(args.coords);

    return ToolResponse();
}

void ToolRoundOffVertex::update_poly(double r)
{
    r = std::min(r_max, r);
    auto delta = r / tan(alpha);
    auto u = r / sin(alpha);
    vxp->position = to_coordi(p0 + vp * delta);
    vxp->arc_center = to_coordi(p0 + vh * u);
    vxn->position = to_coordi(p0 + vn * delta);
}

void ToolRoundOffVertex::update_cursor(const Coordi &c)
{
    auto vm = Coordd(c) - p0;
    auto u = std::max(sqrt(vm.mag_sq()) * vh.dot(normalize(vm)), 0.);
    auto r = u * sin(alpha);
    r = std::min(r_max, r);
    radius_current = r;
    update_poly(r);
    update_tip();
}

void ToolRoundOffVertex::update_tip()
{
    imp->tool_bar_set_tip(
            "<b>LMB:</b>set radius <b>RMB:</b>cancel <b>Return:</b>enter radius <b>e:</b>flip arc <i>Current radius:"
            + dim_to_string(radius_current, false) + "</i>");
}

ToolResponse ToolRoundOffVertex::update(const ToolArgs &args)
{
    if (args.type == ToolEventType::MOVE) {
        if (imp->dialogs.get_nonmodal() == nullptr)
            update_cursor(args.coords);
    }
    else if (args.type == ToolEventType::CLICK) {
        if (args.button == 1) {
            return ToolResponse::commit();
        }
        else if (args.button == 3) {
            selection.clear();
            return ToolResponse::revert();
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
                return ToolResponse::commit();
            }
        }
    }
    else if (args.type == ToolEventType::KEY) {
        if (args.key == GDK_KEY_Escape) {
            selection.clear();
            return ToolResponse::revert();
        }
        else if (args.key == GDK_KEY_e) {
            vxp->arc_reverse = !vxp->arc_reverse;
        }
        else if (args.key == GDK_KEY_Return) {
            auto win = imp->dialogs.show_enter_datum_window("Enter arc radius", radius_current);
            win->set_range(0, r_max);
            /*auto r = imp->dialogs.ask_datum("Enter arc radius", radius_current);
            if (r.first && r.second > 0) {
                update_poly(r.second);
                poly->temp = false;
                return ToolResponse::commit();
            }*/
        }
    }
    return ToolResponse();
}
} // namespace horizon
