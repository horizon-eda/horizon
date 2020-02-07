#include "tool_line_loop_to_polygon.hpp"
#include "common/polygon.hpp"
#include "common/line.hpp"
#include "common/arc.hpp"
#include "imp/imp_interface.hpp"
#include <iostream>
#include <functional>
#include "document/idocument.hpp"

namespace horizon {

ToolLineLoopToPolygon::ToolLineLoopToPolygon(IDocument *c, ToolID tid) : ToolBase(c, tid)
{
}

bool ToolLineLoopToPolygon::can_begin()
{
    if (!(doc.r->has_object_type(ObjectType::LINE) && doc.r->has_object_type(ObjectType::POLYGON)))
        return false;
    for (const auto &it : selection) {
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

static std::deque<std::pair<Junction *, Connector *>>
visit(Junction *node, Junction *from, const std::map<Junction *, std::set<Connector *>> &junction_connections,
      std::deque<std::pair<Junction *, Connector *>> path)
{
    if (!junction_connections.count(node))
        return {};
    for (auto con : junction_connections.at(node)) {
        Junction *next = nullptr;
        if (con->get_from() == node)
            next = con->get_to();
        else if (con->get_to() == node)
            next = con->get_from();
        else // single junction
            return {};
        if (next != from) {
            if (std::count_if(path.begin(), path.end(), [next](const auto &a) { return a.first == next; })) {
                while (path.size() && path.front().first != next) {
                    path.pop_front();
                }
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


void ToolLineLoopToPolygon::remove_from_selection(ObjectType type, const UUID &uu)
{
    auto sel = std::find_if(selection.begin(), selection.end(),
                            [type, uu](const auto &x) { return x.type == type && x.uuid == uu; });
    if (sel != selection.end())
        selection.erase(sel);
}

ToolResponse ToolLineLoopToPolygon::begin(const ToolArgs &args)
{
    bool success = false;
    std::string error_message;
    while (true) {
        Junction *start_junction = nullptr;
        for (const auto &it : selection) {
            if (it.type == ObjectType::JUNCTION) {
                start_junction = doc.r->get_junction(it.uuid);
                break;
            }
            else if (it.type == ObjectType::LINE) {
                start_junction = doc.r->get_line(it.uuid)->from;
                break;
            }
            else if (it.type == ObjectType::ARC) {
                start_junction = doc.r->get_arc(it.uuid)->from;
                break;
            }
        }
        if (start_junction == nullptr) {
            error_message = "found no start junction";
            break;
        }

        std::vector<Connector> all_connectors;
        {
            auto all_lines = doc.r->get_lines();
            for (auto li : all_lines) {
                all_connectors.emplace_back(li);
            }
            auto all_arcs = doc.r->get_arcs();
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
        if (path.size() == 0) {
            error_message = "didn't find a loop";
            break;
        }
        std::set<Connector *> connectors;
        auto poly = doc.r->insert_polygon(UUID::random());
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
        if (doc.c || doc.y || doc.f)
            poly->layer = 0;

        for (auto con : connectors) {
            if (con->li) {
                doc.r->delete_line(con->li->uuid);
                remove_from_selection(ObjectType::LINE, con->li->uuid);
            }
            else if (con->arc) {
                doc.r->delete_arc(con->arc->uuid);
                remove_from_selection(ObjectType::ARC, con->arc->uuid);
            }
        }
        for (auto &it : path) {
            doc.r->delete_junction(it.first->uuid);
            remove_from_selection(ObjectType::JUNCTION, it.first->uuid);
        }
        success = true;
    }
    if (success) {
        return ToolResponse::commit();
    }
    else {
        imp->tool_bar_flash(error_message);
        return ToolResponse::revert();
    }
}

ToolResponse ToolLineLoopToPolygon::update(const ToolArgs &args)
{

    return ToolResponse();
}
} // namespace horizon
