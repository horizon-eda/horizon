#include <iostream>
#include "tool_drag_round.hpp"
#include "document/idocument_board.hpp"
#include "board/board.hpp"
#include "imp/imp_interface.hpp"
#include "util/selection_util.hpp"
#include "util/geom_util.hpp"

namespace horizon {

bool ToolDragRound::can_begin()
{
    size_t junctions = 0;

    if (!doc.b)
        return false;

    //return sel_has_type(selection, ObjectType::JUNCTION);
    for (const auto &it : selection) {
        if (it.type != ObjectType::JUNCTION)
            continue;

        auto ju = &doc.b->get_board()->junctions.at(it.uuid);

        if (ju->connected_lines.size() || ju->connected_arcs.size())
            continue;
        if (ju->connected_vias.size() || ju->connected_connection_lines.size()
                || ju->connected_net_ties.size())
            continue;

        if (ju->connected_tracks.size() != 2)
            continue;

        junctions++;
    }
    return junctions > 0;
}

static double angle_wrap(double angle)
{
    double wrapped;

    wrapped = fmod(angle, 2 * M_PI);
    if (wrapped < 0)
        wrapped += 2 * M_PI;
    return wrapped;
}

static Coordi angle_to_coord(double angle, float length)
{
    Coordf base = { length, 0.0 };

    base = base.rotate(angle);
    return Coordi(base.x * 1e6, base.y * 1e6);
}


static Track::Connection &other(Track *track, BoardJunction *ju)
{
    if (track->from.junc == ju)
        return track->to;
    else
        return track->from;
}

/* TODO: move into constructor
 * I had originally used references rather than pointers, but then the compiler
 * says you need to initialize all references :(
 */
ToolDragRound::RoundInfo ToolDragRound::RoundInfo::setup(Board *board, Track *first, Track *second, BoardJunction *junc)
{
    double angle_first = (other(first, junc).get_position() - junc->position).angle();
    double angle_second = (other(second, junc).get_position() - junc->position).angle();
    double axis, d, inv_cosine;
    bool direction;

    /* determine angle to move in.  180° corner case is already excluded when hitting this */
    direction = angle_wrap(angle_first - angle_second) < angle_wrap(angle_second - angle_first);
    if (direction) {
        d = angle_wrap(angle_first - angle_second) / 2.0;
        axis = angle_wrap(angle_second + d);
    } else {
        d = angle_wrap(angle_second - angle_first) / 2.0;
        axis = angle_wrap(angle_first + d);
    }
    inv_cosine = 1. / cos(d);

#ifdef DEBUG
    std::cout << "round-corner tool\n";
    std::cout << "first:  " << angle_to_string(angle_from_rad(angle_first)) << "\n";
    std::cout << "second: " << angle_to_string(angle_from_rad(angle_second)) << "\n";
    std::cout << "axis:   " << angle_to_string(angle_from_rad(axis)) << "\n";
    std::cout << "dir:    " << direction << "\n";
#endif

    /* create new junction on second track */
    auto newjuuid = horizon::UUID::random();
    auto newjunc = &board->junctions.emplace(newjuuid, newjuuid).first->second;

    newjunc->net = junc->net;
    newjunc->layer = junc->layer;
    newjunc->position = junc->position;

    if (second->from.junc == junc)
        second->from = { newjunc };
    else
        second->to = { newjunc };
    second->update_refs(*board); // not sure if needed?

    /* create new track for the arc */
    auto arcuuid = horizon::UUID::random();
    auto arc = &board->tracks.emplace(arcuuid, arcuuid).first->second;

    arc->net = first->net;
    arc->layer = first->layer;
    arc->width = first->width;
    arc->width_from_rules = first->width_from_rules;

    /* make arc go the right way around... */
    if (direction) {
        arc->from = { junc };
        arc->to = { newjunc };
    } else {
        arc->from = { newjunc };
        arc->to = { junc };
    }

    arc->update_refs(*board); // not sure if needed, again?

    return {
        first, second, arc,
        junc, newjunc,
        junc->position,
        angle_first, angle_second,
        axis, inv_cosine,
    };
}

void ToolDragRound::RoundInfo::apply(double distance) const
{
    distance += offset;

    junc->position = orig + angle_to_coord(angle_first, distance);
    newjunc->position = orig + angle_to_coord(angle_second, distance);

    if (distance < 0.01)
        arc->center.reset();
    else
        arc->center = orig + angle_to_coord(axis, distance * inv_cosine);
}

ToolResponse ToolDragRound::begin(const ToolArgs &args)
{
    auto board = doc.b->get_board();

    selection.clear();
    for (const auto &it : args.selection) {
        if (it.type != ObjectType::JUNCTION)
            continue;

        auto ju = &board->junctions.at(it.uuid);

        if (ju->connected_lines.size()
                || ju->connected_arcs.size()
                || ju->connected_vias.size()
                || ju->connected_connection_lines.size()
                || ju->connected_net_ties.size())
            continue;

        if (ju->connected_tracks.size() != 2)
            continue;

        auto first = &board->tracks.at(ju->connected_tracks.at(0));
        auto second = &board->tracks.at(ju->connected_tracks.at(1));

        if (first->is_parallel_to(*second))
            continue;
        if (first->get_length() == 0 || second->get_length() == 0)
            continue;

        auto ri = RoundInfo::setup(board, first, second, ju);

        /* FIXME: make this less dumb (n^2 scaling currently) */
        for (auto &it : round_info) {
            double angle_axis = angle_wrap(it.axis - ri.axis);

            if (angle_axis > M_PI)
                angle_axis -= 2 * M_PI;

            /* 180° is NOT acceptable here, it means the bend is going the
             * other direction!
             */
            if (fabs(angle_axis) > 0.01)
                continue;

            double angle_origins = angle_wrap((it.orig - ri.orig).angle() - ri.axis);
            double lim = fabs(sin(angle_origins));

            /* using 0.12 here allows "roughly aligned" things that were routed
             * on a normal track but without going to the design rule minimum
             * distance to be bent as a group
             */
            /* 180° IS acceptable here (hence using sin()), it reflects which
             * of the two points is closer to the "bend center" and is handled
             * below
             */
            if (lim > 0.12) {
#ifdef DEBUG
                std::cout << "angle & origin no match " << lim << "\n";
                std::cout << "a-a: " << angle_to_string(angle_from_rad(angle_axis)) << "\n";
                std::cout << "o-a: " << angle_to_string(angle_from_rad(angle_origins)) << "\n";
#endif
                continue;
            }

            /* direction? */
            if (angle_origins > 0.5 * M_PI && angle_origins < 1.5 * M_PI) {
                it.offset = std::max(it.offset, ri.offset + (it.orig - ri.orig).magd() / 1e6 / it.inv_cosine);
                it.apply(0.0);
            } else {
                ri.offset = std::max(ri.offset, it.offset + (it.orig - ri.orig).magd() / 1e6 / it.inv_cosine);
                ri.apply(0.0);
            }
        }

        round_info.push_back(std::move(ri));
        selection.emplace(it);
        selection.emplace(ri.newjunc->uuid, ObjectType::JUNCTION);
        selection.emplace(ri.arc->uuid, ObjectType::TRACK);

        nets.insert(ju->net->uuid);
    }
    pos_orig = args.coords;

    if (selection.size() < 1)
        return ToolResponse::end();

    imp->tool_bar_set_actions({
            {InToolActionID::LMB, "place"},
            {InToolActionID::RMB, "cancel"},
    });

    return ToolResponse();
}

ToolResponse ToolDragRound::update(const ToolArgs &args)
{
    if (args.type == ToolEventType::MOVE) {
        /* double distance = (args.coords - pos_orig).magd() / 1e6;
         * ^ really poor UX because if you go too far you need to
         * "search" for your origin point which is not visible :(
         *
         * use positive X direction in lieu of better ideas...
         */
        double distance = (args.coords.x - pos_orig.x) / 1e6;

        if (distance < 0)
            distance = 0;

        for (const auto &it : round_info) {
            it.apply(distance);
        }
        doc.b->get_board()->update_airwires(true, nets);
    }
    else if (args.type == ToolEventType::ACTION) {
        switch (args.action) {
        case InToolActionID::LMB: {
            doc.b->get_board()->update_airwires(false, nets);
            return ToolResponse::commit();
        }

        case InToolActionID::LMB_RELEASE:
            if (is_transient) {
                doc.b->get_board()->update_airwires(false, nets);
                return ToolResponse::commit();
            }
            break;

        case InToolActionID::RMB:
        case InToolActionID::CANCEL:
            return ToolResponse::revert();

        default:;
        }
    }
    return ToolResponse();
}
} // namespace horizon
