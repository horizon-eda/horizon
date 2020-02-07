#include "tool_enter_datum.hpp"
#include "common/polygon.hpp"
#include "document/idocument_package.hpp"
#include "pool/package.hpp"
#include "document/idocument_padstack.hpp"
#include "pool/padstack.hpp"
#include "document/idocument_schematic.hpp"
#include "schematic/schematic.hpp"
#include "imp/imp_interface.hpp"
#include "util/accumulator.hpp"
#include <cmath>
#include <iostream>

namespace horizon {

ToolEnterDatum::ToolEnterDatum(IDocument *c, ToolID tid) : ToolBase(c, tid)
{
}

template <typename T> T sgn(T x)
{
    if (x < 0)
        return -1;
    else
        return 1;
}

bool ToolEnterDatum::can_begin()
{
    std::set<ObjectType> types;
    const std::set<ObjectType> types_supported = {
            ObjectType::POLYGON_EDGE, ObjectType::POLYGON_VERTEX, ObjectType::POLYGON_ARC_CENTER,
            ObjectType::HOLE,         ObjectType::JUNCTION,       ObjectType::LINE,
            ObjectType::PAD,          ObjectType::NET_LABEL,      ObjectType::LINE_NET,
            ObjectType::TEXT,         ObjectType::DIMENSION};
    for (const auto &it : selection) {
        types.insert(it.type);
    }
    if (types.size() == 1) {
        auto type = *types.begin();
        return types_supported.count(type);
    }
    else {
        return false;
    }
}

ToolResponse ToolEnterDatum::begin(const ToolArgs &args)
{
    std::cout << "tool enter datum\n";
    bool edge_mode = false;
    bool arc_mode = false;
    bool hole_mode = false;
    bool junction_mode = false;
    bool line_mode = false;
    bool pad_mode = false;
    bool net_mode = false;
    bool text_mode = false;
    bool dim_mode = false;
    bool vertex_mode = false;

    Accumulator<int64_t> ai;
    Accumulator<Coordi> ac;

    for (const auto &it : args.selection) {
        if (it.type == ObjectType::POLYGON_EDGE) {
            edge_mode = true;
            Polygon *poly = doc.r->get_polygon(it.uuid);
            auto v1i = it.vertex;
            auto v2i = (it.vertex + 1) % poly->vertices.size();
            Polygon::Vertex *v1 = &poly->vertices.at(v1i);
            Polygon::Vertex *v2 = &poly->vertices.at(v2i);
            ai.accumulate(sqrt((v1->position - v2->position).mag_sq()));
        }
        if (it.type == ObjectType::POLYGON_ARC_CENTER) {
            arc_mode = true;
            ac.accumulate(doc.r->get_polygon(it.uuid)->vertices.at(it.vertex).arc_center);
        }
        if (it.type == ObjectType::HOLE) {
            hole_mode = true;
            ac.accumulate(doc.r->get_hole(it.uuid)->placement.shift);
        }
        if (it.type == ObjectType::JUNCTION) {
            junction_mode = true;
            ac.accumulate(doc.r->get_junction(it.uuid)->position);
        }
        if (it.type == ObjectType::LINE) {
            line_mode = true;
            auto li = doc.r->get_line(it.uuid);
            auto p0 = li->from->position;
            auto p1 = li->to->position;
            ai.accumulate(sqrt((p0 - p1).mag_sq()));
        }
        if (it.type == ObjectType::PAD) {
            pad_mode = true;
        }
        if (it.type == ObjectType::NET_LABEL) {
            net_mode = true;
        }
        if (it.type == ObjectType::LINE_NET) {
            net_mode = true;
        }
        if (it.type == ObjectType::TEXT) {
            text_mode = true;
        }
        if (it.type == ObjectType::DIMENSION) {
            dim_mode = true;
        }
        if (it.type == ObjectType::POLYGON_VERTEX) {
            vertex_mode = true;
            ac.accumulate(doc.r->get_polygon(it.uuid)->vertices.at(it.vertex).position);
        }
    }
    int m_total = edge_mode + arc_mode + hole_mode + junction_mode + line_mode + pad_mode + net_mode + text_mode
                  + dim_mode + vertex_mode;
    if (m_total != 1) {
        return ToolResponse::end();
    }
    if (edge_mode) {
        auto r = imp->dialogs.ask_datum("Edge length", ai.get());
        if (!r.first) {
            return ToolResponse::end();
        }
        double l = r.second / 2.0;
        for (const auto &it : args.selection) {
            if (it.type == ObjectType::POLYGON_EDGE) {
                Polygon *poly = doc.r->get_polygon(it.uuid);
                auto v1i = it.vertex;
                auto v2i = (it.vertex + 1) % poly->vertices.size();
                Polygon::Vertex *v1 = &poly->vertices.at(v1i);
                Polygon::Vertex *v2 = &poly->vertices.at(v2i);
                Coordi center = (v1->position + v2->position) / 2;
                Coord<double> half = v2->position - center;
                double halflen = sqrt(half.mag_sq());
                double factor = l / halflen;
                half *= factor;
                Coordi halfi(half.x, half.y);

                const auto &uu = it.uuid;
                bool has_v1 =
                        std::find_if(args.selection.cbegin(), args.selection.cend(),
                                     [uu, v1i](const auto a) {
                                         return a.type == ObjectType::POLYGON_VERTEX && a.uuid == uu && a.vertex == v1i;
                                     })
                        != args.selection.cend();
                bool has_v2 =
                        std::find_if(args.selection.cbegin(), args.selection.cend(),
                                     [uu, v2i](const auto a) {
                                         return a.type == ObjectType::POLYGON_VERTEX && a.uuid == uu && a.vertex == v2i;
                                     })
                        != args.selection.cend();
                if (has_v1 && has_v2) {
                    // nop
                }
                else if (has_v1 && !has_v2) {
                    // keep v1, move only v2
                    auto t = v2->position;
                    v2->position = v1->position + halfi * 2;
                    auto d = v2->position - t;
                    v2->arc_center += d;
                }
                else if (!has_v1 && has_v2) {
                    // keep v2, move only v1
                    auto t = v1->position;
                    v1->position = v2->position - halfi * 2;
                    auto d = v1->position - t;
                    v1->arc_center += d;
                }
                else if (!has_v1 && !has_v2) {
                    auto t = v1->position;
                    v1->position = center - halfi;
                    auto d = v1->position - t;
                    v1->arc_center += d;

                    t = v2->position;
                    v2->position = center + halfi;
                    d = v2->position - t;
                    v2->arc_center += d;
                }
            }
        }
    }
    else if (line_mode) {
        auto r = imp->dialogs.ask_datum("Line length", ai.get());
        if (!r.first) {
            return ToolResponse::end();
        }
        double l = r.second / 2.0;
        for (const auto &it : args.selection) {
            if (it.type == ObjectType::LINE) {
                Line *line = doc.r->get_line(it.uuid);
                Junction *j1 = dynamic_cast<Junction *>(line->from.ptr);
                Junction *j2 = dynamic_cast<Junction *>(line->to.ptr);
                Coordi center = (j1->position + j2->position) / 2;
                Coord<double> half = j2->position - center;
                double halflen = sqrt(half.mag_sq());
                double factor = l / halflen;
                half *= factor;
                Coordi halfi(half.x, half.y);

                bool has_v1 = std::find_if(args.selection.cbegin(), args.selection.cend(),
                                           [j1](const auto a) {
                                               return a.type == ObjectType::JUNCTION && a.uuid == j1->uuid;
                                           })
                              != args.selection.cend();
                bool has_v2 = std::find_if(args.selection.cbegin(), args.selection.cend(),
                                           [j2](const auto a) {
                                               return a.type == ObjectType::JUNCTION && a.uuid == j2->uuid;
                                           })
                              != args.selection.cend();
                if (has_v1 && has_v2) {
                    // nop
                }
                else if (has_v1 && !has_v2) {
                    // keep v1, move only v2
                    j2->position = j1->position + halfi * 2;
                }
                else if (!has_v1 && has_v2) {
                    // keep v2, move only v1
                    j1->position = j2->position - halfi * 2;
                }
                else if (!has_v1 && !has_v2) {
                    j1->position = center - halfi;
                    j2->position = center + halfi;
                }
            }
        }
    }
    else if (arc_mode) {
        auto r = imp->dialogs.ask_datum("Arc radius");
        if (!r.first) {
            return ToolResponse::end();
        }
        double l = r.second;
        for (const auto &it : args.selection) {
            if (it.type == ObjectType::POLYGON_ARC_CENTER) {
                Polygon *poly = doc.r->get_polygon(it.uuid);
                auto v1i = it.vertex;
                auto v2i = (it.vertex + 1) % poly->vertices.size();
                Polygon::Vertex *v1 = &poly->vertices.at(v1i);
                Polygon::Vertex *v2 = &poly->vertices.at(v2i);
                Coord<double> r1 = v1->position - v1->arc_center;
                Coord<double> r2 = v2->position - v1->arc_center;
                r1 *= l / sqrt(r1.mag_sq());
                r2 *= l / sqrt(r2.mag_sq());
                v1->position = v1->arc_center + Coordi(r1.x, r1.y);
                v2->position = v1->arc_center + Coordi(r2.x, r2.y);
            }
        }
    }
    else if (hole_mode) {
        auto r = imp->dialogs.ask_datum_coord("Hole position", ac.get());
        if (!r.first) {
            return ToolResponse::end();
        }
        for (const auto &it : args.selection) {
            if (it.type == ObjectType::HOLE) {
                doc.r->get_hole(it.uuid)->placement.shift = r.second;
            }
        }
    }
    else if (junction_mode) {
        bool r;
        Coordi c;
        std::pair<bool, bool> rc;
        std::tie(r, c, rc) = imp->dialogs.ask_datum_coord2("Junction position", ac.get());

        if (!r) {
            return ToolResponse::end();
        }
        for (const auto &it : args.selection) {
            if (it.type == ObjectType::JUNCTION) {
                if (rc.first)
                    doc.r->get_junction(it.uuid)->position.x = c.x;
                if (rc.second)
                    doc.r->get_junction(it.uuid)->position.y = c.y;
            }
        }
    }
    else if (pad_mode) {
        Pad *pad = nullptr;
        for (const auto &it : args.selection) {
            if (it.type == ObjectType::PAD) {
                pad = &doc.k->get_package()->pads.at(it.uuid);
                break;
            }
        }
        if (!pad)
            return ToolResponse::end();
        std::string new_text;
        bool r;
        std::tie(r, new_text) = imp->dialogs.ask_datum_string("Enter pad name", pad->name);
        if (r) {
            for (const auto &it : args.selection) {
                if (it.type == ObjectType::PAD) {
                    doc.k->get_package()->pads.at(it.uuid).name = new_text;
                }
            }
        }
        else {
            return ToolResponse::revert();
        }
    }
    else if (net_mode) {
        Net *net = nullptr;
        for (const auto &it : args.selection) {
            if (it.type == ObjectType::NET_LABEL) {
                auto la = &doc.c->get_sheet()->net_labels.at(it.uuid);
                if (net) {
                    if (net != la->junction->net)
                        return ToolResponse::end();
                }
                else {
                    net = la->junction->net;
                }
            }
            if (it.type == ObjectType::LINE_NET) {
                auto li = &doc.c->get_sheet()->net_lines.at(it.uuid);
                if (net) {
                    if (net != li->net)
                        return ToolResponse::end();
                }
                else {
                    net = li->net;
                }
            }
        }
        if (!net)
            return ToolResponse::end();
        bool r = false;
        std::string net_name;
        std::tie(r, net_name) = imp->dialogs.ask_datum_string("Enter net name", net->name);
        if (r) {
            net->name = net_name;
        }
    }
    else if (text_mode) {
        Text *text = nullptr;
        for (const auto &it : args.selection) {
            if (it.type == ObjectType::TEXT) {
                text = doc.r->get_text(it.uuid);
                break;
            }
        }
        if (!text)
            return ToolResponse::end();
        std::string new_text;
        bool r;
        std::tie(r, new_text) = imp->dialogs.ask_datum_string_multiline("Enter text", text->text);
        if (r) {
            for (const auto &it : args.selection) {
                if (it.type == ObjectType::TEXT) {
                    doc.r->get_text(it.uuid)->text = new_text;
                }
            }
        }
        else {
            return ToolResponse::revert();
        }
    }
    else if (dim_mode) {
        Dimension *dim = nullptr;
        int vertex = -1;
        for (const auto &it : args.selection) {
            if (it.type == ObjectType::DIMENSION) {
                if (vertex != -1) { // two vertices
                    imp->tool_bar_flash("select exactly one dimension point");
                    return ToolResponse::end();
                }
                vertex = it.vertex;
                dim = doc.r->get_dimension(it.uuid);
            }
        }
        if (!dim)
            return ToolResponse::end();
        Coordi *p_ref = nullptr;
        Coordi *p_var = nullptr;
        if (vertex == 0) {
            p_var = &dim->p0;
            p_ref = &dim->p1;
        }
        else if (vertex == 1) {
            p_var = &dim->p1;
            p_ref = &dim->p0;
        }
        else {
            imp->tool_bar_flash("select either start or end point");
            return ToolResponse::end();
        }
        int64_t def = 0;
        switch (dim->mode) {
        case Dimension::Mode::HORIZONTAL:
            def = std::abs(p_ref->x - p_var->x);
            break;
        case Dimension::Mode::VERTICAL:
            def = std::abs(p_ref->y - p_var->y);
            break;
        case Dimension::Mode::DISTANCE:
            def = sqrt((*p_ref - *p_var).mag_sq());
            break;
        }
        auto r = imp->dialogs.ask_datum("Dimension length", def);
        if (!r.first) {
            return ToolResponse::end();
        }
        switch (dim->mode) {
        case Dimension::Mode::HORIZONTAL:
            p_var->x = p_ref->x + (r.second * sgn(p_var->x - p_ref->x));
            break;

        case Dimension::Mode::VERTICAL:
            p_var->y = p_ref->y + (r.second * sgn(p_var->y - p_ref->y));
            break;

        case Dimension::Mode::DISTANCE: {
            Coord<double> v = *p_var - *p_ref;
            v *= 1 / sqrt(v.mag_sq());
            v *= r.second;
            *p_var = *p_ref + Coordi(v.x, v.y);
        } break;
        }
    }
    else if (vertex_mode) {
        bool r;
        Coordi c;
        std::pair<bool, bool> rc;
        std::tie(r, c, rc) = imp->dialogs.ask_datum_coord2("Vertex position", ac.get());

        if (!r) {
            return ToolResponse::end();
        }
        for (const auto &it : args.selection) {
            if (it.type == ObjectType::POLYGON_VERTEX) {
                if (rc.first)
                    doc.r->get_polygon(it.uuid)->vertices.at(it.vertex).position.x = c.x;
                if (rc.second)
                    doc.r->get_polygon(it.uuid)->vertices.at(it.vertex).position.y = c.y;
            }
        }
    }
    else {
        return ToolResponse::end();
    }


    return ToolResponse::commit();
}
ToolResponse ToolEnterDatum::update(const ToolArgs &args)
{
    return ToolResponse();
}
} // namespace horizon
