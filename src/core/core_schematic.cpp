#include "core_schematic.hpp"
#include "core_properties.hpp"
#include "pool/part.hpp"
#include "util/util.hpp"
#include <algorithm>
#include "nlohmann/json.hpp"
#include <giomm/file.h>
#include <glibmm/fileutils.h>
#include "pool/ipool.hpp"
#include "util/str_util.hpp"
#include "util/picture_load.hpp"
#include "logger/logger.hpp"
#include <filesystem>
#include <iostream>

namespace horizon {
namespace fs = std::filesystem;


CoreSchematic::CoreSchematic(const Filenames &fns, IPool &pool, IPool &pool_caching)
    : Core(pool, &pool_caching), blocks(BlocksSchematic::new_from_file(fns.blocks, pool_caching)),
      rules(get_top_schematic()->rules), bom_export_settings(get_top_block()->bom_export_settings),
      pdf_export_settings(get_top_schematic()->pdf_export_settings), filenames(fns)
{
    block_uuid = blocks->top_block;
    sheet_uuid = get_current_schematic()->get_sheet_at_index(1).uuid;
    for (auto &[uu, block] : blocks->blocks) {
        block.schematic.load_pictures(filenames.pictures_dir);
        block.symbol.load_pictures(filenames.pictures_dir);
    }
    rebuild("init");
}

Junction *CoreSchematic::get_junction(const UUID &uu)
{
    if (get_block_symbol_mode())
        return &get_block_symbol().junctions.at(uu);
    else
        return &get_sheet()->junctions.at(uu);
}

const Schematic *CoreSchematic::get_current_schematic() const
{
    return &blocks->blocks.at(block_uuid).schematic;
}

Schematic *CoreSchematic::get_current_schematic()
{
    return const_cast<Schematic *>(static_cast<const CoreSchematic *>(this)->get_current_schematic());
}

const Schematic *CoreSchematic::get_top_schematic() const
{
    return &blocks->get_top_block_item().schematic;
}

Schematic *CoreSchematic::get_top_schematic()
{
    return const_cast<Schematic *>(static_cast<const CoreSchematic *>(this)->get_top_schematic());
}


Block *CoreSchematic::get_current_block()
{
    return &blocks->blocks.at(block_uuid).block;
}

const Block *CoreSchematic::get_current_block() const
{
    return &blocks->blocks.at(block_uuid).block;
}

Block *CoreSchematic::get_top_block()
{
    return &blocks->get_top_block_item().block;
}

const Block *CoreSchematic::get_top_block() const
{
    return &blocks->get_top_block_item().block;
}

LayerProvider &CoreSchematic::get_layer_provider()
{
    if (get_block_symbol_mode())
        return get_block_symbol();
    else
        return *get_sheet();
}

Sheet *CoreSchematic::get_sheet()
{
    if (get_block_symbol_mode())
        throw std::runtime_error("in block symbol mode");

    return &get_current_schematic()->sheets.at(sheet_uuid);
}

const Sheet *CoreSchematic::get_sheet() const
{
    if (get_block_symbol_mode())
        throw std::runtime_error("in block symbol mode");

    return &get_current_schematic()->sheets.at(sheet_uuid);
}

BlockSymbol &CoreSchematic::get_block_symbol()
{
    if (!get_block_symbol_mode())
        throw std::runtime_error("not block symbol mode");

    return blocks->blocks.at(block_uuid).symbol;
}

Junction *CoreSchematic::insert_junction(const UUID &uu)
{
    if (get_block_symbol_mode())
        return &get_block_symbol().junctions.emplace(std::make_pair(uu, uu)).first->second;
    else
        return &get_sheet()->junctions.emplace(std::make_pair(uu, uu)).first->second;
}

void CoreSchematic::delete_junction(const UUID &uu)
{
    if (get_block_symbol_mode())
        get_block_symbol().junctions.erase(uu);
    else
        get_sheet()->junctions.erase(uu);
}

std::map<UUID, Line> *CoreSchematic::get_line_map()
{
    if (get_block_symbol_mode())
        return &get_block_symbol().lines;
    else
        return &get_sheet()->lines;
}
std::map<UUID, Arc> *CoreSchematic::get_arc_map()
{
    if (get_block_symbol_mode())
        return &get_block_symbol().arcs;
    else
        return &get_sheet()->arcs;
}
std::map<UUID, Text> *CoreSchematic::get_text_map()
{
    if (get_block_symbol_mode())
        return &get_block_symbol().texts;
    else
        return &get_sheet()->texts;
}
std::map<UUID, Picture> *CoreSchematic::get_picture_map()
{
    if (get_block_symbol_mode())
        return &get_block_symbol().pictures;
    else
        return &get_sheet()->pictures;
}

bool CoreSchematic::has_object_type(ObjectType ty) const
{
    if (get_block_symbol_mode()) {
        switch (ty) {
        case ObjectType::JUNCTION:
        case ObjectType::TEXT:
        case ObjectType::LINE:
        case ObjectType::ARC:
        case ObjectType::PICTURE:
        case ObjectType::BLOCK_SYMBOL_PORT:
            return true;
            break;
        default:
            return false;
        }
    }
    else {
        switch (ty) {
        case ObjectType::JUNCTION:
        case ObjectType::SCHEMATIC_SYMBOL:
        case ObjectType::SCHEMATIC_BLOCK_SYMBOL:
        case ObjectType::BUS_LABEL:
        case ObjectType::BUS_RIPPER:
        case ObjectType::NET_LABEL:
        case ObjectType::LINE_NET:
        case ObjectType::POWER_SYMBOL:
        case ObjectType::TEXT:
        case ObjectType::LINE:
        case ObjectType::ARC:
        case ObjectType::PICTURE:
        case ObjectType::SCHEMATIC_NET_TIE:
            return true;
            break;
        default:
            return false;
        }
    }
}

Rules *CoreSchematic::get_rules()
{
    return &rules;
}

bool CoreSchematic::get_property(ObjectType type, const UUID &uu, ObjectProperty::ID property, PropertyValue &value)
{
    if (Core::get_property(type, uu, property, value))
        return true;
    switch (type) {
    case ObjectType::NET: {
        auto &net = get_current_block()->nets.at(uu);
        switch (property) {
        case ObjectProperty::ID::NAME:
            dynamic_cast<PropertyValueString &>(value).value = net.name;
            return true;

        case ObjectProperty::ID::NET_CLASS:
            dynamic_cast<PropertyValueUUID &>(value).value = net.net_class->uuid;
            return true;

        case ObjectProperty::ID::DIFFPAIR: {
            std::string s;
            if (net.diffpair) {
                s = (net.diffpair_primary ? "Pri: " : "Sec: ") + net.diffpair->name;
            }
            else {
                s = "None";
            }
            dynamic_cast<PropertyValueString &>(value).value = s;
            return true;
        }

        case ObjectProperty::ID::IS_POWER:
            dynamic_cast<PropertyValueBool &>(value).value = net.is_power;
            return true;

        case ObjectProperty::ID::IS_PORT:
            dynamic_cast<PropertyValueBool &>(value).value = net.is_port;
            return true;

        default:
            return false;
        }
    } break;

    case ObjectType::NET_LABEL: {
        auto label = &get_sheet()->net_labels.at(uu);
        switch (property) {
        case ObjectProperty::ID::NAME:
            if (label->junction->net)
                dynamic_cast<PropertyValueString &>(value).value = label->junction->net->name;
            else
                dynamic_cast<PropertyValueString &>(value).value = "<no net>";
            return true;

        case ObjectProperty::ID::OFFSHEET_REFS:
            dynamic_cast<PropertyValueBool &>(value).value = label->offsheet_refs;
            return true;

        case ObjectProperty::ID::SIZE:
            dynamic_cast<PropertyValueInt &>(value).value = label->size;
            return true;

        case ObjectProperty::ID::IS_PORT:
            dynamic_cast<PropertyValueBool &>(value).value = label->show_port;
            return true;

        default:
            return false;
        }
    } break;

    case ObjectType::BUS_RIPPER: {
        const auto &ripper = get_sheet()->bus_rippers.at(uu);
        switch (property) {
        case ObjectProperty::ID::TEXT_POSITION:
            dynamic_cast<PropertyValueInt &>(value).value = static_cast<int>(ripper.text_position);
            return true;

        default:
            return false;
        }
    } break;

    case ObjectType::COMPONENT: {
        const auto comp = &get_current_block()->components.at(uu);
        const auto inst = get_block_instance_mapping();
        BlockInstanceMapping::ComponentInfo *info = nullptr;
        if (inst && inst->components.count(comp->uuid))
            info = &inst->components.at(comp->uuid);
        switch (property) {
        case ObjectProperty::ID::REFDES:
            if (!in_hierarchy())
                dynamic_cast<PropertyValueString &>(value).value = comp->get_prefix() + "?";
            if (info)
                dynamic_cast<PropertyValueString &>(value).value = info->refdes;
            else
                dynamic_cast<PropertyValueString &>(value).value = comp->refdes;
            return true;

        case ObjectProperty::ID::VALUE:
            if (get_current_block()->components.at(uu).part)
                dynamic_cast<PropertyValueString &>(value).value = comp->part->get_value();
            else
                dynamic_cast<PropertyValueString &>(value).value = comp->value;
            return true;

        case ObjectProperty::ID::MPN:
            if (get_current_block()->components.at(uu).part)
                dynamic_cast<PropertyValueString &>(value).value = comp->part->get_MPN();
            else
                dynamic_cast<PropertyValueString &>(value).value = "<no part>";
            return true;

        case ObjectProperty::ID::NOPOPULATE:
            if (get_current_block()->components.at(uu).part) {
                if (info)
                    dynamic_cast<PropertyValueBool &>(value).value = info->nopopulate;
                else
                    dynamic_cast<PropertyValueBool &>(value).value = comp->nopopulate;
            }
            else {
                dynamic_cast<PropertyValueBool &>(value).value = false;
            }
            return true;

        default:
            return false;
        }
    } break;

    case ObjectType::BLOCK_INSTANCE: {
        auto &inst = get_current_block()->block_instances.at(uu);
        switch (property) {
        case ObjectProperty::ID::REFDES:
            dynamic_cast<PropertyValueString &>(value).value = inst.refdes;
            return true;

        case ObjectProperty::ID::NAME:
            dynamic_cast<PropertyValueString &>(value).value = inst.block->name;
            return true;

        default:
            return false;
        }
    } break;


    case ObjectType::SCHEMATIC_SYMBOL: {
        auto sym = &get_sheet()->symbols.at(uu);
        switch (property) {
        case ObjectProperty::ID::DISPLAY_DIRECTIONS:
            dynamic_cast<PropertyValueBool &>(value).value = sym->display_directions;
            return true;

        case ObjectProperty::ID::DISPLAY_ALL_PADS:
            dynamic_cast<PropertyValueBool &>(value).value = sym->display_all_pads;
            return true;

        case ObjectProperty::ID::PIN_NAME_DISPLAY:
            dynamic_cast<PropertyValueInt &>(value).value = static_cast<int>(sym->pin_display_mode);
            return true;

        case ObjectProperty::ID::EXPAND:
            dynamic_cast<PropertyValueInt &>(value).value = static_cast<int>(sym->expand);
            return true;

        case ObjectProperty::ID::REFDES:
            dynamic_cast<PropertyValueString &>(value).value =
                    get_top_block()->get_component_info(*sym->component, instance_path).refdes + sym->gate->suffix;
            return true;

        case ObjectProperty::ID::VALUE:
            dynamic_cast<PropertyValueString &>(value).value = sym->custom_value;
            return true;

        default:
            return false;
        }
    } break;

    case ObjectType::BLOCK_SYMBOL_PORT: {
        const auto &sym = get_block_symbol();
        const auto &port = sym.ports.at(uu);
        switch (property) {
        case ObjectProperty::ID::NAME:
            dynamic_cast<PropertyValueString &>(value).value = port.name;
            return true;

        case ObjectProperty::ID::LENGTH:
            dynamic_cast<PropertyValueInt &>(value).value = port.length;
            return true;

        case ObjectProperty::ID::PIN_NAME_ORIENTATION:
            dynamic_cast<PropertyValueInt &>(value).value = static_cast<int>(port.name_orientation);
            return true;

        default:
            return false;
        }
    } break;

    default:
        return false;
    }
}

bool CoreSchematic::set_property(ObjectType type, const UUID &uu, ObjectProperty::ID property,
                                 const PropertyValue &value)
{
    if (Core::set_property(type, uu, property, value))
        return true;
    switch (type) {
    case ObjectType::COMPONENT: {
        auto comp = &get_current_block()->components.at(uu);
        const auto inst = get_block_instance_mapping();
        BlockInstanceMapping::ComponentInfo *info = nullptr;
        if (inst)
            info = &inst->components[comp->uuid];
        switch (property) {
        case ObjectProperty::ID::REFDES:
            if (info)
                info->refdes = dynamic_cast<const PropertyValueString &>(value).value;
            else
                comp->refdes = dynamic_cast<const PropertyValueString &>(value).value;
            break;

        case ObjectProperty::ID::VALUE:
            if (comp->part)
                return false;
            comp->value = dynamic_cast<const PropertyValueString &>(value).value;
            break;

        case ObjectProperty::ID::NOPOPULATE:
            if (info)
                info->nopopulate = dynamic_cast<const PropertyValueBool &>(value).value;
            else
                comp->nopopulate = dynamic_cast<const PropertyValueBool &>(value).value;
            break;

        default:
            return false;
        }
    } break;

    case ObjectType::BLOCK_INSTANCE: {
        auto &inst = get_current_block()->block_instances.at(uu);
        switch (property) {
        case ObjectProperty::ID::REFDES:
            inst.refdes = dynamic_cast<const PropertyValueString &>(value).value;
            break;

        default:
            return false;
        }
    } break;

    case ObjectType::NET: {
        auto &net = get_current_block()->nets.at(uu);
        switch (property) {
        case ObjectProperty::ID::NAME:
            net.name = dynamic_cast<const PropertyValueString &>(value).value;
            break;

        case ObjectProperty::ID::NET_CLASS: {
            net.net_class = &get_current_block()->net_classes.at(dynamic_cast<const PropertyValueUUID &>(value).value);
        } break;

        case ObjectProperty::ID::IS_POWER:
            if (net.is_power_forced)
                return false;
            net.is_power = dynamic_cast<const PropertyValueBool &>(value).value;
            break;

        case ObjectProperty::ID::IS_PORT:
            net.is_port = dynamic_cast<const PropertyValueBool &>(value).value;
            break;

        default:
            return false;
        }
    } break;

    case ObjectType::SCHEMATIC_SYMBOL: {
        auto sym = &get_sheet()->symbols.at(uu);
        switch (property) {
        case ObjectProperty::ID::DISPLAY_DIRECTIONS:
            sym->display_directions = dynamic_cast<const PropertyValueBool &>(value).value;
            break;

        case ObjectProperty::ID::DISPLAY_ALL_PADS:
            sym->display_all_pads = dynamic_cast<const PropertyValueBool &>(value).value;
            break;

        case ObjectProperty::ID::PIN_NAME_DISPLAY:
            sym->pin_display_mode =
                    static_cast<SchematicSymbol::PinDisplayMode>(dynamic_cast<const PropertyValueInt &>(value).value);
            break;

        case ObjectProperty::ID::EXPAND:
            sym->expand = dynamic_cast<const PropertyValueInt &>(value).value;
            sym->apply_expand();
            break;

        case ObjectProperty::ID::VALUE:
            sym->custom_value = dynamic_cast<const PropertyValueString &>(value).value;
            trim(sym->custom_value);
            break;

        default:
            return false;
        }
    } break;

    case ObjectType::NET_LABEL: {
        auto label = &get_sheet()->net_labels.at(uu);
        switch (property) {
        case ObjectProperty::ID::OFFSHEET_REFS:
            label->offsheet_refs = dynamic_cast<const PropertyValueBool &>(value).value;
            break;

        case ObjectProperty::ID::SIZE:
            label->size = dynamic_cast<const PropertyValueInt &>(value).value;
            break;

        case ObjectProperty::ID::IS_PORT:
            label->show_port = dynamic_cast<const PropertyValueBool &>(value).value;
            break;

        default:
            return false;
        }
    } break;

    case ObjectType::BUS_RIPPER: {
        auto &ripper = get_sheet()->bus_rippers.at(uu);
        switch (property) {
        case ObjectProperty::ID::TEXT_POSITION:
            ripper.text_position =
                    static_cast<BusRipper::TextPosition>(dynamic_cast<const PropertyValueInt &>(value).value);
            return true;

        default:
            return false;
        }
    } break;

    case ObjectType::BLOCK_SYMBOL_PORT: {
        auto &sym = get_block_symbol();
        auto &port = sym.ports.at(uu);
        switch (property) {
        case ObjectProperty::ID::NAME:
            port.name = dynamic_cast<const PropertyValueString &>(value).value;
            break;

        case ObjectProperty::ID::LENGTH:
            port.length = dynamic_cast<const PropertyValueInt &>(value).value;
            break;

        case ObjectProperty::ID::PIN_NAME_ORIENTATION:
            port.name_orientation =
                    static_cast<PinNameOrientation>(dynamic_cast<const PropertyValueInt &>(value).value);
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

bool CoreSchematic::get_property_meta(ObjectType type, const UUID &uu, ObjectProperty::ID property, PropertyMeta &meta)
{
    if (Core::get_property_meta(type, uu, property, meta))
        return true;
    switch (type) {
    case ObjectType::NET:
        switch (property) {
        case ObjectProperty::ID::IS_POWER: {
            const auto &net = get_current_block()->nets.at(uu);
            meta.is_settable = !net.is_power_forced && !net.is_port;
            return true;
        }

        case ObjectProperty::ID::IS_PORT: {
            const auto &net = get_current_block()->nets.at(uu);
            meta.is_settable = !net.is_power;
            meta.is_visible = !current_block_is_top();
            return true;
        }

        case ObjectProperty::ID::NET_CLASS: {
            PropertyMetaNetClasses &m = dynamic_cast<PropertyMetaNetClasses &>(meta);
            const auto &net = get_current_block()->nets.at(uu);
            if (net.is_power)
                meta.is_settable = current_block_is_top();
            m.net_classes.clear();
            for (const auto &it : get_current_block()->net_classes) {
                m.net_classes.emplace(it.first, it.second.name);
            }
            return true;
        }

        default:
            return false;
        }
        break;

    case ObjectType::COMPONENT:
        switch (property) {
        case ObjectProperty::ID::REFDES:
        case ObjectProperty::ID::NOPOPULATE:
            meta.is_settable = in_hierarchy();
            return true;

        case ObjectProperty::ID::VALUE:
            meta.is_settable = get_current_block()->components.at(uu).part == nullptr;
            return true;

        default:
            return false;
        }
        break;

    case ObjectType::SCHEMATIC_SYMBOL:
        switch (property) {
        case ObjectProperty::ID::EXPAND:
            meta.is_settable = get_sheet()->symbols.at(uu).pool_symbol->can_expand;
            return true;

        default:
            return false;
        }
        break;

    case ObjectType::TEXT:
        switch (property) {
        case ObjectProperty::ID::ALLOW_UPSIDE_DOWN:
            meta.is_visible = false;
            return true;
        default:
            return false;
        }
        break;

    case ObjectType::NET_LABEL:
        switch (property) {
        case ObjectProperty::ID::IS_PORT:
            if (current_block_is_top()) {
                meta.is_visible = false;
                return true;
            }

            if (get_sheet()->net_labels.at(uu).junction->net)
                meta.is_settable = get_sheet()->net_labels.at(uu).junction->net->is_port;
            else
                meta.is_settable = false;
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

std::string CoreSchematic::get_display_name(ObjectType type, const UUID &uu)
{
    return get_display_name(type, uu, sheet_uuid);
}

std::string CoreSchematic::get_display_name(ObjectType type, const UUID &uu, const UUID &sh)
{
    switch (type) {
    case ObjectType::NET:
        return get_current_block()->nets.at(uu).name;

    case ObjectType::LINE_NET: {
        const auto &li = get_current_schematic()->sheets.at(sh).net_lines.at(uu);
        return li.net ? li.net->name : "";
    }

    case ObjectType::JUNCTION: {
        if (sh) {
            const auto &ju = get_current_schematic()->sheets.at(sh).junctions.at(uu);
            return ju.net ? ju.net->name : "";
        }
        else {
            return Core::get_display_name(type, uu);
        }
    }

    case ObjectType::NET_LABEL: {
        const auto &la = get_current_schematic()->sheets.at(sh).net_labels.at(uu);
        return la.junction->net ? la.junction->net->name : "";
    }

    case ObjectType::SCHEMATIC_SYMBOL: {
        const auto &sym = get_current_schematic()->sheets.at(sh).symbols.at(uu);
        const auto &comp = *sym.component;
        const auto comp_info = get_top_block()->get_component_info(comp, get_instance_path());
        return comp_info.refdes + sym.gate->suffix;
    }

    case ObjectType::SCHEMATIC_BLOCK_SYMBOL: {
        const auto &sym = get_current_schematic()->sheets.at(sh).block_symbols.at(uu);
        return sym.block_instance->refdes;
    }

    case ObjectType::COMPONENT: {
        const auto &comp = get_current_block()->components.at(uu);
        const auto comp_info = get_top_block()->get_component_info(comp, get_instance_path());
        return comp_info.refdes;
    }

    case ObjectType::BLOCK_INSTANCE:
        return get_current_block()->block_instances.at(uu).refdes;

    case ObjectType::POWER_SYMBOL:
        return get_current_schematic()->sheets.at(sh).power_symbols.at(uu).net->name;

    case ObjectType::BUS_RIPPER:
        return get_current_schematic()->sheets.at(sh).bus_rippers.at(uu).bus_member->net->name;

    case ObjectType::TEXT:
        if (sh)
            return get_current_schematic()->sheets.at(sh).texts.at(uu).text;
        else
            return Core::get_display_name(type, uu);

    case ObjectType::BLOCK_SYMBOL_PORT:
        return get_block_symbol().ports.at(uu).name;

    default:
        return Core::get_display_name(type, uu);
    }
}

void CoreSchematic::add_sheet()
{
    auto &sch = *get_current_schematic();
    auto &sheet = sch.add_sheet();
    sheet.pool_frame = sch.sheets.at(sheet_uuid).pool_frame;
    rebuild("add sheet");
}

void CoreSchematic::delete_sheet(const UUID &uu)
{
    auto sch = get_current_schematic();
    sch->delete_sheet(uu);
    if (sch->sheets.count(uu) == 0) { // got deleted
        if (sheet_uuid == uu) {       // deleted current sheet
            sheet_uuid = sch->get_sheet_at_index(1).uuid;
        }
        rebuild("delete sheet");
    }
}

void CoreSchematic::set_block_symbol_mode(const UUID &block)
{
    if (tool_is_active())
        return;
    if (blocks->blocks.count(block) == 0)
        return;
    instance_path.clear();
    block_uuid = block;
    sheet_uuid = UUID();
}

bool CoreSchematic::get_block_symbol_mode() const
{
    return sheet_uuid == UUID();
}

const UUID &CoreSchematic::get_current_block_uuid() const
{
    return block_uuid;
}

const UUID &CoreSchematic::get_top_block_uuid() const
{
    return blocks->top_block;
}

const BlocksSchematic &CoreSchematic::get_blocks() const
{
    return *blocks;
}

BlocksSchematic &CoreSchematic::get_blocks()
{
    return *blocks;
}

bool CoreSchematic::current_block_is_top() const
{
    return block_uuid == blocks->top_block;
}

void CoreSchematic::set_instance_path(const UUID &sheet, const UUID &block, const UUIDVec &path)
{
    if (tool_is_active())
        return;

    if (path.size() && !get_top_block()->block_instance_mappings.count(path)) {
        Logger::log_critical("instance mapping not found", Logger::Domain::SCHEMATIC, uuid_vec_to_string(path));
        return;
    }
    if (path.size() && get_top_block()->block_instance_mappings.at(path).block != block)
        return;

    if (blocks->blocks.count(block) == 0)
        return;

    if (blocks->blocks.at(block).schematic.sheets.count(sheet) == 0)
        return;

    const bool need_expand = (instance_path != path) || !sheet_uuid;
    block_uuid = block;
    sheet_uuid = sheet;
    instance_path = path;
    if (need_expand)
        blocks->blocks.at(block_uuid).schematic.expand(false, this);
}

bool CoreSchematic::in_hierarchy() const
{
    return current_block_is_top() || instance_path.size();
}

const UUIDVec &CoreSchematic::get_instance_path() const
{
    return instance_path;
}

const BlockInstanceMapping *CoreSchematic::get_block_instance_mapping() const
{
    if (instance_path.size()) {
        auto &map = blocks->get_top_block_item().block.block_instance_mappings.at(instance_path);
        assert(map.block == block_uuid);
        return &map;
    }
    else
        return nullptr;
}

BlockInstanceMapping *CoreSchematic::get_block_instance_mapping()
{
    return const_cast<BlockInstanceMapping *>(static_cast<const CoreSchematic *>(this)->get_block_instance_mapping());
}

unsigned int CoreSchematic::get_sheet_index(const class UUID &sheet) const
{
    if (in_hierarchy()) {
        return blocks->get_top_block_item().schematic.sheet_mapping.sheet_numbers.at(
                uuid_vec_append(instance_path, sheet));
    }
    else {
        return get_current_schematic()->sheets.at(sheet).index;
    }
}

unsigned int CoreSchematic::get_sheet_index_for_path(const class UUID &sheet, const UUIDVec &path) const
{
    return blocks->get_top_block_item().schematic.sheet_mapping.sheet_numbers.at(uuid_vec_append(path, sheet));
}

unsigned int CoreSchematic::get_sheet_total() const
{
    if (in_hierarchy())
        return blocks->get_top_block_item().schematic.sheet_mapping.sheet_total;
    else
        return get_current_schematic()->sheets.size();
}

Schematic &CoreSchematic::get_schematic_for_instance_path(const UUIDVec &path)
{
    Block *block = get_top_block();
    for (const auto &uu : path) {
        block = block->block_instances.at(uu).block;
    }
    return blocks->blocks.at(block->uuid).schematic;
}

void CoreSchematic::fix_current_block()
{
    auto &top_block = blocks->get_top_block_item();
    bool block_changed = false;
    if (blocks->blocks.count(block_uuid) == 0) {
        block_uuid = top_block.uuid;
        block_changed = true;
        instance_path.clear();
    }

    UUIDVec valid_instance_path;
    const Block *block = get_top_block();
    for (const auto &uu : instance_path) {
        if (block->block_instances.count(uu)) {
            valid_instance_path.push_back(uu);
            block = block->block_instances.at(uu).block;
        }
        else {
            instance_path = valid_instance_path;
            block_uuid = block->uuid;
            sheet_uuid = blocks->get_schematic(block_uuid).get_sheet_at_index(1).uuid;
            break;
        }
    }

    if (sheet_uuid || block_changed) {
        auto sch = get_current_schematic();
        if (sch->sheets.count(sheet_uuid) == 0)
            sheet_uuid = sch->get_sheet_at_index(1).uuid;
    }
}

void CoreSchematic::rebuild_internal(bool from_undo, const std::string &comment)
{
    clock_t begin = clock();
    auto &top_block = blocks->get_top_block_item();


    fix_current_block();


    top_block.block.create_instance_mappings();
    get_top_schematic()->update_sheet_mapping();
    for (auto &[uu, block] : blocks->blocks) {
        if (uu == top_block.uuid)
            continue;

        top_block.block.update_non_top(block.block);
    }
    for (auto &[uu, block] : blocks->blocks) {
        block.symbol.expand();
    }
    for (auto &[uu, block] : blocks->blocks) {
        IInstanceMappingProvider *prv = nullptr;
        if (uu == block_uuid)
            prv = this;
        block.schematic.expand(false, prv);
    }
    rebuild_finish(from_undo, comment);
    clock_t end = clock();
    double elapsed_secs = double(end - begin) / CLOCKS_PER_SEC;
    std::cout << "rebuild took " << elapsed_secs << std::endl;
}

const Sheet &CoreSchematic::get_canvas_data()
{
    return *get_sheet();
}

const BlockSymbol &CoreSchematic::get_canvas_data_block_symbol()
{
    return get_block_symbol();
}

class HistoryItemSchematic : public HistoryManager::HistoryItem {
public:
    HistoryItemSchematic(const BlocksSchematic &bl, const std::string &cm) : HistoryManager::HistoryItem(cm), blocks(bl)
    {
    }
    BlocksSchematic blocks;
};

std::unique_ptr<HistoryManager::HistoryItem> CoreSchematic::make_history_item(const std::string &comment)
{
    return std::make_unique<HistoryItemSchematic>(*blocks, comment);
}

void CoreSchematic::history_load(const HistoryManager::HistoryItem &it)
{
    const auto &x = dynamic_cast<const HistoryItemSchematic &>(it);
    blocks.emplace(x.blocks);
    // required to get refdes right
    fix_current_block();
    blocks->blocks.at(block_uuid).schematic.expand(false, this);
}

std::string CoreSchematic::get_history_comment_context() const
{
    if (get_block_symbol_mode()) {
        return "on block symbol " + get_current_block()->name;
    }
    else if (instance_path.size()) {
        return "on sheet " + get_top_block()->instance_path_to_string(instance_path) + "/" + get_sheet()->name;
    }
    else if (current_block_is_top()) {
        return "on sheet " + get_sheet()->name;
    }
    else {
        return "on sheet " + get_current_block()->name + "/" + get_sheet()->name;
    }
}

void CoreSchematic::reload_pool()
{

    PictureKeeper keeper;
    for (const auto &[uu_block, block] : blocks->blocks) {
        for (const auto &[uu_sheet, sheet] : block.schematic.sheets) {
            keeper.save(sheet.pictures);
        }
        keeper.save(block.symbol.pictures);
    }
    struct SerializedBlock : public Blocks::BlockItemInfo {
        SerializedBlock(const BlocksSchematic::BlockItemSchematic &b)
            : BlockItemInfo(b.uuid, b.block_filename, b.symbol_filename, b.schematic_filename),
              schematic(b.schematic.serialize()), block(b.block.serialize()), symbol(b.symbol.serialize())
        {
        }

        json schematic;
        json block;
        json symbol;
    };
    std::vector<SerializedBlock> blocks_serialized;
    for (const auto block : blocks->get_blocks_sorted()) {
        blocks_serialized.emplace_back(*block);
    }

    m_pool.clear();
    m_pool_caching.clear();

    blocks->blocks.clear();
    for (const auto &block : blocks_serialized) {
        blocks->blocks.emplace(
                std::piecewise_construct, std::forward_as_tuple(block.uuid),
                std::forward_as_tuple(block, block.block, block.symbol, block.schematic, m_pool_caching, *blocks));
    }

    for (auto &[uu_block, block] : blocks->blocks) {
        for (auto &[uu_sheet, sheet] : block.schematic.sheets) {
            keeper.restore(sheet.pictures);
        }
        keeper.restore(block.symbol.pictures);
    }

    bom_export_settings.update_refs(m_pool_caching);
    rebuild("reload pool");
    s_signal_can_undo_redo.emit();
}

std::pair<Coordi, Coordi> CoreSchematic::get_bbox()
{
    return get_sheet()->frame.get_bbox();
}

const std::string &CoreSchematic::get_filename() const
{
    return filenames.blocks;
}

void CoreSchematic::save(const std::string &suffix)
{
    auto &top = blocks->get_top_block_item();
    top.schematic.rules = rules;
    top.block.bom_export_settings = bom_export_settings;
    top.schematic.pdf_export_settings = pdf_export_settings;
    save_json_to_file(filenames.blocks + suffix, blocks->serialize());

    std::list<const std::map<UUID, Picture> *> pictures;
    std::list<const std::map<UUID, Picture> *> sym_pictures;
    const auto bp = fs::u8path(blocks->base_path);
    for (auto &[uu, block] : blocks->blocks) {
        const auto sch_filename = (bp / fs::u8path(block.schematic_filename + suffix)).u8string();
        ensure_parent_dir(sch_filename);
        save_json_to_file(sch_filename, block.schematic.serialize());

        const auto block_filename = (bp / fs::u8path(block.block_filename + suffix)).u8string();
        ensure_parent_dir(block_filename);
        save_json_to_file(block_filename, block.block.serialize());

        if (block.symbol_filename.size()) {
            const auto sym_filename = (bp / fs::u8path(block.symbol_filename + suffix)).u8string();
            ensure_parent_dir(sym_filename);
            save_json_to_file(sym_filename, block.symbol.serialize());
        }

        for (const auto &[uu_sh, sheet] : block.schematic.sheets) {
            pictures.push_back(&sheet.pictures);
        }
        sym_pictures.push_back(&block.symbol.pictures);
    }
    pictures_save(pictures, filenames.pictures_dir, "sch");
    pictures_save(sym_pictures, filenames.pictures_dir, "sym");
}


void CoreSchematic::delete_autosave()
{
    std::vector<fs::path> autosave_filenames;
    autosave_filenames.push_back(fs::u8path(filenames.blocks + autosave_suffix));
    const auto bp = fs::u8path(blocks->base_path);

    for (const auto &[uu, block] : blocks->blocks) {
        autosave_filenames.push_back(bp / fs::u8path(block.block_filename + autosave_suffix));
        autosave_filenames.push_back(bp / fs::u8path(block.schematic_filename + autosave_suffix));
        if (block.symbol_filename.size())
            autosave_filenames.push_back(bp / fs::u8path(block.symbol_filename + autosave_suffix));
    }

    for (const auto &path : autosave_filenames) {
        fs::remove(path);
    }
}

} // namespace horizon
