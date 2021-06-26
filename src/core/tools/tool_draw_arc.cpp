#include "tool_draw_arc.hpp"
#include "imp/imp_interface.hpp"
#include "document/idocument.hpp"
#include "common/arc.hpp"
#include "canvas/canvas_gl.hpp"

namespace horizon {

void ToolDrawArc::apply_settings()
{
    if (temp_arc)
        temp_arc->width = settings.width;
}

bool ToolDrawArc::can_begin()
{
    return doc.r->has_object_type(ObjectType::ARC);
}

Junction *ToolDrawArc::make_junction(const Coordd &coords)
{
    Junction *ju;
    ju = doc.r->insert_junction(UUID::random());
    imp->set_snap_filter({{ObjectType::JUNCTION, ju->uuid}});
    ju->position.x = coords.x;
    ju->position.y = coords.y;
    return ju;
}

ToolResponse ToolDrawArc::begin(const ToolArgs &args)
{
    temp_junc = make_junction(args.coords);
    temp_arc = nullptr;
    from_junc = nullptr;
    to_junc = nullptr;
    state = State::CENTER_START;
    annotation = imp->get_canvas()->create_annotation();
    annotation->set_visible(true);
    annotation->set_display(LayerDisplay(true, LayerDisplay::Mode::OUTLINE));
    update_tip();
    return ToolResponse();
}

ToolDrawArc::~ToolDrawArc()
{
    if (annotation) {
        imp->get_canvas()->remove_annotation(annotation);
        annotation = nullptr;
    }
}

void ToolDrawArc::update_tip()
{
    std::vector<ActionLabelInfo> actions;
    actions.reserve(8);
    switch (state) {
    case State::FROM:
        actions.emplace_back(InToolActionID::LMB, "place from junction");
        break;

    case State::TO:
        actions.emplace_back(InToolActionID::LMB, "place to junction");
        break;

    case State::CENTER:
        actions.emplace_back(InToolActionID::LMB, "place center junction");
        break;

    case State::CENTER_START:
        actions.emplace_back(InToolActionID::LMB, "place center");
        break;

    case State::RADIUS:
        actions.emplace_back(InToolActionID::LMB, "set radius");
        break;

    case State::START_ANGLE:
        actions.emplace_back(InToolActionID::LMB, "set start angle");
        break;

    case State::END_ANGLE:
        actions.emplace_back(InToolActionID::LMB, "set end angle");
        break;
    }

    actions.emplace_back(InToolActionID::RMB, "cancel");
    if (state == State::CENTER || state == State::END_ANGLE)
        actions.emplace_back(InToolActionID::FLIP_ARC);

    if (state == State::FROM || state == State::CENTER_START)
        actions.emplace_back(InToolActionID::ARC_MODE, "switch mode");
    actions.emplace_back(InToolActionID::ENTER_WIDTH, "line width");
    imp->tool_bar_set_actions(actions);
}

void ToolDrawArc::set_radius_angle(double r, double a, double b)
{
    const auto c = Coordd(temp_arc->center->position);
    temp_arc->from->position = (c + Coordd::euler(r, a)).to_coordi();
    temp_arc->to->position = (c + Coordd::euler(r, b)).to_coordi();
}

void ToolDrawArc::update_end_angle(const Coordi &c)
{
    const auto d = c - temp_arc->center->position;
    const auto end_angle = atan2(d.y, d.x);
    if (flipped)
        set_radius_angle(radius, end_angle, start_angle);
    else
        set_radius_angle(radius, start_angle, end_angle);
}

ToolResponse ToolDrawArc::update(const ToolArgs &args)
{
    if (args.type == ToolEventType::MOVE) {
        if (state == State::RADIUS) {
            radius = (args.coords - temp_arc->center->position).magd();
            set_radius_angle(radius, 1e-3, 0);
            annotation->clear();
            annotation->draw_line(temp_arc->center->position, args.coords, ColorP::FROM_LAYER, 0);
        }
        else if (state == State::START_ANGLE) {
            annotation->clear();
            const Coordf c = temp_arc->center->position;
            const auto d = Coordf(args.coords) - c;
            const auto a = atan2(d.y, d.x);
            annotation->draw_line(c, c + Coordf::euler(radius, a), ColorP::FROM_LAYER, 0);
        }
        else if (state == State::END_ANGLE) {
            update_end_angle(args.coords);
            annotation->clear();
            annotation->draw_line(temp_arc->center->position, temp_arc->from->position, ColorP::FROM_LAYER, 0);
            annotation->draw_line(temp_arc->center->position, temp_arc->to->position, ColorP::FROM_LAYER, 0);
        }
        else if (temp_junc) {
            temp_junc->position = args.coords;
        }
    }
    else if (args.type == ToolEventType::ACTION) {
        switch (args.action) {
        case InToolActionID::LMB:
            switch (state) {
            case State::FROM:
                if (args.target.type == ObjectType::JUNCTION) {
                    from_junc = doc.r->get_junction(args.target.path.at(0));
                }
                else {
                    from_junc = temp_junc;
                    temp_junc = make_junction(args.coords);
                }
                state = State::TO;
                break;

            case State::TO:
                if (args.target.type == ObjectType::JUNCTION) {
                    to_junc = doc.r->get_junction(args.target.path.at(0));
                }
                else {
                    to_junc = temp_junc;
                    temp_junc = make_junction(args.coords);
                }
                temp_arc = doc.r->insert_arc(UUID::random());
                apply_settings();
                temp_arc->from = from_junc;
                temp_arc->to = to_junc;
                temp_arc->center = temp_junc;
                temp_arc->layer = args.work_layer;
                state = State::CENTER;
                break;

            case State::CENTER:
                if (args.target.type == ObjectType::JUNCTION) {
                    temp_arc->center = doc.r->get_junction(args.target.path.at(0));
                    doc.r->delete_junction(temp_junc->uuid);
                    temp_junc = nullptr;
                }
                else {
                    temp_arc->center = temp_junc;
                }
                return ToolResponse::commit();
                break;


            case State::CENTER_START:
                temp_arc = doc.r->insert_arc(UUID::random());
                apply_settings();
                temp_arc->from = make_junction(temp_junc->position);
                temp_arc->to = make_junction(temp_junc->position);
                temp_arc->center = temp_junc;
                temp_arc->layer = args.work_layer;
                state = State::RADIUS;
                temp_junc = nullptr;
                break;

            case State::RADIUS:
                state = State::START_ANGLE;
                break;

            case State::START_ANGLE: {
                const auto d = args.coords - temp_arc->center->position;
                start_angle = atan2(d.y, d.x);
                state = State::END_ANGLE;
            } break;

            case State::END_ANGLE:
                return ToolResponse::commit();
                break;
            }
            break;

        case InToolActionID::RMB:
        case InToolActionID::CANCEL:
            return ToolResponse::revert();

        case InToolActionID::ENTER_WIDTH:
            ask_line_width();
            break;

        case InToolActionID::FLIP_ARC:
            if (temp_arc) {
                if (state == State::END_ANGLE) {
                    flipped = !flipped;
                    update_end_angle(args.coords);
                }
                else {
                    temp_arc->reverse();
                }
            }
            break;

        case InToolActionID::ARC_MODE:
            if (state == State::FROM)
                state = State::CENTER_START;
            else if (state == State::CENTER_START)
                state = State::FROM;
            break;

        default:;
        }
    }
    else if (args.type == ToolEventType::LAYER_CHANGE) {
        if (temp_arc)
            temp_arc->layer = args.work_layer;
    }
    update_tip();
    return ToolResponse();
}
} // namespace horizon
