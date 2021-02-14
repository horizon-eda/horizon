#include "tool_measure.hpp"
#include "document/idocument.hpp"
#include "imp/imp_interface.hpp"
#include "canvas/canvas_gl.hpp"
#include <sstream>
#include "util/geom_util.hpp"
#include "common/object_descr.hpp"

namespace horizon {


ToolResponse ToolMeasure::begin(const ToolArgs &args)
{
    annotation = imp->get_canvas()->create_annotation();
    annotation->set_visible(true);
    annotation->set_display(LayerDisplay(true, LayerDisplay::Mode::OUTLINE));
    update_tip();
    return ToolResponse();
}


void ToolMeasure::update_tip()
{
    std::vector<ActionLabelInfo> actions;
    actions.reserve(8);

    switch (state) {
    case State::FROM:
    case State::DONE:
        actions.emplace_back(InToolActionID::LMB, "select from point");
        break;
    case State::TO:
        actions.emplace_back(InToolActionID::LMB, "select to point");
        break;
    }
    actions.emplace_back(InToolActionID::RMB, "cancel");
    std::stringstream ss;
    if (state != State::FROM) {
        const auto delta = to - from;
        if (delta.y == 0) {
            ss << "ΔX: " << dim_to_string(delta.x, true);
        }
        else if (delta.x == 0) {
            ss << "ΔY: " << dim_to_string(delta.y, true);
        }
        else {
            ss << coord_to_string(delta, true) << " D:" << dim_to_string(sqrt(delta.mag_sq()), false);
            ss << " α:" << angle_to_string(angle_from_rad(atan2(delta.y, delta.x)), false);
        }
    }

    if (state == State::DONE) {
        ss << " " << s_from << "→" << s_to;
    }

    imp->tool_bar_set_actions(actions);
    imp->tool_bar_set_tip(ss.str());
}


static std::string name_from_target(const Target &trg)
{
    if (!trg.is_valid())
        return "None";

    return object_descriptions.at(trg.type).name;
}

ToolResponse ToolMeasure::update(const ToolArgs &args)
{
    if (args.type == ToolEventType::MOVE) {
        switch (state) {
        case State::FROM:
            from = args.coords;
            break;
        case State::TO:
            to = args.coords;
            annotation->clear();
            {
                const auto v = Coordf(to - from);
                const auto l = sqrt(v.mag_sq());
                const auto vn = (v / l).rotate(M_PI / 2);
                const auto m = vn * l / 20;
                annotation->draw_line(from, to, ColorP::FROM_LAYER, 2);
                for (const auto p : {Coordf(from), Coordf(to)}) {
                    annotation->draw_line(p, p + m, ColorP::FROM_LAYER, 2);
                    annotation->draw_line(p, p - m, ColorP::FROM_LAYER, 2);
                }
            }
            break;
        case State::DONE:;
        }
        update_tip();
        return ToolResponse();
    }
    else if (args.type == ToolEventType::ACTION) {
        switch (args.action) {
        case InToolActionID::LMB:
            switch (state) {
            case State::FROM:
                s_from = name_from_target(args.target);
                state = State::TO;
                break;
            case State::TO:
                s_to = name_from_target(args.target);
                state = State::DONE;
                break;
            case State::DONE:
                state = State::TO;
                from = args.coords;
                s_from = name_from_target(args.target);
                s_to.clear();
                break;
            default:;
            }
            update_tip();
            break;

        case InToolActionID::RMB:
        case InToolActionID::CANCEL:
            return ToolResponse::end();

        default:;
        }
    }
    return ToolResponse();
}

ToolMeasure::~ToolMeasure()
{
    if (annotation) {
        imp->get_canvas()->remove_annotation(annotation);
        annotation = nullptr;
    }
}
} // namespace horizon
