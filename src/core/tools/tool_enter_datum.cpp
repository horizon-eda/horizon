#include "tool_enter_datum.hpp"
#include "common/polygon.hpp"
#include "document/idocument_package.hpp"
#include "pool/package.hpp"
#include "document/idocument_padstack.hpp"
#include "pool/padstack.hpp"
#include "document/idocument_schematic.hpp"
#include "document/idocument_board.hpp"
#include "board/board.hpp"
#include "schematic/schematic.hpp"
#include "imp/imp_interface.hpp"
#include "util/accumulator.hpp"
#include "util/selection_util.hpp"
#include "util/geom_util.hpp"
#include <cmath>
#include <iostream>

namespace horizon {

static std::set<ObjectType> types_from_sel(const std::set<SelectableRef> &sel)
{
    std::set<ObjectType> types;
    for (const auto &it : sel) {
        types.insert(it.type);
    }
    return types;
}

static bool sel_only_has_types(const std::set<SelectableRef> &sel, const std::set<ObjectType> &types)
{
    const auto types_present = types_from_sel(sel);
    std::set<ObjectType> remainder;
    std::set_difference(types_present.begin(), types_present.end(), types.begin(), types.end(),
                        std::inserter(remainder, remainder.begin()));
    return (remainder.size() == 0) && (sel.size() > 0);
}

ToolEnterDatum::Mode ToolEnterDatum::get_mode() const
{
    if (sel_find_exactly_one(selection, ObjectType::DIMENSION))
        return Mode::DIMENSION;
    if (sel_find_exactly_one(selection, ObjectType::PAD))
        return Mode::PAD;
    if (sel_find_exactly_one(selection, ObjectType::NET_LABEL))
        return Mode::NET;
    if (sel_find_exactly_one(selection, ObjectType::LINE_NET))
        return Mode::NET;
    if (sel_only_has_types(selection, {ObjectType::POLYGON_VERTEX}))
        return Mode::POLYGON_VERTEX;
    if (sel_only_has_types(selection, {ObjectType::JUNCTION}))
        return Mode::JUNCTION;
    if (sel_only_has_types(selection, {ObjectType::LINE, ObjectType::JUNCTION}))
        return Mode::LINE;
    if (sel_only_has_types(selection, {ObjectType::TRACK, ObjectType::JUNCTION}))
        return Mode::TRACK;
    if (sel_only_has_types(selection, {ObjectType::POLYGON_EDGE, ObjectType::POLYGON_VERTEX}))
        return Mode::POLYGON_EDGE;
    return Mode::INVALID;
}

bool ToolEnterDatum::can_begin()
{
    return get_mode() != Mode::INVALID;
}

ToolResponse ToolEnterDatum::begin(const ToolArgs &args)
{
    const auto mode = get_mode();
    switch (mode) {
    case Mode::PAD: {
        const auto pad_uu = sel_find_one(selection, ObjectType::PAD).uuid;
        auto &pad = doc.k->get_package().pads.at(pad_uu);
        if (auto r = imp->dialogs.ask_datum_string("Enter pad name", pad.name)) {
            pad.name = *r;
            return ToolResponse::commit();
        }
    } break;

    case Mode::POLYGON_VERTEX: {
        Accumulator<Coordi> acc;
        for (const auto &it : selection) {
            if (it.type == ObjectType::POLYGON_VERTEX)
                acc.accumulate(doc.r->get_polygon(it.uuid)->vertices.at(it.vertex).position);
        }
        if (auto r = imp->dialogs.ask_datum_coord2("Vertex position", acc.get())) {
            auto [c, rc] = *r;
            for (const auto &it : selection) {
                if (it.type == ObjectType::POLYGON_VERTEX) {
                    if (rc.first)
                        doc.r->get_polygon(it.uuid)->vertices.at(it.vertex).position.x = c.x;
                    if (rc.second)
                        doc.r->get_polygon(it.uuid)->vertices.at(it.vertex).position.y = c.y;
                }
            }
            return ToolResponse::commit();
        }

    } break;

    case Mode::NET: {
        Net *net = nullptr;
        if (auto s = sel_find_exactly_one(selection, ObjectType::NET_LABEL)) {
            const auto &la = doc.c->get_sheet()->net_labels.at(s->uuid);
            net = la.junction->net;
        }
        if (auto s = sel_find_exactly_one(selection, ObjectType::LINE_NET)) {
            const auto &li = doc.c->get_sheet()->net_lines.at(s->uuid);
            net = li.net;
        }
        if (!net)
            return ToolResponse::end();

        if (auto r = imp->dialogs.ask_datum_string("Enter net name", net->name)) {
            net->name = *r;
            return ToolResponse::commit();
        }
    } break;

    case Mode::DIMENSION: {
        Dimension *dim = nullptr;
        int vertex = -1;
        for (const auto &it : selection) {
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
            def = (*p_ref - *p_var).magd();
            break;
        }
        if (auto r = imp->dialogs.ask_datum("Dimension length", def)) {
            switch (dim->mode) {
            case Dimension::Mode::HORIZONTAL:
                p_var->x = p_ref->x + (*r * sgn(p_var->x - p_ref->x));
                break;

            case Dimension::Mode::VERTICAL:
                p_var->y = p_ref->y + (*r * sgn(p_var->y - p_ref->y));
                break;

            case Dimension::Mode::DISTANCE: {
                const Coordd v = Coordd(*p_var - *p_ref).normalize() * (*r);
                *p_var = *p_ref + Coordi(v.x, v.y);
            } break;
            }
            return ToolResponse::commit();
        }
    } break;

    case Mode::LINE: {
        Accumulator<int64_t> acc;

        for (const auto &it : selection) {
            if (it.type == ObjectType::LINE) {
                const auto li = doc.r->get_line(it.uuid);
                auto p0 = li->from->position;
                auto p1 = li->to->position;
                acc.accumulate((p0 - p1).magd());
            }
        }

        if (auto r = imp->dialogs.ask_datum("Line length", acc.get())) {
            double l = *r / 2.0;
            for (const auto &it : selection) {
                if (it.type == ObjectType::LINE) {
                    Line *line = doc.r->get_line(it.uuid);
                    Junction *j1 = line->from.ptr;
                    Junction *j2 = line->to.ptr;
                    Coordi center = (j1->position + j2->position) / 2;
                    Coordd half = j2->position - center;
                    const double halflen = half.mag();
                    const double factor = l / halflen;
                    half *= factor;
                    Coordi halfi(half.x, half.y);

                    bool has_v1 = std::find_if(selection.cbegin(), selection.cend(),
                                               [j1](const auto a) {
                                                   return a.type == ObjectType::JUNCTION && a.uuid == j1->uuid;
                                               })
                                  != selection.cend();
                    bool has_v2 = std::find_if(selection.cbegin(), selection.cend(),
                                               [j2](const auto a) {
                                                   return a.type == ObjectType::JUNCTION && a.uuid == j2->uuid;
                                               })
                                  != selection.cend();
                    if (has_v1 && has_v2) {
                        // nop
                    }
                    else if (has_v1 && !has_v2) {

                        // keep v2, move only v1
                        j1->position = j2->position - halfi * 2;
                    }
                    else if (!has_v1 && has_v2) {
                        // keep v1, move only v2
                        j2->position = j1->position + halfi * 2;
                    }
                    else if (!has_v1 && !has_v2) {
                        j1->position = center - halfi;
                        j2->position = center + halfi;
                    }
                }
            }
            return ToolResponse::commit();
        }

    } break;

    case Mode::TRACK: {
        Accumulator<int64_t> acc;

        for (const auto &it : selection) {
            if (it.type == ObjectType::TRACK) {
                const auto &tr = doc.b->get_board()->tracks.at(it.uuid);
                if (tr.from.is_junc() || tr.to.is_junc()) {
                    auto p0 = tr.from.get_position();
                    auto p1 = tr.to.get_position();
                    acc.accumulate((p0 - p1).magd());
                }
            }
        }

        if (auto r = imp->dialogs.ask_datum("Track length", acc.get())) {
            double l = *r / 2.0;
            for (const auto &it : selection) {
                if (it.type == ObjectType::TRACK) {
                    auto &tr = doc.b->get_board()->tracks.at(it.uuid);
                    if (tr.from.is_junc() || tr.to.is_junc()) {
                        Junction *j1 = tr.from.junc;
                        Junction *j2 = tr.to.junc;
                        Coordi center = (tr.from.get_position() + tr.to.get_position()) / 2;
                        Coordd half = tr.to.get_position() - center;
                        const double halflen = half.mag();
                        const double factor = l / halflen;
                        half *= factor;
                        Coordi halfi(half.x, half.y);

                        bool has_v1 =
                                std::find_if(selection.cbegin(), selection.cend(),
                                             [j1](const auto a) {
                                                 return j1 && a.type == ObjectType::JUNCTION && a.uuid == j1->uuid;
                                             })
                                != selection.cend();
                        bool has_v2 =
                                std::find_if(selection.cbegin(), selection.cend(),
                                             [j2](const auto a) {
                                                 return j2 && a.type == ObjectType::JUNCTION && a.uuid == j2->uuid;
                                             })
                                != selection.cend();
                        if (has_v1 && has_v2) {
                            // nop
                        }
                        else if (has_v1 && !has_v2 && j1) {
                            // keep v2, move only v1
                            j1->position = tr.to.get_position() - halfi * 2;
                        }
                        else if (!has_v1 && has_v2 && j2) {
                            // keep v1, move only v2
                            j2->position = tr.from.get_position() + halfi * 2;
                        }
                        else if (!has_v1 && !has_v2) {
                            if (j1 && j2) {
                                j1->position = center - halfi;
                                j2->position = center + halfi;
                            }
                            else if (j1) {
                                j1->position = tr.to.get_position() - halfi * 2;
                            }
                            else if (j2) {
                                j2->position = tr.from.get_position() + halfi * 2;
                            }
                        }
                    }
                }
            }
            return ToolResponse::commit();
        }

    } break;

    case Mode::POLYGON_EDGE: {

        Accumulator<int64_t> acc;

        for (const auto &it : selection) {
            if (it.type == ObjectType::POLYGON_EDGE) {
                const auto poly = doc.r->get_polygon(it.uuid);
                auto v1i = it.vertex;
                auto v2i = (it.vertex + 1) % poly->vertices.size();
                auto *v1 = &poly->vertices.at(v1i);
                auto *v2 = &poly->vertices.at(v2i);
                acc.accumulate((v1->position - v2->position).magd());
            }
        }

        if (auto r = imp->dialogs.ask_datum("Edge length", acc.get())) {
            double l = *r / 2.0;
            for (const auto &it : selection) {
                if (it.type == ObjectType::POLYGON_EDGE) {
                    Polygon *poly = doc.r->get_polygon(it.uuid);
                    auto v1i = it.vertex;
                    auto v2i = (it.vertex + 1) % poly->vertices.size();
                    Polygon::Vertex *v1 = &poly->vertices.at(v1i);
                    Polygon::Vertex *v2 = &poly->vertices.at(v2i);
                    Coordi center = (v1->position + v2->position) / 2;
                    Coordd half = v2->position - center;
                    const double halflen = half.mag();
                    const double factor = l / halflen;
                    half *= factor;
                    Coordi halfi(half.x, half.y);

                    const auto &uu = it.uuid;
                    bool has_v1 = std::find_if(selection.cbegin(), selection.cend(),
                                               [uu, v1i](const auto a) {
                                                   return a.type == ObjectType::POLYGON_VERTEX && a.uuid == uu
                                                          && a.vertex == v1i;
                                               })
                                  != selection.cend();
                    bool has_v2 = std::find_if(selection.cbegin(), selection.cend(),
                                               [uu, v2i](const auto a) {
                                                   return a.type == ObjectType::POLYGON_VERTEX && a.uuid == uu
                                                          && a.vertex == v2i;
                                               })
                                  != selection.cend();
                    if (has_v1 && has_v2) {
                        // nop
                    }
                    else if (has_v1 && !has_v2) {
                        // keep v2, move only v1
                        auto t = v1->position;
                        v1->position = v2->position - halfi * 2;
                        auto d = v1->position - t;
                        v1->arc_center += d;
                    }
                    else if (!has_v1 && has_v2) {
                        // keep v1, move only v2
                        auto t = v2->position;
                        v2->position = v1->position + halfi * 2;
                        auto d = v2->position - t;
                        v2->arc_center += d;
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
            return ToolResponse::commit();
        }
    } break;

    case Mode::JUNCTION: {
        Accumulator<Coordi> acc;
        for (const auto &it : selection) {
            if (it.type == ObjectType::JUNCTION) {
                acc.accumulate(doc.r->get_junction(it.uuid)->position);
            }
        }
        if (auto r = imp->dialogs.ask_datum_coord2("Junction position", acc.get())) {
            auto [c, rc] = *r;
            for (const auto &it : selection) {
                if (it.type == ObjectType::JUNCTION) {
                    if (rc.first)
                        doc.r->get_junction(it.uuid)->position.x = c.x;
                    if (rc.second)
                        doc.r->get_junction(it.uuid)->position.y = c.y;
                }
            }
            return ToolResponse::commit();
        }
    } break;

    case Mode::INVALID:
        break;
    }

    return ToolResponse::end();
}
ToolResponse ToolEnterDatum::update(const ToolArgs &args)
{
    return ToolResponse();
}
} // namespace horizon
