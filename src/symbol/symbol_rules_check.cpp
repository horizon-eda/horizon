#include "pool/symbol.hpp"
#include "rules/cache.hpp"
#include "util/accumulator.hpp"
#include "util/util.hpp"
#include <ctype.h>

namespace horizon {
RulesCheckResult SymbolRules::check_symbol(const Symbol &sym) const
{
    RulesCheckResult r;
    r.level = RulesCheckErrorLevel::PASS;

    if (sym.name.size() == 0) {
        r.errors.emplace_back(RulesCheckErrorLevel::FAIL);
        auto &x = r.errors.back();
        x.comment = "Package has no name";
        x.has_location = false;
    }
    {
        const Text *txt_refdes = nullptr;
        const Text *txt_value = nullptr;
        {
            {
                int refdes_count = 0;
                int value_count = 0;
                for (const auto &[uu, txt] : sym.texts) {
                    if (txt.text == "$REFDES") {
                        refdes_count++;
                        txt_refdes = &txt;
                    }
                    else if (txt.text == "$VALUE") {
                        value_count++;
                        txt_value = &txt;
                    }
                }
                if (refdes_count != 1) {
                    r.errors.emplace_back(RulesCheckErrorLevel::FAIL);
                    auto &x = r.errors.back();
                    x.comment = "Symbol has " + std::to_string(refdes_count) + " reference designators, need 1";
                    x.has_location = false;
                }
                if (value_count != 1) {
                    r.errors.emplace_back(RulesCheckErrorLevel::FAIL);
                    auto &x = r.errors.back();
                    x.comment = "Symbol has " + std::to_string(refdes_count) + " value, need 1";
                    x.has_location = false;
                }
            }
            auto check_text = [&r](const Text *txt, const std::string &name) {
                if (txt) {
                    if (txt->width != 0 || txt->size != 1.5_mm) {
                        r.errors.emplace_back(RulesCheckErrorLevel::FAIL);
                        auto &x = r.errors.back();
                        x.comment = name + " text must have 0 line width and 1.5mm size";
                        x.has_location = false;
                    }
                }
            };
            check_text(txt_refdes, "Refdes");
            check_text(txt_value, "Value");
            if (txt_refdes && txt_value) {
                if (txt_refdes->placement.shift.x != txt_value->placement.shift.x) {
                    r.errors.emplace_back(RulesCheckErrorLevel::WARN);
                    auto &x = r.errors.back();
                    x.comment = "Refdes and value text should be at same X position";
                    x.has_location = false;
                }
                if (txt_refdes->placement.shift.y != -txt_value->placement.shift.y) {
                    r.errors.emplace_back(RulesCheckErrorLevel::WARN);
                    auto &x = r.errors.back();
                    x.comment = "Refdes and value text should be at mirrored Y position";
                    x.has_location = false;
                }
                if (txt_value->placement.shift.y > txt_refdes->placement.shift.y) {
                    r.errors.emplace_back(RulesCheckErrorLevel::WARN);
                    auto &x = r.errors.back();
                    x.comment = "Refdes should be above value";
                    x.has_location = false;
                }
            }
        }
        {
            Coordi bb;
            for (const auto &[uu, junction] : sym.junctions) {
                const auto &p = junction.position;
                bb = Coordi::max(bb, {std::abs(p.x), std::abs(p.y)});
            }
            auto find_junction = [&sym](const Coordi &p) {
                return std::any_of(sym.junctions.begin(), sym.junctions.end(),
                                   [&p](const auto &x) { return x.second.position == p; });
            };
            const bool is_box = find_junction(bb * Coordi(1, 1)) && find_junction(bb * Coordi(1, -1))
                                && find_junction(bb * Coordi(-1, 1)) && find_junction(bb * Coordi(-1, -1));
            if (is_box) {
                r.errors.emplace_back(RulesCheckErrorLevel::PASS, "Is box symbol");
                const int64_t txt_spacing = 1.25_mm;
                if (txt_refdes && txt_refdes->placement.shift.y != bb.y + txt_spacing) {
                    r.errors.emplace_back(RulesCheckErrorLevel::WARN);
                    auto &x = r.errors.back();
                    x.comment = "Refdes should be 1.25mm above symbol box";
                    x.has_location = true;
                    x.location = txt_refdes->placement.shift;
                }
                if (txt_value && txt_value->placement.shift.y != -bb.y - txt_spacing) {
                    r.errors.emplace_back(RulesCheckErrorLevel::WARN);
                    auto &x = r.errors.back();
                    x.comment = "Value should be 1.25mm below symbol box";
                    x.has_location = true;
                    x.location = txt_value->placement.shift;
                }
            }
        }
    }

    {
        std::set<UUID> pins_from_unit;
        for (const auto &[uu, pin] : sym.unit->pins) {
            pins_from_unit.insert(uu);
        }
        Coordi bb;
        const int64_t grid = 1.25_mm;
        std::map<Coordi, std::set<UUID>> pin_pos;
        for (const auto &[uu, pin] : sym.pins) {
            pins_from_unit.erase(uu);
            {
                Coordi pr(pin.position.x % grid, pin.position.y % grid);
                if (pr.x || pr.y) {
                    r.errors.emplace_back(RulesCheckErrorLevel::FAIL);
                    auto &x = r.errors.back();
                    x.comment = "Pin " + sym.unit->pins.at(uu).primary_name
                                + " is off-grid, residual: " + coord_to_string(pr, true);
                    x.has_location = true;
                    x.location = pin.position;
                }
            }
            {
                const auto &p = pin.position;
                bb = Coordi::max(bb, {std::abs(p.x), std::abs(p.y)});
            }
            pin_pos[pin.position].insert(uu);
        }
        for (const auto &[pos, pins] : pin_pos) {
            if (pins.size() > 1) {
                r.errors.emplace_back(RulesCheckErrorLevel::FAIL);
                auto &x = r.errors.back();
                x.comment = "Overlapping pins: ";
                for (const auto pin : pins) {
                    x.comment += sym.unit->pins.at(pin).primary_name + ", ";
                }
                x.comment.pop_back();
                x.comment.pop_back();
                x.has_location = true;
                x.location = pos;
            }
        }

        for (const auto &[uu, pin] : sym.pins) {
            std::set<Orientation> orientations;
            const auto p = pin.position;
            if (p.x == bb.x && p.y == bb.y) { // top right
                orientations.insert(Orientation::RIGHT);
                orientations.insert(Orientation::UP);
            }
            else if (p.x == bb.x && p.y == -bb.y) { // bottom right
                orientations.insert(Orientation::RIGHT);
                orientations.insert(Orientation::DOWN);
            }
            else if (p.x == -bb.x && p.y == bb.y) { // top left
                orientations.insert(Orientation::LEFT);
                orientations.insert(Orientation::UP);
            }
            else if (p.x == -bb.x && p.y == -bb.y) { // bottom left
                orientations.insert(Orientation::LEFT);
                orientations.insert(Orientation::DOWN);
            }
            else if (p.x == bb.x) { // right
                orientations.insert(Orientation::RIGHT);
            }
            else if (p.x == -bb.x) { // left
                orientations.insert(Orientation::LEFT);
            }
            else if (p.y == bb.y) { // top
                orientations.insert(Orientation::UP);
            }
            else if (p.y == -bb.y) { // bottom
                orientations.insert(Orientation::DOWN);
            }
            else {
                r.errors.emplace_back(RulesCheckErrorLevel::WARN);
                auto &x = r.errors.back();
                x.comment = "Pin " + sym.unit->pins.at(uu).primary_name + " is not on bounding box";
                x.has_location = true;
                x.location = p;
            }
            if (orientations.size()) {
                if (!orientations.count(pin.orientation)) {
                    r.errors.emplace_back(RulesCheckErrorLevel::WARN);
                    auto &x = r.errors.back();
                    x.comment = "Pin " + sym.unit->pins.at(uu).primary_name + " has improper orientation";
                    x.has_location = true;
                    x.location = p;
                }
            }
        }

        for (const auto &uu : pins_from_unit) {
            r.errors.emplace_back(RulesCheckErrorLevel::FAIL);
            auto &x = r.errors.back();
            x.comment = "Pin " + sym.unit->pins.at(uu).primary_name + " is not placed";
            x.has_location = false;
        }
    }

    r.update();
    return r;
}

RulesCheckResult SymbolRules::check(RuleID id, const Symbol &sym) const
{
    switch (id) {
    case RuleID::SYMBOL_CHECKS:
        return check_symbol(sym);

    default:
        return RulesCheckResult();
    }
}
} // namespace horizon
