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
        if (it.type == ObjectType::JUNCTION || it.type == ObjectType::LINE) {
            return true;
        }
    }
    return false;
}

static std::vector<Junction *> visit(Junction *node, Junction *from,
                                     const std::map<Junction *, std::set<Line *>> &junction_connections,
                                     std::vector<Junction *> path)
{
    for (auto li : junction_connections.at(node)) {
        Junction *next = nullptr;
        std::cout << li << std::endl;
        if (li->from == node)
            next = li->to;
        else if (li->to == node)
            next = li->from;
        else
            throw std::runtime_error("oops");
        if (next != from) {
            if (std::count(path.begin(), path.end(), next)) {
                // found cycle
                path.push_back(node);
                return path;
            }
            else {
                path.push_back(node);
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
    }
    if (start_junction == nullptr)
        return ToolResponse::end();

    auto all_lines = core.r->get_lines();
    std::map<Junction *, std::set<Line *>> junction_connections;
    for (auto li : all_lines) {
        for (auto &it_ft : {li->from, li->to}) {
            junction_connections[it_ft];
            junction_connections[it_ft].insert(li);
        }
    }

    auto path = visit(start_junction, nullptr, junction_connections, {});
    std::set<Line *> lines;
    auto poly = core.r->insert_polygon(UUID::random());
    for (const auto ju : path) {
        poly->append_vertex(ju->position);
        for (const auto li : junction_connections.at(ju))
            lines.insert(li);
    }
    poly->layer = start_junction->layer;

    for (auto li : lines) {
        core.r->delete_line(li->uuid);
    }
    for (auto ju : path) {
        core.r->delete_junction(ju->uuid);
    }
    core.r->commit();
    return ToolResponse::end();
}

ToolResponse ToolLineLoopToPolygon::update(const ToolArgs &args)
{

    return ToolResponse();
}
} // namespace horizon
