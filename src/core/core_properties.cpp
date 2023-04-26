#include "core_properties.hpp"
#include "common/dimension.hpp"
#include "common/hole.hpp"
#include "common/layer_provider.hpp"
#include "common/polygon.hpp"
#include "common/picture.hpp"
#include "common/arc.hpp"
#include "common/text.hpp"
#include "common/line.hpp"
#include "common/keepout.hpp"
#include "core.hpp"
#include "util/util.hpp"
#include <assert.h>

namespace horizon {
#define HANDLED                                                                                                        \
    if (handled) {                                                                                                     \
        *handled = true;                                                                                               \
    }
#define NOT_HANDLED                                                                                                    \
    if (handled) {                                                                                                     \
        *handled = false;                                                                                              \
    }

bool Core::get_property(ObjectType type, const UUID &uu, ObjectProperty::ID property, PropertyValue &value)
{
    switch (type) {
    case ObjectType::HOLE: {
        auto hole = get_hole(uu);
        switch (property) {
        case ObjectProperty::ID::PLATED:
            dynamic_cast<PropertyValueBool &>(value).value = hole->plated;
            return true;

        case ObjectProperty::ID::DIAMETER:
            dynamic_cast<PropertyValueInt &>(value).value = hole->diameter;
            return true;

        case ObjectProperty::ID::LENGTH:
            dynamic_cast<PropertyValueInt &>(value).value = hole->length;
            return true;

        case ObjectProperty::ID::SHAPE:
            dynamic_cast<PropertyValueInt &>(value).value = static_cast<int>(hole->shape);
            return true;

        case ObjectProperty::ID::PARAMETER_CLASS:
            dynamic_cast<PropertyValueString &>(value).value = hole->parameter_class;
            return true;

        case ObjectProperty::ID::SPAN:
            dynamic_cast<PropertyValueLayerRange &>(value).value = hole->span;
            return true;

        case ObjectProperty::ID::POSITION_X:
        case ObjectProperty::ID::POSITION_Y:
        case ObjectProperty::ID::ANGLE:
            get_placement(hole->placement, value, property);
            return true;

        default:
            return false;
        }
    } break;

    case ObjectType::LINE: {
        auto line = get_line(uu);
        switch (property) {
        case ObjectProperty::ID::WIDTH:
            dynamic_cast<PropertyValueInt &>(value).value = line->width;
            return true;

        case ObjectProperty::ID::LAYER:
            dynamic_cast<PropertyValueInt &>(value).value = line->layer;
            return true;

        default:
            return false;
        }
    } break;

    case ObjectType::ARC: {
        auto arc = get_arc(uu);
        switch (property) {
        case ObjectProperty::ID::WIDTH:
            dynamic_cast<PropertyValueInt &>(value).value = arc->width;
            return true;

        case ObjectProperty::ID::LAYER:
            dynamic_cast<PropertyValueInt &>(value).value = arc->layer;
            return true;

        default:
            return false;
        }
    } break;

    case ObjectType::TEXT: {
        auto text = get_text(uu);
        switch (property) {
        case ObjectProperty::ID::WIDTH:
            dynamic_cast<PropertyValueInt &>(value).value = text->width;
            return true;

        case ObjectProperty::ID::SIZE:
            dynamic_cast<PropertyValueInt &>(value).value = text->size;
            return true;

        case ObjectProperty::ID::LAYER:
            dynamic_cast<PropertyValueInt &>(value).value = text->layer;
            return true;

        case ObjectProperty::ID::TEXT:
            dynamic_cast<PropertyValueString &>(value).value = text->text;
            return true;

        case ObjectProperty::ID::POSITION_X:
        case ObjectProperty::ID::POSITION_Y:
        case ObjectProperty::ID::MIRROR:
            get_placement(text->placement, value, property);
            return true;

        case ObjectProperty::ID::ANGLE:
            if (text->placement.mirror) {
                Placement pl = text->placement;
                pl.invert_angle();
                pl.inc_angle_deg(180);
                dynamic_cast<PropertyValueInt &>(value).value = pl.get_angle();
            }
            else {
                dynamic_cast<PropertyValueInt &>(value).value = text->placement.get_angle();
            }
            return true;

        case ObjectProperty::ID::FONT:
            dynamic_cast<PropertyValueInt &>(value).value = static_cast<int>(text->font);
            return true;

        case ObjectProperty::ID::ALLOW_UPSIDE_DOWN:
            dynamic_cast<PropertyValueBool &>(value).value = text->allow_upside_down;
            return true;

        default:
            return false;
        }
    } break;

    case ObjectType::DIMENSION: {
        auto dim = get_dimension(uu);
        switch (property) {
        case ObjectProperty::ID::SIZE:
            dynamic_cast<PropertyValueInt &>(value).value = dim->label_size;
            return true;

        case ObjectProperty::ID::MODE:
            dynamic_cast<PropertyValueInt &>(value).value = static_cast<int>(dim->mode);
            return true;

        default:
            return false;
        }
    } break;

    case ObjectType::POLYGON: {
        auto poly = get_polygon(uu);
        switch (property) {
        case ObjectProperty::ID::LAYER:
            dynamic_cast<PropertyValueInt &>(value).value = poly->layer;
            return true;

        case ObjectProperty::ID::PARAMETER_CLASS:
            dynamic_cast<PropertyValueString &>(value).value = poly->parameter_class;
            return true;

        case ObjectProperty::ID::USAGE: {
            std::string usage;
            if (poly->usage) {
                auto ty = poly->usage->get_type();
                if (ty == PolygonUsage::Type::PLANE) {
                    usage = "Plane";
                }
                else if (ty == PolygonUsage::Type::KEEPOUT) {
                    usage = "Keepout";
                }
                else {
                    usage = "Invalid";
                }
            }
            else {
                usage = "None";
            }
            dynamic_cast<PropertyValueString &>(value).value = usage;
            return true;
        } break;

        default:
            return false;
        }
    } break;

    case ObjectType::KEEPOUT: {
        auto keepout = get_keepout(uu);
        switch (property) {
        case ObjectProperty::ID::KEEPOUT_CLASS:
            dynamic_cast<PropertyValueString &>(value).value = keepout->keepout_class;
            return true;

        default:
            return false;
        }
    } break;

    case ObjectType::PICTURE: {
        auto pic = get_picture(uu);
        switch (property) {
        case ObjectProperty::ID::SIZE:
            dynamic_cast<PropertyValueInt &>(value).value = pic->px_size;
            return true;

        case ObjectProperty::ID::ON_TOP:
            dynamic_cast<PropertyValueBool &>(value).value = pic->on_top;
            return true;

        case ObjectProperty::ID::OPACITY:
            dynamic_cast<PropertyValueDouble &>(value).value = pic->opacity;
            return true;

        case ObjectProperty::ID::POSITION_X:
        case ObjectProperty::ID::POSITION_Y:
        case ObjectProperty::ID::ANGLE:
            get_placement(pic->placement, value, property);
            return true;

        default:
            return false;
        }
    } break;

    default:
        return false;
    }
    return false;
}

bool Core::set_property(ObjectType type, const UUID &uu, ObjectProperty::ID property, const PropertyValue &value)
{
    switch (type) {
    case ObjectType::HOLE: {
        auto hole = get_hole(uu);
        switch (property) {
        case ObjectProperty::ID::PLATED:
            hole->plated = dynamic_cast<const PropertyValueBool &>(value).value;
            break;

        case ObjectProperty::ID::DIAMETER:
            hole->diameter = dynamic_cast<const PropertyValueInt &>(value).value;
            break;

        case ObjectProperty::ID::LENGTH:
            if (hole->shape != Hole::Shape::SLOT)
                return false;
            hole->length = dynamic_cast<const PropertyValueInt &>(value).value;
            break;

        case ObjectProperty::ID::SHAPE:
            hole->shape = static_cast<Hole::Shape>(dynamic_cast<const PropertyValueInt &>(value).value);
            break;

        case ObjectProperty::ID::PARAMETER_CLASS:
            hole->parameter_class = dynamic_cast<const PropertyValueString &>(value).value;
            break;

        case ObjectProperty::ID::SPAN:
            hole->span = dynamic_cast<const PropertyValueLayerRange &>(value).value;
            break;

        case ObjectProperty::ID::POSITION_X:
        case ObjectProperty::ID::POSITION_Y:
        case ObjectProperty::ID::ANGLE:
            set_placement(hole->placement, value, property);
            break;

        default:
            return false;
        }
    } break;

    case ObjectType::LINE: {
        auto line = get_line(uu);
        switch (property) {
        case ObjectProperty::ID::WIDTH:
            line->width = dynamic_cast<const PropertyValueInt &>(value).value;
            break;

        case ObjectProperty::ID::LAYER:
            line->layer = dynamic_cast<const PropertyValueInt &>(value).value;
            break;

        default:
            return false;
        }
    } break;

    case ObjectType::ARC: {
        auto arc = get_arc(uu);
        switch (property) {
        case ObjectProperty::ID::WIDTH:
            arc->width = dynamic_cast<const PropertyValueInt &>(value).value;
            break;

        case ObjectProperty::ID::LAYER:
            arc->layer = dynamic_cast<const PropertyValueInt &>(value).value;
            break;

        default:
            return false;
        }
    } break;

    case ObjectType::TEXT: {
        auto text = get_text(uu);
        switch (property) {
        case ObjectProperty::ID::WIDTH:
            text->width = dynamic_cast<const PropertyValueInt &>(value).value;
            break;

        case ObjectProperty::ID::SIZE:
            text->size = dynamic_cast<const PropertyValueInt &>(value).value;
            break;

        case ObjectProperty::ID::LAYER:
            text->layer = dynamic_cast<const PropertyValueInt &>(value).value;
            break;

        case ObjectProperty::ID::TEXT:
            text->text = dynamic_cast<const PropertyValueString &>(value).value;
            break;

        case ObjectProperty::ID::POSITION_X:
        case ObjectProperty::ID::POSITION_Y:
        case ObjectProperty::ID::MIRROR:
            set_placement(text->placement, value, property);
            break;

        case ObjectProperty::ID::ANGLE:
            text->placement.set_angle(dynamic_cast<const PropertyValueInt &>(value).value);
            if (text->placement.mirror) {
                text->placement.invert_angle();
                text->placement.inc_angle_deg(180);
            }
            break;

        case ObjectProperty::ID::FONT:
            text->font = static_cast<TextData::Font>(dynamic_cast<const PropertyValueInt &>(value).value);
            break;

        case ObjectProperty::ID::ALLOW_UPSIDE_DOWN:
            text->allow_upside_down = dynamic_cast<const PropertyValueBool &>(value).value;
            break;

        default:
            return false;
        }
    } break;

    case ObjectType::DIMENSION: {
        auto dim = get_dimension(uu);
        switch (property) {
        case ObjectProperty::ID::SIZE:
            dim->label_size = dynamic_cast<const PropertyValueInt &>(value).value;
            break;

        case ObjectProperty::ID::MODE:
            dim->mode = static_cast<Dimension::Mode>(dynamic_cast<const PropertyValueInt &>(value).value);
            break;

        default:
            return false;
        }
    } break;

    case ObjectType::POLYGON: {
        auto poly = get_polygon(uu);
        switch (property) {
        case ObjectProperty::ID::LAYER:
            poly->layer = dynamic_cast<const PropertyValueInt &>(value).value;
            break;

        case ObjectProperty::ID::PARAMETER_CLASS:
            poly->parameter_class = dynamic_cast<const PropertyValueString &>(value).value;
            break;

        default:
            return false;
        }
    } break;

    case ObjectType::KEEPOUT: {
        auto keepout = get_keepout(uu);
        switch (property) {
        case ObjectProperty::ID::KEEPOUT_CLASS:
            keepout->keepout_class = dynamic_cast<const PropertyValueString &>(value).value;
            break;

        default:
            return false;
        }
    } break;

    case ObjectType::PICTURE: {
        auto pic = get_picture(uu);
        switch (property) {
        case ObjectProperty::ID::SIZE:
            pic->px_size = dynamic_cast<const PropertyValueInt &>(value).value;
            break;

        case ObjectProperty::ID::ON_TOP:
            pic->on_top = dynamic_cast<const PropertyValueBool &>(value).value;
            break;

        case ObjectProperty::ID::OPACITY:
            pic->opacity = dynamic_cast<const PropertyValueDouble &>(value).value;
            break;

        case ObjectProperty::ID::POSITION_X:
        case ObjectProperty::ID::POSITION_Y:
        case ObjectProperty::ID::ANGLE:
            set_placement(pic->placement, value, property);
            break;

        default:
            return false;
        }
    } break;

    default:
        return false;
    }
    if (!property_transaction) {
        rebuild_internal(false, "edit properties");
        set_needs_save(true);
    }
    return true;
}

bool Core::get_property_meta(ObjectType type, const UUID &uu, ObjectProperty::ID property, class PropertyMeta &meta)
{
    meta.is_settable = true;
    switch (type) {
    case ObjectType::HOLE:
        switch (property) {
        case ObjectProperty::ID::LENGTH:
            meta.is_settable = get_hole(uu)->shape == Hole::Shape::SLOT;
            return true;
        case ObjectProperty::ID::SPAN:
            layers_to_meta(meta);
            return true;
        default:
            return false;
        }
        break;
    case ObjectType::TEXT:
        switch (property) {
        case ObjectProperty::ID::LAYER:
            layers_to_meta(meta);
            return true;
        default:
            return false;
        }
        break;
    case ObjectType::LINE:
        switch (property) {
        case ObjectProperty::ID::LAYER:
            layers_to_meta(meta);
            return true;
        default:
            return false;
        }
        break;
    case ObjectType::ARC:
        switch (property) {
        case ObjectProperty::ID::LAYER:
            layers_to_meta(meta);
            return true;
        default:
            return false;
        }
        break;
    case ObjectType::POLYGON:
        switch (property) {
        case ObjectProperty::ID::LAYER:
            layers_to_meta(meta);
            return true;
        default:
            return false;
        }
        break;
    default:
        return false;
    }
    return false;
}

void Core::layers_to_meta(PropertyMeta &meta)
{
    PropertyMetaLayers &m = dynamic_cast<PropertyMetaLayers &>(meta);
    m.layers.clear();
    const auto &prv = get_layer_provider();
    for (const auto &it : prv.get_layers()) {
        m.layers.emplace(it.first, it.second);
    }
    m.is_visible = prv.get_layers().size() > 1;
}

void Core::get_placement(const Placement &placement, class PropertyValue &value, ObjectProperty::ID property)
{
    switch (property) {
    case ObjectProperty::ID::POSITION_X:
        dynamic_cast<PropertyValueInt &>(value).value = placement.shift.x;
        break;
    case ObjectProperty::ID::POSITION_Y:
        dynamic_cast<PropertyValueInt &>(value).value = placement.shift.y;
        break;
    case ObjectProperty::ID::ANGLE:
        dynamic_cast<PropertyValueInt &>(value).value = placement.get_angle();
        break;
    case ObjectProperty::ID::MIRROR:
        dynamic_cast<PropertyValueBool &>(value).value = placement.mirror;
        break;
    default:;
    }
}

void Core::set_placement(Placement &placement, const class PropertyValue &value, ObjectProperty::ID property)
{
    switch (property) {
    case ObjectProperty::ID::POSITION_X:
        placement.shift.x = dynamic_cast<const PropertyValueInt &>(value).value;
        break;
    case ObjectProperty::ID::POSITION_Y:
        placement.shift.y = dynamic_cast<const PropertyValueInt &>(value).value;
        break;
    case ObjectProperty::ID::ANGLE:
        placement.set_angle(dynamic_cast<const PropertyValueInt &>(value).value);
        break;
    case ObjectProperty::ID::MIRROR:
        placement.mirror = dynamic_cast<const PropertyValueBool &>(value).value;
        break;
    default:;
    }
}


} // namespace horizon
