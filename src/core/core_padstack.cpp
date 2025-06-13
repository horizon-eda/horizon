#include "core_padstack.hpp"
#include "core_properties.hpp"
#include <nlohmann/json.hpp>
#include "util/util.hpp"
#include <giomm/file.h>
#include <glibmm/fileutils.h>

namespace horizon {
CorePadstack::CorePadstack(const std::string &filename, IPool &pool)
    : Core(pool, nullptr), padstack(Padstack::new_from_file(filename)), m_filename(filename),
      parameter_program_code(padstack.parameter_program.get_code()), parameter_set(padstack.parameter_set),
      parameters_required(padstack.parameters_required)
{
    rebuild("init");
}

bool CorePadstack::has_object_type(ObjectType ty) const
{
    switch (ty) {
    case ObjectType::HOLE:
    case ObjectType::POLYGON:
    case ObjectType::POLYGON_VERTEX:
    case ObjectType::POLYGON_EDGE:
    case ObjectType::POLYGON_ARC_CENTER:
    case ObjectType::SHAPE:
        return true;
        break;
    default:;
    }

    return false;
}

bool CorePadstack::get_property(ObjectType type, const UUID &uu, ObjectProperty::ID property, PropertyValue &value)
{
    if (Core::get_property(type, uu, property, value))
        return true;
    switch (type) {
    case ObjectType::HOLE: {
        auto hole = &padstack.holes.at(uu);
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
    case ObjectType::SHAPE: {
        auto shape = &padstack.shapes.at(uu);
        switch (property) {
        case ObjectProperty::ID::LAYER:
            dynamic_cast<PropertyValueInt &>(value).value = shape->layer;
            return true;

        case ObjectProperty::ID::POSITION_X:
        case ObjectProperty::ID::POSITION_Y:
        case ObjectProperty::ID::ANGLE:
            get_placement(shape->placement, value, property);
            return true;

        case ObjectProperty::ID::PARAMETER_CLASS:
            dynamic_cast<PropertyValueString &>(value).value = shape->parameter_class;
            return true;

        case ObjectProperty::ID::FORM:
            dynamic_cast<PropertyValueInt &>(value).value = static_cast<int>(shape->form);
            return true;

        case ObjectProperty::ID::DIAMETER:
            if (shape->params.size())
                dynamic_cast<PropertyValueInt &>(value).value = shape->params.at(0);
            return true;

        case ObjectProperty::ID::WIDTH:
            if (shape->params.size())
                dynamic_cast<PropertyValueInt &>(value).value = shape->params.at(0);
            return true;

        case ObjectProperty::ID::HEIGHT:
            if (shape->params.size() > 1)
                dynamic_cast<PropertyValueInt &>(value).value = shape->params.at(1);
            return true;

        default:
            return false;
        }
    } break;

    default:
        return false;
    }
}

bool CorePadstack::set_property(ObjectType type, const UUID &uu, ObjectProperty::ID property,
                                const PropertyValue &value)
{
    if (Core::set_property(type, uu, property, value))
        return true;
    switch (type) {
    case ObjectType::HOLE: {
        auto hole = &padstack.holes.at(uu);
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

    case ObjectType::SHAPE: {
        auto shape = &padstack.shapes.at(uu);
        switch (property) {
        case ObjectProperty::ID::LAYER:
            shape->layer = dynamic_cast<const PropertyValueInt &>(value).value;
            break;

        case ObjectProperty::ID::POSITION_X:
        case ObjectProperty::ID::POSITION_Y:
        case ObjectProperty::ID::ANGLE:
            set_placement(shape->placement, value, property);
            break;

        case ObjectProperty::ID::PARAMETER_CLASS:
            shape->parameter_class = dynamic_cast<const PropertyValueString &>(value).value;
            break;

        case ObjectProperty::ID::FORM:
            shape->form = static_cast<Shape::Form>(dynamic_cast<const PropertyValueInt &>(value).value);
            shape->params.resize(shape->form == Shape::Form::CIRCLE ? 1 : 2);
            break;

        case ObjectProperty::ID::DIAMETER:
            if (shape->form == Shape::Form::CIRCLE) {
                shape->params.resize(1);
                shape->params.at(0) = dynamic_cast<const PropertyValueInt &>(value).value;
            }
            break;

        case ObjectProperty::ID::WIDTH:
        case ObjectProperty::ID::HEIGHT:
            if (shape->form == Shape::Form::OBROUND || shape->form == Shape::Form::RECTANGLE) {
                shape->params.resize(2);
                size_t idx = property == ObjectProperty::ID::WIDTH ? 0 : 1;
                shape->params.at(idx) = dynamic_cast<const PropertyValueInt &>(value).value;
            }
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

bool CorePadstack::get_property_meta(ObjectType type, const UUID &uu, ObjectProperty::ID property, PropertyMeta &meta)
{
    if (Core::get_property_meta(type, uu, property, meta))
        return true;
    switch (type) {
    case ObjectType::HOLE: {
        auto hole = &padstack.holes.at(uu);
        switch (property) {
        case ObjectProperty::ID::LENGTH:
            meta.is_settable = hole->shape == Hole::Shape::SLOT;
            return true;
        case ObjectProperty::ID::SPAN:
            layers_to_meta(meta);
            return true;
        default:
            return false;
        }
    } break;

    case ObjectType::SHAPE: {
        auto shape = &padstack.shapes.at(uu);
        switch (property) {
        case ObjectProperty::ID::LAYER:
            layers_to_meta(meta);
            return true;

        case ObjectProperty::ID::DIAMETER:
            meta.is_visible = shape->form == Shape::Form::CIRCLE;
            return true;

        case ObjectProperty::ID::WIDTH:
        case ObjectProperty::ID::HEIGHT:
            meta.is_visible = shape->form == Shape::Form::OBROUND || shape->form == Shape::Form::RECTANGLE;
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

std::string CorePadstack::get_display_name(ObjectType type, const UUID &uu)
{
    switch (type) {
    case ObjectType::HOLE:
        return padstack.holes.at(uu).shape == Hole::Shape::ROUND ? "Round" : "Slot";

    case ObjectType::SHAPE: {
        auto form = padstack.shapes.at(uu).form;
        switch (form) {
        case Shape::Form::CIRCLE:
            return "Circle";

        case Shape::Form::OBROUND:
            return "Obround";

        case Shape::Form::RECTANGLE:
            return "Rectangle";

        default:
            return "unknown";
        }
    } break;

    default:
        return Core::get_display_name(type, uu);
    }
}

LayerProvider &CorePadstack::get_layer_provider()
{
    return padstack;
}

std::map<UUID, Polygon> *CorePadstack::get_polygon_map()
{
    return &padstack.polygons;
}

void CorePadstack::rebuild_internal(bool from_undo, const std::string &comment)
{
    rebuild_finish(from_undo, comment);
}

class HistoryItemPadstack : public HistoryManager::HistoryItem {
public:
    HistoryItemPadstack(const Padstack &s, const std::string &cm) : HistoryManager::HistoryItem(cm), padstack(s)
    {
    }
    Padstack padstack;
};

std::unique_ptr<HistoryManager::HistoryItem> CorePadstack::make_history_item(const std::string &comment)
{
    return std::make_unique<HistoryItemPadstack>(padstack, comment);
}

void CorePadstack::history_load(const HistoryManager::HistoryItem &it)
{
    const auto &x = dynamic_cast<const HistoryItemPadstack &>(it);
    padstack = x.padstack;
}

const Padstack &CorePadstack::get_canvas_data()
{
    return padstack;
}

Padstack &CorePadstack::get_padstack()
{
    return padstack;
}

std::pair<Coordi, Coordi> CorePadstack::get_bbox()
{
    auto bb = padstack.get_bbox();
    int64_t pad = 1_mm;
    bb.first.x -= pad;
    bb.first.y -= pad;

    bb.second.x += pad;
    bb.second.y += pad;
    return bb;
}

const std::string &CorePadstack::get_filename() const
{
    return m_filename;
}

void CorePadstack::save(const std::string &suffix)
{
    padstack.parameter_program.set_code(parameter_program_code);
    padstack.parameter_set = parameter_set;
    padstack.parameters_required = parameters_required;

    s_signal_save.emit();

    json j = padstack.serialize();
    save_json_to_file(m_filename + suffix, j);
}


void CorePadstack::delete_autosave()
{
    if (Glib::file_test(m_filename + autosave_suffix, Glib::FILE_TEST_IS_REGULAR))
        Gio::File::create_for_path(m_filename + autosave_suffix)->remove();
}

void CorePadstack::set_temp_mode()
{
    Gio::File::create_for_path(m_filename)->remove();
    m_filename.clear();
}

void CorePadstack::set_filename(const std::string &filename)
{
    m_filename = filename;
}

} // namespace horizon
