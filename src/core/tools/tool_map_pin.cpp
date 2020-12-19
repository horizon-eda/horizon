#include "tool_map_pin.hpp"
#include "document/idocument_symbol.hpp"
#include "pool/symbol.hpp"
#include "imp/imp_interface.hpp"
#include "tool_helper_move.hpp"
#include "util/util.hpp"
#include "canvas/canvas_gl.hpp"

namespace horizon {

bool ToolMapPin::can_begin()
{
    return doc.y;
}

void ToolMapPin::create_pin(const UUID &uu)
{
    auto orientation = Orientation::RIGHT;
    if (pin) {
        orientation = pin->orientation;
    }
    pin_last2 = pin_last;
    pin_last = pin;

    pin = &doc.y->insert_symbol_pin(uu);
    pin->length = 2.5_mm;
    pin->name = doc.y->get_symbol().unit->pins.at(uu).primary_name;
    pin->orientation = orientation;
    update_annotation();
}

ToolResponse ToolMapPin::begin(const ToolArgs &args)
{
    bool from_sidebar = false;
    for (const auto &it : args.selection) {
        if (it.type == ObjectType::SYMBOL_PIN) {
            if (doc.y->get_symbol().pins.count(it.uuid) == 0) { // unplaced pin
                pins.emplace_back(&doc.y->get_symbol().unit->pins.at(it.uuid), false);
                from_sidebar = true;
            }
        }
    }
    if (pins.size() == 0) {
        for (const auto &it : doc.y->get_symbol().unit->pins) {
            pins.emplace_back(&it.second, false);
        }
    }
    std::sort(pins.begin(), pins.end(), [](const auto &a, const auto &b) {
        return strcmp_natural(a.first->primary_name, b.first->primary_name) < 0;
    });

    for (auto &it : pins) {
        if (doc.y->get_symbol().pins.count(it.first->uuid)) {
            it.second = true;
        }
    }
    auto pins_unplaced = std::count_if(pins.begin(), pins.end(), [](const auto &a) { return a.second == false; });
    if (pins_unplaced == 0) {
        imp->tool_bar_flash("No pins left to place");
        return ToolResponse::end();
    }

    UUID selected_pin;
    if (pins_unplaced > 1 && from_sidebar == false) {
        if (auto r = imp->dialogs.map_pin(pins)) {
            selected_pin = *r;
        }
        else {
            return ToolResponse::end();
        }
    }
    else {
        selected_pin =
                std::find_if(pins.begin(), pins.end(), [](const auto &a) { return a.second == false; })->first->uuid;
    }


    auto x = std::find_if(pins.begin(), pins.end(),
                          [selected_pin](const auto &a) { return a.first->uuid == selected_pin; });
    assert(x != pins.end());
    pin_index = x - pins.begin();

    annotation = imp->get_canvas()->create_annotation();
    annotation->set_visible(true);
    annotation->set_display(LayerDisplay(true, LayerDisplay::Mode::OUTLINE));

    create_pin(selected_pin);
    pin->position = args.coords;
    update_tip();

    selection.clear();

    return ToolResponse();
}

bool ToolMapPin::can_autoplace() const
{
    if (pin_last && pin_last2)
        return pin_last2->orientation == pin_last->orientation;
    else
        return false;
}

void ToolMapPin::update_annotation()
{
    if (!annotation)
        return;
    annotation->clear();
    if (can_autoplace()) {
        auto shift = pin_last->position - pin_last2->position;
        Coordf center = pin_last->position + shift;
        float size = 0.25_mm;
        std::vector<Coordf> pts = {center + Coordf(-size, size), center + Coordf(size, size),
                                   center + Coordf(size, -size), center + Coordf(-size, -size)};
        for (size_t i = 0; i < pts.size(); i++) {
            const auto &p0 = pts.at(i);
            const auto &p2 = pts.at((i + 1) % pts.size());
            annotation->draw_line(p0, p2, ColorP::PIN_HIDDEN, 0);
        }

        Placement pl;
        pl.set_angle(orientation_to_angle(pin_last->orientation));
        Coordf p = pl.transform(Coordf(pin_last->length, 0));
        annotation->draw_line(center, center - p, ColorP::PIN_HIDDEN, 0);
    }
}

void ToolMapPin::update_tip()
{

    std::vector<ActionLabelInfo> actions;
    actions.reserve(8);
    actions.emplace_back(InToolActionID::LMB);
    actions.emplace_back(InToolActionID::RMB);
    actions.emplace_back(InToolActionID::ROTATE);
    actions.emplace_back(InToolActionID::MIRROR);
    actions.emplace_back(InToolActionID::EDIT, "select pin");
    if (can_autoplace()) {
        actions.emplace_back(InToolActionID::AUTOPLACE_NEXT_PIN);
        actions.emplace_back(InToolActionID::AUTOPLACE_ALL_PINS);
    }

    imp->tool_bar_set_actions(actions);
}

ToolResponse ToolMapPin::update(const ToolArgs &args)
{
    if (args.type == ToolEventType::MOVE) {
        pin->position = args.coords;
    }
    else if (args.type == ToolEventType::ACTION) {
        switch (args.action) {
        case InToolActionID::LMB:
            pins.at(pin_index).second = true;
            pin_index++;
            while (pin_index < pins.size()) {
                if (pins.at(pin_index).second == false)
                    break;
                pin_index++;
            }
            if (pin_index == pins.size()) {
                return ToolResponse::commit();
            }
            create_pin(pins.at(pin_index).first->uuid);
            pin->position = args.coords;
            break;

        case InToolActionID::RMB:
        case InToolActionID::CANCEL:
            if (pin) {
                doc.y->get_symbol().pins.erase(pin->uuid);
            }
            return ToolResponse::commit();

        case InToolActionID::AUTOPLACE_ALL_PINS:
        case InToolActionID::AUTOPLACE_NEXT_PIN:
            if (can_autoplace()) {
                auto shift = pin_last->position - pin_last2->position;
                while (1) {
                    pin->position = pin_last->position + shift;

                    pins.at(pin_index).second = true;
                    pin_index++;
                    while (pin_index < pins.size()) {
                        if (pins.at(pin_index).second == false)
                            break;
                        pin_index++;
                    }
                    if (pin_index == pins.size()) {
                        return ToolResponse::commit();
                    }
                    create_pin(pins.at(pin_index).first->uuid);
                    pin->position = args.coords;

                    if (args.action == InToolActionID::AUTOPLACE_NEXT_PIN)
                        break;
                }
            }
            break;

        case InToolActionID::EDIT: {
            if (auto r = imp->dialogs.map_pin(pins)) {
                UUID selected_pin = *r;
                doc.y->get_symbol().pins.erase(pin->uuid);

                auto x = std::find_if(pins.begin(), pins.end(),
                                      [selected_pin](const auto &a) { return a.first->uuid == selected_pin; });
                assert(x != pins.end());
                pin_index = x - pins.begin();

                auto p1 = pin_last;
                auto p2 = pin_last2;
                create_pin(selected_pin);
                pin->position = args.coords;
                pin_last2 = p2;
                pin_last = p1;
            }
        } break;

        case InToolActionID::ROTATE:
        case InToolActionID::MIRROR:
            pin->orientation =
                    ToolHelperMove::transform_orientation(pin->orientation, args.action == InToolActionID::ROTATE);
            break;

        default:;
        }
    }
    update_tip();
    return ToolResponse();
}

ToolMapPin::~ToolMapPin()
{
    if (annotation) {
        imp->get_canvas()->remove_annotation(annotation);
        annotation = nullptr;
    }
}

} // namespace horizon
