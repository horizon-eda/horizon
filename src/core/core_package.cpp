#include "core_package.hpp"
#include "core_properties.hpp"
#include <nlohmann/json.hpp>
#include "logger/logger.hpp"
#include "util/util.hpp"
#include <giomm/file.h>
#include <glibmm/fileutils.h>
#include <glibmm/miscutils.h>
#include "pool/ipool.hpp"

namespace horizon {
CorePackage::CorePackage(const std::string &filename, IPool &pool)
    : Core(pool, nullptr), package(Package::new_from_file(filename, pool)), m_filename(filename),
      m_pictures_dir(Glib::build_filename(Glib::path_get_dirname(filename), "pictures")), rules(package.rules),
      grid_settings(package.grid_settings), parameter_program_code(package.parameter_program.get_code()),
      parameter_set(package.parameter_set), parameters_fixed(package.parameters_fixed), models(package.models),
      default_model(package.default_model)
{
    package.load_pictures(m_pictures_dir);
    rebuild("init");
}

bool CorePackage::has_object_type(ObjectType ty) const
{
    switch (ty) {
    case ObjectType::JUNCTION:
    case ObjectType::POLYGON:
    case ObjectType::POLYGON_EDGE:
    case ObjectType::POLYGON_VERTEX:
    case ObjectType::POLYGON_ARC_CENTER:
    case ObjectType::PAD:
    case ObjectType::TEXT:
    case ObjectType::LINE:
    case ObjectType::ARC:
    case ObjectType::KEEPOUT:
    case ObjectType::DIMENSION:
    case ObjectType::PICTURE:
        return true;
        break;
    default:;
    }

    return false;
}

LayerProvider &CorePackage::get_layer_provider()
{
    return package;
}

Package &CorePackage::get_package()
{
    return package;
}

Rules *CorePackage::get_rules()
{
    return &rules;
}

std::map<UUID, Junction> *CorePackage::get_junction_map()
{
    return &package.junctions;
}
std::map<UUID, Line> *CorePackage::get_line_map()
{
    return &package.lines;
}
std::map<UUID, Arc> *CorePackage::get_arc_map()
{
    return &package.arcs;
}
std::map<UUID, Text> *CorePackage::get_text_map()
{
    return &package.texts;
}
std::map<UUID, Polygon> *CorePackage::get_polygon_map()
{
    return &package.polygons;
}
std::map<UUID, Keepout> *CorePackage::get_keepout_map()
{
    return &package.keepouts;
}
std::map<UUID, Dimension> *CorePackage::get_dimension_map()
{
    return &package.dimensions;
}
std::map<UUID, Picture> *CorePackage::get_picture_map()
{
    return &package.pictures;
}

std::pair<Coordi, Coordi> CorePackage::get_bbox()
{
    auto bb = package.get_bbox();
    int64_t pad = 1_mm;
    bb.first.x -= pad;
    bb.first.y -= pad;

    bb.second.x += pad;
    bb.second.y += pad;
    return bb;
}

bool CorePackage::get_property(ObjectType type, const UUID &uu, ObjectProperty::ID property, PropertyValue &value)
{
    if (Core::get_property(type, uu, property, value))
        return true;
    switch (type) {
    case ObjectType::PAD: {
        auto pad = &package.pads.at(uu);
        switch (property) {
        case ObjectProperty::ID::NAME:
            dynamic_cast<PropertyValueString &>(value).value = pad->name;
            return true;

        case ObjectProperty::ID::VALUE:
            dynamic_cast<PropertyValueString &>(value).value = pad->pool_padstack->name;
            return true;

        case ObjectProperty::ID::POSITION_X:
        case ObjectProperty::ID::POSITION_Y:
        case ObjectProperty::ID::ANGLE:
            get_placement(pad->placement, value, property);
            return true;

        case ObjectProperty::ID::PAD_TYPE: {
            const auto ps = package.pads.at(uu).pool_padstack;
            std::string pad_type;
            switch (ps->type) {
            case Padstack::Type::MECHANICAL:
                pad_type = "Mechanical";
                break;
            case Padstack::Type::BOTTOM:
                pad_type = "Bottom";
                break;
            case Padstack::Type::TOP:
                pad_type = "Top";
                break;
            case Padstack::Type::THROUGH:
                pad_type = "Through";
                break;
            default:
                pad_type = "Invalid";
            }
            dynamic_cast<PropertyValueString &>(value).value = pad_type;
            return true;
        } break;

        default:
            return false;
        }
    } break;

    default:
        return false;
    }
}

bool CorePackage::set_property(ObjectType type, const UUID &uu, ObjectProperty::ID property, const PropertyValue &value)
{
    if (Core::set_property(type, uu, property, value))
        return true;
    switch (type) {
    case ObjectType::PAD: {
        auto pad = &package.pads.at(uu);
        switch (property) {
        case ObjectProperty::ID::NAME:
            pad->name = dynamic_cast<const PropertyValueString &>(value).value;
            break;

        case ObjectProperty::ID::POSITION_X:
        case ObjectProperty::ID::POSITION_Y:
        case ObjectProperty::ID::ANGLE:
            set_placement(pad->placement, value, property);
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

std::string CorePackage::get_display_name(ObjectType type, const UUID &uu)
{
    switch (type) {
    case ObjectType::PAD:
        return package.pads.at(uu).name;

    default:
        return Core::get_display_name(type, uu);
    }
}

bool CorePackage::get_property_meta(ObjectType type, const UUID &uu, ObjectProperty::ID property, PropertyMeta &meta)
{
    return Core::get_property_meta(type, uu, property, meta);
}

void CorePackage::rebuild_internal(bool from_undo, const std::string &comment)
{
    if (auto r = package.apply_parameter_set({})) {
        Logger::get().log_critical("Parameter program failed", Logger::Domain::CORE, r.value());
    }
    package.expand();
    package.update_warnings();
    rebuild_finish(from_undo, comment);
}

class HistoryItemPackage : public HistoryManager::HistoryItem {
public:
    HistoryItemPackage(const Package &k, const std::string &cm) : HistoryManager::HistoryItem(cm), package(k)
    {
    }
    Package package;
};

std::unique_ptr<HistoryManager::HistoryItem> CorePackage::make_history_item(const std::string &comment)
{
    return std::make_unique<HistoryItemPackage>(package, comment);
}

void CorePackage::history_load(const HistoryManager::HistoryItem &it)
{
    const auto &x = dynamic_cast<const HistoryItemPackage &>(it);
    package = x.package;
}

const Package &CorePackage::get_canvas_data()
{
    return package;
}

void CorePackage::reload_pool()
{
    m_pool.clear();
    package.update_refs(m_pool);
    rebuild("reload pool");
    s_signal_can_undo_redo.emit();
}

json CorePackage::get_meta()
{
    return get_meta_from_file(m_filename);
}

const std::string &CorePackage::get_filename() const
{
    return m_filename;
}

void CorePackage::save(const std::string &suffix)
{
    package.parameter_set = parameter_set;
    package.parameters_fixed = parameters_fixed;
    package.parameter_program.set_code(parameter_program_code);
    package.models = models;
    package.default_model = default_model;
    package.rules = rules;
    package.grid_settings = grid_settings;

    s_signal_save.emit();

    json j = package.serialize();
    save_json_to_file(m_filename + suffix, j);
    package.save_pictures(m_pictures_dir);
}


void CorePackage::delete_autosave()
{
    if (Glib::file_test(m_filename + autosave_suffix, Glib::FILE_TEST_IS_REGULAR))
        Gio::File::create_for_path(m_filename + autosave_suffix)->remove();
}

void CorePackage::set_temp_mode()
{
    Gio::File::create_for_path(m_filename)->remove();
    m_filename.clear();
    m_pictures_dir.clear();
}

void CorePackage::set_filename(const std::string &filename)
{
    m_filename = filename;
    m_pictures_dir = Glib::build_filename(Glib::path_get_dirname(filename), "pictures");
}

} // namespace horizon
