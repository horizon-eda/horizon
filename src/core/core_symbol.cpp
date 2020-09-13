#include "core_symbol.hpp"
#include "core_properties.hpp"
#include <algorithm>
#include <fstream>
#include "nlohmann/json.hpp"
#include "util/util.hpp"
#include <giomm/file.h>
#include <glibmm/fileutils.h>
#include "pool/ipool.hpp"

namespace horizon {
CoreSymbol::CoreSymbol(const std::string &filename, IPool &pool)
    : Core(pool), sym(Symbol::new_from_file(filename, pool)), m_filename(filename)
{
    rebuild();
}

bool CoreSymbol::has_object_type(ObjectType ty) const
{
    switch (ty) {
    case ObjectType::JUNCTION:
    case ObjectType::LINE:
    case ObjectType::ARC:
    case ObjectType::SYMBOL_PIN:
    case ObjectType::TEXT:
    case ObjectType::POLYGON:
    case ObjectType::POLYGON_EDGE:
    case ObjectType::POLYGON_VERTEX:
        return true;
        break;
    default:;
    }

    return false;
}

Rules *CoreSymbol::get_rules()
{
    return &rules;
}

std::map<UUID, Text> *CoreSymbol::get_text_map()
{
    return &sym.texts;
}

std::map<UUID, Polygon> *CoreSymbol::get_polygon_map()
{
    return &sym.polygons;
}

Junction *CoreSymbol::get_junction(const UUID &uu)
{
    return sym.get_junction(uu);
}
Line *CoreSymbol::get_line(const UUID &uu)
{
    return &sym.lines.at(uu);
}
SymbolPin &CoreSymbol::get_symbol_pin(const UUID &uu)
{
    return sym.pins.at(uu);
}
Arc *CoreSymbol::get_arc(const UUID &uu)
{
    return &sym.arcs.at(uu);
}

Junction *CoreSymbol::insert_junction(const UUID &uu)
{
    auto x = sym.junctions.emplace(std::make_pair(uu, uu));
    return &(x.first->second);
}
void CoreSymbol::delete_junction(const UUID &uu)
{
    sym.junctions.erase(uu);
}

Line *CoreSymbol::insert_line(const UUID &uu)
{
    auto x = sym.lines.emplace(std::make_pair(uu, uu));
    return &(x.first->second);
}
void CoreSymbol::delete_line(const UUID &uu)
{
    sym.lines.erase(uu);
}

Arc *CoreSymbol::insert_arc(const UUID &uu)
{
    auto x = sym.arcs.emplace(std::make_pair(uu, uu));
    return &(x.first->second);
}
void CoreSymbol::delete_arc(const UUID &uu)
{
    sym.arcs.erase(uu);
}

void CoreSymbol::delete_symbol_pin(const UUID &uu)
{
    sym.pins.erase(uu);
}
SymbolPin &CoreSymbol::insert_symbol_pin(const UUID &uu)
{
    return sym.pins.emplace(uu, uu).first->second;
}

std::vector<Line *> CoreSymbol::get_lines()
{
    std::vector<Line *> r;
    for (auto &it : sym.lines) {
        r.push_back(&it.second);
    }
    return r;
}

std::vector<Arc *> CoreSymbol::get_arcs()
{
    std::vector<Arc *> r;
    for (auto &it : sym.arcs) {
        r.push_back(&it.second);
    }
    return r;
}

bool CoreSymbol::get_property(ObjectType type, const UUID &uu, ObjectProperty::ID property, PropertyValue &value)
{
    if (Core::get_property(type, uu, property, value))
        return true;
    switch (type) {
    case ObjectType::SYMBOL_PIN: {
        auto pin = &sym.pins.at(uu);
        switch (property) {
        case ObjectProperty::ID::NAME:
            dynamic_cast<PropertyValueString &>(value).value = sym.unit->pins.at(uu).primary_name;
            return true;

        case ObjectProperty::ID::NAME_VISIBLE:
            dynamic_cast<PropertyValueBool &>(value).value = pin->name_visible;
            return true;

        case ObjectProperty::ID::PAD_VISIBLE:
            dynamic_cast<PropertyValueBool &>(value).value = pin->pad_visible;
            return true;

        case ObjectProperty::ID::LENGTH:
            dynamic_cast<PropertyValueInt &>(value).value = pin->length;
            return true;

        case ObjectProperty::ID::DOT:
            dynamic_cast<PropertyValueBool &>(value).value = pin->decoration.dot;
            return true;

        case ObjectProperty::ID::PIN_NAME_ORIENTATION:
            dynamic_cast<PropertyValueInt &>(value).value = static_cast<int>(pin->name_orientation);
            return true;

        case ObjectProperty::ID::CLOCK:
            dynamic_cast<PropertyValueBool &>(value).value = pin->decoration.clock;
            return true;

        case ObjectProperty::ID::SCHMITT:
            dynamic_cast<PropertyValueBool &>(value).value = pin->decoration.schmitt;
            return true;

        case ObjectProperty::ID::DRIVER:
            dynamic_cast<PropertyValueInt &>(value).value = static_cast<int>(pin->decoration.driver);
            return true;

        default:
            return false;
        }
    } break;

    default:
        return false;
    }
}

bool CoreSymbol::set_property(ObjectType type, const UUID &uu, ObjectProperty::ID property, const PropertyValue &value)
{
    if (Core::set_property(type, uu, property, value))
        return true;
    switch (type) {
    case ObjectType::SYMBOL_PIN: {
        auto pin = &sym.pins.at(uu);
        switch (property) {
        case ObjectProperty::ID::NAME_VISIBLE:
            pin->name_visible = dynamic_cast<const PropertyValueBool &>(value).value;
            break;

        case ObjectProperty::ID::PAD_VISIBLE:
            pin->pad_visible = dynamic_cast<const PropertyValueBool &>(value).value;
            break;

        case ObjectProperty::ID::LENGTH:
            pin->length = dynamic_cast<const PropertyValueInt &>(value).value;
            break;

        case ObjectProperty::ID::DOT:
            pin->decoration.dot = dynamic_cast<const PropertyValueBool &>(value).value;
            break;

        case ObjectProperty::ID::PIN_NAME_ORIENTATION:
            pin->name_orientation =
                    static_cast<SymbolPin::NameOrientation>(dynamic_cast<const PropertyValueInt &>(value).value);
            break;

        case ObjectProperty::ID::CLOCK:
            pin->decoration.clock = dynamic_cast<const PropertyValueBool &>(value).value;
            break;

        case ObjectProperty::ID::SCHMITT:
            pin->decoration.schmitt = dynamic_cast<const PropertyValueBool &>(value).value;
            break;

        case ObjectProperty::ID::DRIVER:
            pin->decoration.driver =
                    static_cast<SymbolPin::Decoration::Driver>(dynamic_cast<const PropertyValueInt &>(value).value);
            break;

        default:
            return false;
        }
    } break;

    default:
        return false;
    }
    if (!property_transaction) {
        rebuild(false);
        set_needs_save(true);
    }
    return true;
}

bool CoreSymbol::get_property_meta(ObjectType type, const UUID &uu, ObjectProperty::ID property, PropertyMeta &meta)
{
    return Core::get_property_meta(type, uu, property, meta);
}

std::string CoreSymbol::get_display_name(ObjectType type, const UUID &uu)
{
    switch (type) {
    case ObjectType::SYMBOL_PIN:
        return get_symbol_pin(uu).name;

    default:
        return Core::get_display_name(type, uu);
    }
}

void CoreSymbol::rebuild(bool from_undo)
{
    sym.expand(pin_display_mode);
    Core::rebuild(from_undo);
}

void CoreSymbol::history_push()
{
    history.push_back(std::make_unique<CoreSymbol::HistoryItem>(sym));
}

void CoreSymbol::history_load(unsigned int i)
{
    auto x = dynamic_cast<CoreSymbol::HistoryItem *>(history.at(history_current).get());
    sym = x->sym;
    sym.expand(pin_display_mode);
    s_signal_rebuilt.emit();
}

LayerProvider &CoreSymbol::get_layer_provider()
{
    return sym;
}

const Symbol *CoreSymbol::get_canvas_data()
{
    return &sym;
}

std::pair<Coordi, Coordi> CoreSymbol::get_bbox()
{
    auto bb = sym.get_bbox(true);
    int64_t pad = 1_mm;
    bb.first.x -= pad;
    bb.first.y -= pad;

    bb.second.x += pad;
    bb.second.y += pad;
    return bb;
}

Symbol &CoreSymbol::get_symbol()
{
    return sym;
}

void CoreSymbol::reload_pool()
{
    m_pool.clear();
    sym.unit = m_pool.get_unit(sym.unit.uuid);
    history_clear();
    rebuild();
}

void CoreSymbol::set_pin_display_mode(Symbol::PinDisplayMode mode)
{
    if (tool_is_active())
        return;
    pin_display_mode = mode;
    sym.expand(pin_display_mode);
}

const std::string &CoreSymbol::get_filename() const
{
    return m_filename;
}

void CoreSymbol::save(const std::string &suffix)
{
    s_signal_save.emit();

    json j = sym.serialize();
    save_json_to_file(m_filename + suffix, j);
}


void CoreSymbol::delete_autosave()
{
    if (Glib::file_test(m_filename + autosave_suffix, Glib::FILE_TEST_IS_REGULAR))
        Gio::File::create_for_path(m_filename + autosave_suffix)->remove();
}

} // namespace horizon
