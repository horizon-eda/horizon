#include "tool_line_loop_to_polygon.hpp"
#include "common/polygon.hpp"
#include "common/line.hpp"
#include "imp/imp_interface.hpp"
#include <iostream>
#include <functional>

namespace horizon {

ToolLineLoopToPolygon::ToolLineLoopToPolygon(Core *c, ToolID tid) : ToolBase(c, tid)
{
}

bool ToolLineLoopToPolygon::can_begin()
{
    if (!(core.r->has_object_type(ObjectType::LINE) && core.r->has_object_type(ObjectType::POLYGON)))
        return false;
    for (const auto &it : core.r->selection) {
        if (it.type == ObjectType::JUNCTION || it.type == ObjectType::LINE || it.type == ObjectType::ARC) {
            return true;
        }
    }
    return false;
}


class Connector {
public:
    Connector(Line *l) : li(l), arc(nullptr)
    {
    }
    Connector(Arc *a) : li(nullptr), arc(a)
    {
    }
    Line *li;
    Arc *arc;
    Junction *get_from()
    {
        if (li)
            return li->from;
        else
            return arc->from;
    };
    Junction *get_to()
    {
        if (li)
            return li->to;
        else
            return arc->to;
    };
};

static std::vector<std::pair<Junction *, Connector *>>
visit(Junction *node, Junction *from, const std::map<Junction *, std::set<Connector *>> &junction_connections,
      std::vector<std::pair<Junction *, Connector *>> path)
{
    for (auto con : junction_connections.at(node)) {
        Junction *next = nullptr;
        if (con->get_from() == node)
            next = con->get_to();
        else if (con->get_to() == node)
            next = con->get_from();
        else
            throw std::runtime_error("oops");
        if (next != from) {
            if (std::count_if(path.begin(), path.end(), [next](const auto &a) { return a.first == next; })) {
                // found cycle
                path.emplace_back(node, con);
                return path;
            }
            else {
                path.emplace_back(node, con);
                auto r = visit(next, node, junction_connections, path);
                if (r.size())
                    return r;
            }
        }
    }
    return {};
}


ToolResponse ToolLineLoopToPolygon::begin(const ToolArgs &args)
{
    Junction *start_junction = nullptr;
    for (const auto &it : core.r->selection) {
        if (it.type == ObjectType::JUNCTION) {
            start_junction = core.r->get_junction(it.uuid);
            break;
        }
        else if (it.type == ObjectType::LINE) {
            start_junction = core.r->get_line(it.uuid)->from;
            break;
        }
        else if (it.type == ObjectType::ARC) {
            start_junction = core.r->get_arc(it.uuid)->from;
            break;
        }
    }
    if (start_junction == nullptr)
        return ToolResponse::end();

    std::vector<Connector> all_connectors;
    {
        auto all_lines = core.r->get_lines();
        for (auto li : all_lines) {
            all_connectors.emplace_back(li);
        }
        auto all_arcs = core.r->get_arcs();
        for (auto ar : all_arcs) {
            all_connectors.emplace_back(ar);
        }
    }
    std::map<Junction *, std::set<Connector *>> junction_connections;
    for (auto &con : all_connectors) {
        for (auto &it_ft : {con.get_from(), con.get_to()}) {
            junction_connections[it_ft];
            junction_connections[it_ft].insert(&con);
        }
    }

    auto path = visit(start_junction, nullptr, junction_connections, {});
    std::set<Connector *> connectors;
    auto poly = core.r->insert_polygon(UUID::random());
    for (const auto &it : path) {
        auto v = poly->append_vertex(it.first->position);
        if (it.second->arc) {
            v->type = Polygon::Vertex::Type::ARC;
            v->arc_center = it.second->arc->center->position;
            v->arc_reverse = it.first == it.second->arc->to;
        }
        for (const auto con : junction_connections.at(it.first))
            connectors.insert(con);
    }
    poly->layer = start_junction->layer;

    for (auto con : connectors) {
        if (con->li)
            core.r->delete_line(con->li->uuid);
        else if (con->arc)
            core.r->delete_arc(con->arc->uuid);
    }
    for (auto &it : path) {
        core.r->delete_junction(it.first->uuid);
    }
    core.r->commit();
    return ToolResponse::end();
}

ToolResponse ToolLineLoopToPolygon::update(const ToolArgs &args)
{

    return ToolResponse();
}
} // namespace horizon
