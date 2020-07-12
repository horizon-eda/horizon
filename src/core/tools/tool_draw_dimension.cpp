#include "tool_draw_dimension.hpp"
#include "common/dimension.hpp"
#include "imp/imp_interface.hpp"
#include "pool/part.hpp"
#include <algorithm>
#include "document/idocument.hpp"
#include <gdk/gdkkeysyms.h>
#include <sstream>

namespace horizon {
ToolDrawDimension::ToolDrawDimension(IDocument *c, ToolID tid) : ToolBase(c, tid)
{
}

bool ToolDrawDimension::can_begin()
{
    return doc.r->has_object_type(ObjectType::DIMENSION);
}

ToolResponse ToolDrawDimension::begin(const ToolArgs &args)
{
    temp = doc.r->insert_dimension(UUID::random());
    imp->set_snap_filter({{ObjectType::DIMENSION, temp->uuid}});
    temp->p0 = args.coords;
    temp->p1 = args.coords;
    update_tip();
    return ToolResponse();
}

void ToolDrawDimension::update_tip()
{
    std::vector<ActionLabelInfo> actions;
    actions.reserve(8);

    std::stringstream ss;
    switch (state) {
    case State::P0:
        actions.emplace_back(InToolActionID::LMB, "place first point");
        break;
    case State::P1:
        actions.emplace_back(InToolActionID::LMB, "place second point");
        break;
    case State::LABEL:
        actions.emplace_back(InToolActionID::LMB, "place label");
        break;
    }
    actions.emplace_back(InToolActionID::RMB, "cancel");
    if (state == State::P1 || state == State::LABEL) {
        if (restrict_mode == RestrictMode::ARB) {
            actions.emplace_back(InToolActionID::DIMENSION_MODE);
        }
        if (state == State::P1) {
            actions.emplace_back(InToolActionID::RESTRICT);
            if (restrict_mode != RestrictMode::ARB) {
                actions.emplace_back(InToolActionID::ENTER_DATUM, "enter distance");
            }
        }

        switch (temp->mode) {
        case Dimension::Mode::DISTANCE:
            ss << "Distance";
            break;
        case Dimension::Mode::HORIZONTAL:
            ss << "Horizontal";
            break;
        case Dimension::Mode::VERTICAL:
            ss << "Vertical";
            break;
        }

        if (state == State::P1) {
            ss << " Restrict: " << restrict_mode_to_string();
        }
    }
    imp->tool_bar_set_actions(actions);
    imp->tool_bar_set_tip(ss.str());
}

ToolResponse ToolDrawDimension::update(const ToolArgs &args)
{
    if (args.type == ToolEventType::MOVE) {
        switch (state) {
        case State::P0:
            temp->p0 = args.coords;
            temp->p1 = args.coords;
            break;
        case State::P1:
            temp->p1 = get_coord_restrict(temp->p0, args.coords);
            break;
        case State::LABEL: {
            temp->label_distance = temp->project(args.coords - temp->p0);
        } break;
        }
        return ToolResponse();
    }
    else if (args.type == ToolEventType::ACTION) {
        switch (args.action) {
        case InToolActionID::LMB:
            switch (state) {
            case State::P0:
                state = State::P1;
                break;
            case State::P1:
                state = State::LABEL;
                break;
            case State::LABEL:
                return ToolResponse::commit();
                break;
            }
            update_tip();
            break;

        case InToolActionID::RMB:
        case InToolActionID::CANCEL:
            return ToolResponse::revert();

        case InToolActionID::DIMENSION_MODE:
            if ((state == State::P1 || state == State::LABEL) && restrict_mode == RestrictMode::ARB) {
                switch (temp->mode) {
                case Dimension::Mode::DISTANCE:
                    temp->mode = Dimension::Mode::HORIZONTAL;
                    break;
                case Dimension::Mode::HORIZONTAL:
                    temp->mode = Dimension::Mode::VERTICAL;
                    break;
                case Dimension::Mode::VERTICAL:
                    temp->mode = Dimension::Mode::DISTANCE;
                    break;
                }
                temp->label_distance = temp->project(args.coords - temp->p0);
                update_tip();
            }
            break;

        case InToolActionID::RESTRICT:
            if (state == State::P1) {
                cycle_restrict_mode();
                if (restrict_mode == RestrictMode::X) {
                    temp->mode = Dimension::Mode::HORIZONTAL;
                }
                else if (restrict_mode == RestrictMode::Y) {
                    temp->mode = Dimension::Mode::VERTICAL;
                }
                else if (restrict_mode == RestrictMode::ARB) {
                    temp->mode = Dimension::Mode::DISTANCE;
                }
                temp->p1 = get_coord_restrict(temp->p0, args.coords);
                update_tip();
            }
            break;

        case InToolActionID::ENTER_DATUM:
            if (state == State::P1) {
                if (restrict_mode == RestrictMode::X) {
                    auto dist = temp->p1.x - temp->p0.x;
                    auto dist_sign = dist > 0 ? 1 : -1;
                    auto r = imp->dialogs.ask_datum("Enter distance", std::abs(dist));
                    if (r.first) {
                        temp->p1 = temp->p0 + Coordi(r.second * dist_sign, 0);
                        state = State::LABEL;
                        update_tip();
                    }
                }
                else if (restrict_mode == RestrictMode::Y) {
                    auto dist = temp->p1.y - temp->p0.y;
                    auto dist_sign = dist > 0 ? 1 : -1;
                    auto r = imp->dialogs.ask_datum("Enter distance", std::abs(dist));
                    if (r.first) {
                        temp->p1 = temp->p0 + Coordi(0, r.second * dist_sign);
                        state = State::LABEL;
                        update_tip();
                    }
                }
            }
            break;

        default:;
        }
    }
    return ToolResponse();
}
} // namespace horizon
