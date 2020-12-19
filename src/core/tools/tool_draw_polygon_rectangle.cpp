#include "tool_draw_polygon_rectangle.hpp"
#include "common/polygon.hpp"
#include "imp/imp_interface.hpp"
#include <iostream>
#include <sstream>
#include "document/idocument.hpp"
#include "nlohmann/json.hpp"

namespace horizon {

static const LutEnumStr<ToolDrawPolygonRectangle::Settings::Mode> mode_lut = {
        {"center", ToolDrawPolygonRectangle::Settings::Mode::CENTER},
        {"corner", ToolDrawPolygonRectangle::Settings::Mode::CORNER},
};

void ToolDrawPolygonRectangle::Settings::load_from_json(const json &j)
{
    mode = mode_lut.lookup(j.at("mode"));
}

json ToolDrawPolygonRectangle::Settings::serialize() const
{
    json j;
    j["mode"] = mode_lut.lookup_reverse(mode);
    return j;
}

bool ToolDrawPolygonRectangle::can_begin()
{
    return doc.r->has_object_type(ObjectType::POLYGON);
}

void ToolDrawPolygonRectangle::apply_settings()
{
    if (!temp)
        return;
    update_polygon();
    update_tip();
}

void ToolDrawPolygonRectangle::update_polygon()
{
    temp->vertices.clear();
    if (step == 1) {
        Coordi p0, p1;
        Coordi min_size(corner_radius * 2 + 100, corner_radius * 2 + 100);
        if (settings.mode == Settings::Mode::CORNER) {
            p0 = Coordi::min(first_pos, second_pos);
            p1 = Coordi::max(Coordi::max(first_pos, second_pos), p0 + min_size);
        }
        else {
            auto &center = first_pos;
            auto a = second_pos - center;
            a.x = std::max(std::abs(a.x), corner_radius + 50);
            a.y = std::max(std::abs(a.y), corner_radius + 50);
            p0 = center - a;
            p1 = center + a;
        }

        if (corner_radius > 0) {
            Polygon::Vertex *v;
            v = temp->append_vertex(Coordi(p0.x + corner_radius, p0.y));
            v->type = Polygon::Vertex::Type::ARC;
            v->arc_reverse = true;
            v->arc_center = Coordi(p0.x + corner_radius, p0.y + corner_radius);

            temp->append_vertex(Coordi(p0.x, p0.y + corner_radius));

            v = temp->append_vertex(Coordi(p0.x, p1.y - corner_radius));
            v->type = Polygon::Vertex::Type::ARC;
            v->arc_reverse = true;
            v->arc_center = Coordi(p0.x + corner_radius, p1.y - corner_radius);
            temp->append_vertex(Coordi(p0.x + corner_radius, p1.y));

            v = temp->append_vertex(Coordi(p1.x - corner_radius, p1.y));
            v->type = Polygon::Vertex::Type::ARC;
            v->arc_reverse = true;
            v->arc_center = Coordi(p1.x - corner_radius, p1.y - corner_radius);
            temp->append_vertex(Coordi(p1.x, p1.y - corner_radius));

            v = temp->append_vertex(Coordi(p1.x, p0.y + corner_radius));
            v->type = Polygon::Vertex::Type::ARC;
            v->arc_reverse = true;
            v->arc_center = Coordi(p1.x - corner_radius, p0.y + corner_radius);
            temp->append_vertex(Coordi(p1.x - corner_radius, p0.y));
        }
        else {

            if (decoration == Decoration::NONE) {
                temp->vertices.emplace_back(p0);
                temp->vertices.emplace_back(Coordi(p0.x, p1.y));
                temp->vertices.emplace_back(p1);
                temp->vertices.emplace_back(Coordi(p1.x, p0.y));
            }
            else if (decoration == Decoration::CHAMFER) {
                if (decoration_pos == 0) {
                    temp->vertices.emplace_back(p0);
                    temp->vertices.emplace_back(Coordi(p0.x, p1.y - decoration_size));
                    temp->vertices.emplace_back(Coordi(p0.x + decoration_size, p1.y));
                    temp->vertices.emplace_back(p1);
                    temp->vertices.emplace_back(Coordi(p1.x, p0.y));
                }
                else if (decoration_pos == 1) {
                    temp->vertices.emplace_back(p0);
                    temp->vertices.emplace_back(Coordi(p0.x, p1.y));
                    temp->vertices.emplace_back(Coordi(p1.x - decoration_size, p1.y));
                    temp->vertices.emplace_back(Coordi(p1.x, p1.y - decoration_size));
                    temp->vertices.emplace_back(Coordi(p1.x, p0.y));
                }
                else if (decoration_pos == 2) {
                    temp->vertices.emplace_back(p0);
                    temp->vertices.emplace_back(Coordi(p0.x, p1.y));
                    temp->vertices.emplace_back(p1);
                    temp->vertices.emplace_back(Coordi(p1.x, p0.y + decoration_size));
                    temp->vertices.emplace_back(Coordi(p1.x - decoration_size, p0.y));
                }
                else if (decoration_pos == 3) {
                    temp->vertices.emplace_back(Coordi(p0.x + decoration_size, p0.y));
                    temp->vertices.emplace_back(Coordi(p0.x, p0.y + decoration_size));
                    temp->vertices.emplace_back(Coordi(p0.x, p1.y));
                    temp->vertices.emplace_back(p1);
                    temp->vertices.emplace_back(Coordi(p1.x, p0.y));
                }
            }
            else if (decoration == Decoration::NOTCH) {
                if (decoration_pos == 0) {
                    temp->vertices.emplace_back(p0);
                    temp->vertices.emplace_back(Coordi(p0.x, p1.y));
                    auto c = (Coordi(p0.x, p1.y) + p1) / 2;
                    temp->vertices.emplace_back(Coordi(c.x - decoration_size / 2, c.y));
                    temp->vertices.emplace_back(Coordi(c.x, c.y - decoration_size / 2));
                    temp->vertices.emplace_back(Coordi(c.x + decoration_size / 2, c.y));
                    temp->vertices.emplace_back(p1);
                    temp->vertices.emplace_back(Coordi(p1.x, p0.y));
                }
                if (decoration_pos == 1) {
                    temp->vertices.emplace_back(p0);
                    temp->vertices.emplace_back(Coordi(p0.x, p1.y));
                    temp->vertices.emplace_back(p1);
                    auto c = (Coordi(p1.x, p0.y) + p1) / 2;
                    temp->vertices.emplace_back(Coordi(c.x, c.y + decoration_size / 2));
                    temp->vertices.emplace_back(Coordi(c.x - decoration_size / 2, c.y));
                    temp->vertices.emplace_back(Coordi(c.x, c.y - decoration_size / 2));
                    temp->vertices.emplace_back(Coordi(p1.x, p0.y));
                }
                if (decoration_pos == 2) {
                    temp->vertices.emplace_back(p0);
                    temp->vertices.emplace_back(Coordi(p0.x, p1.y));
                    temp->vertices.emplace_back(p1);
                    temp->vertices.emplace_back(Coordi(p1.x, p0.y));
                    auto c = (Coordi(p1.x, p0.y) + p0) / 2;
                    temp->vertices.emplace_back(Coordi(c.x + decoration_size / 2, c.y));
                    temp->vertices.emplace_back(Coordi(c.x, c.y + decoration_size / 2));
                    temp->vertices.emplace_back(Coordi(c.x - decoration_size / 2, c.y));
                }
                if (decoration_pos == 3) {
                    temp->vertices.emplace_back(p0);
                    auto c = (Coordi(p0.x, p1.y) + p0) / 2;
                    temp->vertices.emplace_back(Coordi(c.x, c.y - decoration_size / 2));
                    temp->vertices.emplace_back(Coordi(c.x + decoration_size / 2, c.y));
                    temp->vertices.emplace_back(Coordi(c.x, c.y + decoration_size / 2));
                    temp->vertices.emplace_back(Coordi(p0.x, p1.y));
                    temp->vertices.emplace_back(p1);
                    temp->vertices.emplace_back(Coordi(p1.x, p0.y));
                }
            }
        }
    }
}

ToolResponse ToolDrawPolygonRectangle::begin(const ToolArgs &args)
{
    std::cout << "tool draw line poly\n";

    temp = doc.r->insert_polygon(UUID::random());
    imp->set_snap_filter({{ObjectType::POLYGON, temp->uuid}});
    temp->layer = args.work_layer;
    first_pos = args.coords;

    update_tip();
    return ToolResponse();
}

void ToolDrawPolygonRectangle::update_tip()
{

    std::vector<ActionLabelInfo> actions;
    actions.reserve(8);

    if (settings.mode == Settings::Mode::CENTER) {
        if (step == 0) {
            actions.emplace_back(InToolActionID::LMB, "place center");
        }
        else {
            actions.emplace_back(InToolActionID::LMB, "place corner");
        }
    }
    else {
        if (step == 0) {
            actions.emplace_back(InToolActionID::LMB, "place first corner");
        }
        else {
            actions.emplace_back(InToolActionID::LMB, "place second corner");
        }
    }
    actions.emplace_back(InToolActionID::RMB, "cancel");
    actions.emplace_back(InToolActionID::RECTANGLE_MODE, "switch mode");
    actions.emplace_back(InToolActionID::POLYGON_CORNER_RADIUS);

    if (corner_radius == 0) {
        actions.emplace_back(InToolActionID::POLYGON_DECORATION_STYLE);
        actions.emplace_back(InToolActionID::POLYGON_DECORATION_POSITION);
        actions.emplace_back(InToolActionID::POLYGON_DECORATION_SIZE);
    }

    imp->tool_bar_set_actions(actions);
    if (settings.mode == Settings::Mode::CENTER) {
        imp->tool_bar_set_tip("from center");
    }
    else {
        imp->tool_bar_set_tip("corners");
    }
}

ToolResponse ToolDrawPolygonRectangle::update(const ToolArgs &args)
{
    if (args.type == ToolEventType::MOVE) {
        if (step == 0) {
            first_pos = args.coords;
        }
        else if (step == 1) {
            second_pos = args.coords;
            update_polygon();
        }
    }
    else if (args.type == ToolEventType::ACTION) {
        switch (args.action) {
        case InToolActionID::LMB:
            if (step == 0) {
                step = 1;
            }
            else {
                return ToolResponse::commit();
            }
            break;

        case InToolActionID::RMB:
        case InToolActionID::CANCEL:
            return ToolResponse::revert();

        case InToolActionID::RECTANGLE_MODE:
            settings.mode = settings.mode == Settings::Mode::CENTER ? Settings::Mode::CORNER : Settings::Mode::CENTER;
            update_polygon();
            break;

        case InToolActionID::POLYGON_DECORATION_POSITION:
            if (corner_radius == 0) {
                decoration_pos = (decoration_pos + 1) % 4;
                update_polygon();
            }
            break;

        case InToolActionID::POLYGON_DECORATION_SIZE:
            if (corner_radius == 0) {
                if (auto r = imp->dialogs.ask_datum("Decoration size", decoration_size)) {
                    decoration_size = *r;
                }
                update_polygon();
            }
            break;

        case InToolActionID::POLYGON_DECORATION_STYLE:
            if (corner_radius == 0) {
                if (decoration == Decoration::NONE) {
                    decoration = Decoration::CHAMFER;
                }
                else if (decoration == Decoration::CHAMFER) {
                    decoration = Decoration::NOTCH;
                }
                else {
                    decoration = Decoration::NONE;
                }
                update_polygon();
            }
            break;

        case InToolActionID::POLYGON_CORNER_RADIUS: {
            if (auto r = imp->dialogs.ask_datum("Corner radius", corner_radius)) {
                corner_radius = *r;
            }
            update_polygon();
        } break;

        default:;
        }
    }
    else if (args.type == ToolEventType::LAYER_CHANGE) {
        temp->layer = args.work_layer;
    }
    update_tip();
    return ToolResponse();
}
} // namespace horizon
