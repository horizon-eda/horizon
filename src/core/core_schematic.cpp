#include "core_schematic.hpp"
#include "core_properties.hpp"
#include "pool/part.hpp"
#include "util/util.hpp"
#include <algorithm>
#include "nlohmann/json.hpp"

namespace horizon {
CoreSchematic::CoreSchematic(const std::string &schematic_filename, const std::string &block_filename, Pool &pool)
    : block(Block::new_from_file(block_filename, pool)), sch(Schematic::new_from_file(schematic_filename, block, pool)),
      rules(sch.rules), bom_export_settings(block.bom_export_settings), pdf_export_settings(sch.pdf_export_settings),
      m_schematic_filename(schematic_filename), m_block_filename(block_filename)
{
    auto x = std::find_if(sch.sheets.cbegin(), sch.sheets.cend(), [](const auto &a) { return a.second.index == 1; });
    assert(x != sch.sheets.cend());
    sheet_uuid = x->first;
    m_pool = &pool;
    rebuild();
}

Junction *CoreSchematic::get_junction(const UUID &uu, bool work)
{
    auto &sheet = sch.sheets.at(sheet_uuid);
    return &sheet.junctions.at(uu);
}
SchematicSymbol *CoreSchematic::get_schematic_symbol(const UUID &uu, bool work)
{
    auto &sheet = sch.sheets.at(sheet_uuid);
    return &sheet.symbols.at(uu);
}
Text *CoreSchematic::get_text(const UUID &uu, bool work)
{
    auto &sheet = sch.sheets.at(sheet_uuid);
    return &sheet.texts.at(uu);
}
Schematic *CoreSchematic::get_schematic(bool work)
{
    return &sch;
}

Block *CoreSchematic::get_block(bool work)
{
    return get_schematic(work)->block;
}

LayerProvider *CoreSchematic::get_layer_provider()
{
    return get_sheet();
}

Sheet *CoreSchematic::get_sheet(bool work)
{
    return &sch.sheets.at(sheet_uuid);
}
Line *CoreSchematic::get_line(const UUID &uu, bool work)
{
    auto &sheet = sch.sheets.at(sheet_uuid);
    return &sheet.lines.at(uu);
}
Arc *CoreSchematic::get_arc(const UUID &uu, bool work)
{
    auto &sheet = sch.sheets.at(sheet_uuid);
    return &sheet.arcs.at(uu);
}

Junction *CoreSchematic::insert_junction(const UUID &uu, bool work)
{
    auto &sheet = sch.sheets.at(sheet_uuid);
    auto x = sheet.junctions.emplace(std::make_pair(uu, uu));
    return &(x.first->second);
}

LineNet *CoreSchematic::insert_line_net(const UUID &uu, bool work)
{
    auto &sheet = sch.sheets.at(sheet_uuid);
    auto x = sheet.net_lines.emplace(std::make_pair(uu, uu));
    return &(x.first->second);
}

void CoreSchematic::delete_junction(const UUID &uu, bool work)
{
    auto &sheet = sch.sheets.at(sheet_uuid);
    sheet.junctions.erase(uu);
}
void CoreSchematic::delete_line_net(const UUID &uu, bool work)
{
    auto &sheet = sch.sheets.at(sheet_uuid);
    sheet.net_lines.erase(uu);
}
void CoreSchematic::delete_schematic_symbol(const UUID &uu, bool work)
{
    auto &sheet = sch.sheets.at(sheet_uuid);
    sheet.symbols.erase(uu);
}
SchematicSymbol *CoreSchematic::insert_schematic_symbol(const UUID &uu, const Symbol *sym, bool work)
{
    auto &sheet = sch.sheets.at(sheet_uuid);
    auto x = sheet.symbols.emplace(std::make_pair(uu, SchematicSymbol{uu, sym}));
    return &(x.first->second);
    return nullptr;
}

Line *CoreSchematic::insert_line(const UUID &uu, bool work)
{
    auto &sheet = sch.sheets.at(sheet_uuid);
    auto x = sheet.lines.emplace(std::make_pair(uu, uu));
    return &(x.first->second);
}
void CoreSchematic::delete_line(const UUID &uu, bool work)
{
    auto &sheet = sch.sheets.at(sheet_uuid);
    sheet.lines.erase(uu);
}

Arc *CoreSchematic::insert_arc(const UUID &uu, bool work)
{
    auto &sheet = sch.sheets.at(sheet_uuid);
    auto x = sheet.arcs.emplace(std::make_pair(uu, uu));
    return &(x.first->second);
}
void CoreSchematic::delete_arc(const UUID &uu, bool work)
{
    auto &sheet = sch.sheets.at(sheet_uuid);
    sheet.arcs.erase(uu);
}

std::vector<LineNet *> CoreSchematic::get_net_lines(bool work)
{
    auto &sheet = sch.sheets.at(sheet_uuid);
    std::vector<LineNet *> r;
    for (auto &it : sheet.net_lines) {
        r.push_back(&it.second);
    }
    return r;
}
std::vector<NetLabel *> CoreSchematic::get_net_labels(bool work)
{
    auto &sheet = sch.sheets.at(sheet_uuid);
    std::vector<NetLabel *> r;
    for (auto &it : sheet.net_labels) {
        r.push_back(&it.second);
    }
    return r;
}

std::vector<Line *> CoreSchematic::get_lines(bool work)
{
    auto &sheet = sch.sheets.at(sheet_uuid);
    std::vector<Line *> r;
    for (auto &it : sheet.lines) {
        r.push_back(&it.second);
    }
    return r;
}

std::vector<Arc *> CoreSchematic::get_arcs(bool work)
{
    auto &sheet = sch.sheets.at(sheet_uuid);
    std::vector<Arc *> r;
    for (auto &it : sheet.arcs) {
        r.push_back(&it.second);
    }
    return r;
}

void CoreSchematic::delete_text(const UUID &uu, bool work)
{
    auto &sheet = sch.sheets.at(sheet_uuid);
    sheet.texts.erase(uu);
}
Text *CoreSchematic::insert_text(const UUID &uu, bool work)
{
    auto &sheet = sch.sheets.at(sheet_uuid);
    auto x = sheet.texts.emplace(std::make_pair(uu, uu));
    return &(x.first->second);
}

bool CoreSchematic::has_object_type(ObjectType ty) const
{
    switch (ty) {
    case ObjectType::JUNCTION:
    case ObjectType::SCHEMATIC_SYMBOL:
    case ObjectType::BUS_LABEL:
    case ObjectType::BUS_RIPPER:
    case ObjectType::NET_LABEL:
    case ObjectType::LINE_NET:
    case ObjectType::POWER_SYMBOL:
    case ObjectType::TEXT:
    case ObjectType::LINE:
    case ObjectType::ARC:
        return true;
        break;
    default:;
    }

    return false;
}

Rules *CoreSchematic::get_rules()
{
    return &rules;
}

bool CoreSchematic::get_property(ObjectType type, const UUID &uu, ObjectProperty::ID property, PropertyValue &value)
{
    if (Core::get_property(type, uu, property, value))
        return true;
    auto &sheet = sch.sheets.at(sheet_uuid);
    switch (type) {
    case ObjectType::NET: {
        auto net = &block.nets.at(uu);
        switch (property) {
        case ObjectProperty::ID::NAME:
            dynamic_cast<PropertyValueString &>(value).value = net->name;
            return true;

        case ObjectProperty::ID::NET_CLASS:
            dynamic_cast<PropertyValueUUID &>(value).value = net->net_class->uuid;
            return true;

        case ObjectProperty::ID::DIFFPAIR: {
            std::string s;
            if (net->diffpair) {
                s = (net->diffpair_master ? "Master: " : "Slave: ") + net->diffpair->name;
            }
            else {
                s = "None";
            }
            dynamic_cast<PropertyValueString &>(value).value = s;
            return true;
        }

        case ObjectProperty::ID::IS_POWER:
            dynamic_cast<PropertyValueBool &>(value).value = net->is_power;
            return true;

        default:
            return false;
        }
    } break;

    case ObjectType::NET_LABEL: {
        auto label = &sheet.net_labels.at(uu);
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

        default:
            return false;
        }
    } break;

    case ObjectType::COMPONENT: {
        auto comp = &block.components.at(uu);
        switch (property) {
        case ObjectProperty::ID::REFDES:
            dynamic_cast<PropertyValueString &>(value).value = comp->refdes;
            return true;

        case ObjectProperty::ID::VALUE:
            if (block.components.at(uu).part)
                dynamic_cast<PropertyValueString &>(value).value = comp->part->get_value();
            else
                dynamic_cast<PropertyValueString &>(value).value = comp->value;
            return true;

        case ObjectProperty::ID::MPN:
            if (block.components.at(uu).part)
                dynamic_cast<PropertyValueString &>(value).value = comp->part->get_MPN();
            else
                dynamic_cast<PropertyValueString &>(value).value = "<no part>";
            return true;

        default:
            return false;
        }
    } break;

    case ObjectType::SCHEMATIC_SYMBOL: {
        auto sym = &sheet.symbols.at(uu);
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
            dynamic_cast<PropertyValueString &>(value).value = sym->component->refdes + sym->gate->suffix;
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
    auto &sheet = sch.sheets.at(sheet_uuid);
    switch (type) {
    case ObjectType::COMPONENT: {
        auto comp = &block.components.at(uu);
        switch (property) {
        case ObjectProperty::ID::REFDES:
            comp->refdes = dynamic_cast<const PropertyValueString &>(value).value;
            break;

        case ObjectProperty::ID::VALUE:
            if (comp->part)
                return false;
            comp->value = dynamic_cast<const PropertyValueString &>(value).value;
            break;

        default:
            return false;
        }
    } break;

    case ObjectType::NET: {
        auto net = &block.nets.at(uu);
        switch (property) {
        case ObjectProperty::ID::NAME:
            net->name = dynamic_cast<const PropertyValueString &>(value).value;
            break;

        case ObjectProperty::ID::NET_CLASS: {
            net->net_class = &block.net_classes.at(dynamic_cast<const PropertyValueUUID &>(value).value);
        } break;

        case ObjectProperty::ID::IS_POWER:
            if (block.nets.at(uu).is_power_forced)
                return false;
            block.nets.at(uu).is_power = dynamic_cast<const PropertyValueBool &>(value).value;
            break;

        default:
            return false;
        }
    } break;

    case ObjectType::SCHEMATIC_SYMBOL: {
        auto sym = &sheet.symbols.at(uu);
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

        default:
            return false;
        }
    } break;

    case ObjectType::NET_LABEL: {
        auto label = &sheet.net_labels.at(uu);
        switch (property) {
        case ObjectProperty::ID::OFFSHEET_REFS:
            label->offsheet_refs = dynamic_cast<const PropertyValueBool &>(value).value;
            break;

        case ObjectProperty::ID::SIZE:
            label->size = dynamic_cast<const PropertyValueInt &>(value).value;
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

bool CoreSchematic::get_property_meta(ObjectType type, const UUID &uu, ObjectProperty::ID property, PropertyMeta &meta)
{
    if (Core::get_property_meta(type, uu, property, meta))
        return true;
    auto &sheet = sch.sheets.at(sheet_uuid);
    switch (type) {
    case ObjectType::NET:
        switch (property) {
        case ObjectProperty::ID::IS_POWER:
            meta.is_settable = !block.nets.at(uu).is_power_forced;
            return true;

        case ObjectProperty::ID::NET_CLASS: {
            PropertyMetaNetClasses &m = dynamic_cast<PropertyMetaNetClasses &>(meta);
            m.net_classes.clear();
            for (const auto &it : block.net_classes) {
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
        case ObjectProperty::ID::VALUE:
            meta.is_settable = block.components.at(uu).part == nullptr;
            return true;

        default:
            return false;
        }
        break;

    case ObjectType::SCHEMATIC_SYMBOL:
        switch (property) {
        case ObjectProperty::ID::EXPAND:
            meta.is_settable = sheet.symbols.at(uu).pool_symbol->can_expand;
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
    auto &sheet = sch.sheets.at(sh);
    switch (type) {
    case ObjectType::NET:
        return block.nets.at(uu).name;

    case ObjectType::LINE_NET: {
        const auto &li = sheet.net_lines.at(uu);
        return li.net ? li.net->name : "";
    }

    case ObjectType::JUNCTION: {
        const auto &ju = sheet.junctions.at(uu);
        return ju.net ? ju.net->name : "";
    }

    case ObjectType::NET_LABEL: {
        const auto &la = sheet.net_labels.at(uu);
        return la.junction->net ? la.junction->net->name : "";
    }

    case ObjectType::SCHEMATIC_SYMBOL:
        return sheet.symbols.at(uu).component->refdes + sheet.symbols.at(uu).gate->suffix;

    case ObjectType::COMPONENT:
        return block.components.at(uu).refdes;

    case ObjectType::POWER_SYMBOL:
        return sheet.power_symbols.at(uu).net->name;

    case ObjectType::BUS_RIPPER:
        return sheet.bus_rippers.at(uu).bus_member->net->name;

    default:
        return Core::get_display_name(type, uu);
    }
}

void CoreSchematic::add_sheet()
{
    auto uu = UUID::random();
    auto sheet_max = std::max_element(sch.sheets.begin(), sch.sheets.end(),
                                      [](const auto &p1, const auto &p2) { return p1.second.index < p2.second.index; });
    auto *sheet = &sch.sheets.emplace(uu, uu).first->second;
    sheet->index = sheet_max->second.index + 1;
    sheet->name = "sheet " + std::to_string(sheet->index);
    sheet->frame = sch.sheets.at(sheet_uuid).frame;
    rebuild();
}

void CoreSchematic::delete_sheet(const UUID &uu)
{
    if (sch.sheets.size() <= 1)
        return;
    if (sch.sheets.at(uu).symbols.size() > 0) // only delete empty sheets
        return;
    auto deleted_index = sch.sheets.at(uu).index;
    sch.sheets.erase(uu);
    for (auto &it : sch.sheets) {
        if (it.second.index > deleted_index) {
            it.second.index--;
        }
    }
    if (sheet_uuid == uu) { // deleted current sheet
        auto x = std::find_if(sch.sheets.begin(), sch.sheets.end(), [](auto e) { return e.second.index == 1; });
        sheet_uuid = x->first;
    }
    rebuild();
}

void CoreSchematic::set_sheet(const UUID &uu)
{
    if (tool_is_active())
        return;
    if (sch.sheets.count(uu) == 0)
        return;
    sheet_uuid = uu;
}

void CoreSchematic::rebuild(bool from_undo)
{
    sch.expand();
    Core::rebuild(from_undo);
}

const Sheet *CoreSchematic::get_canvas_data()
{
    return &sch.sheets.at(sheet_uuid);
}

void CoreSchematic::commit()
{
    set_needs_save(true);
}

void CoreSchematic::revert()
{
    history_load(history_current);
    reverted = true;
}

bool CoreSchematic::can_search_for_object_type(ObjectType ty) const
{
    switch (ty) {
    case ObjectType::SCHEMATIC_SYMBOL:
    case ObjectType::NET_LABEL:
    case ObjectType::POWER_SYMBOL:
    case ObjectType::BUS_RIPPER:
        return true;
        break;
    default:;
    }

    return false;
}

static void sort_search_results_schematic(std::list<Core::SearchResult> &results, const Core::SearchQuery &q,
                                          CoreSchematic *core)
{
    results.sort([core, q](const auto &a, const auto &b) {
        int index_a = core->get_schematic()->sheets.at(a.sheet).index;
        int index_b = core->get_schematic()->sheets.at(b.sheet).index;

        if (a.sheet == core->get_sheet()->uuid)
            index_a = -1;
        if (b.sheet == core->get_sheet()->uuid)
            index_b = -1;

        if (index_a > index_b)
            return false;
        if (index_a < index_b)
            return true;

        auto da = core->get_display_name(a.type, a.uuid, a.sheet);
        auto db = core->get_display_name(b.type, b.uuid, b.sheet);
        auto ina = !Coordf(a.location).in_range(q.area_visible.first, q.area_visible.second);
        auto inb = !Coordf(b.location).in_range(q.area_visible.first, q.area_visible.second);
        if (ina > inb)
            return false;
        else if (ina < inb)
            return true;

        if (a.type > b.type)
            return false;
        else if (a.type < b.type)
            return true;

        auto c = strcmp_natural(da, db);
        if (c > 0)
            return false;
        else if (c < 0)
            return true;

        if (a.location.x > b.location.x)
            return false;
        else if (a.location.x < b.location.x)
            return true;

        return a.location.y > b.location.y;
    });
}

std::list<Core::SearchResult> CoreSchematic::search(const SearchQuery &q)
{
    std::list<Core::SearchResult> results;
    if (q.query.size() == 0)
        return results;
    for (const auto &it_sheet : sch.sheets) {
        const auto &sheet = it_sheet.second;
        if (q.types.count(ObjectType::SCHEMATIC_SYMBOL)) {
            for (const auto &it : sheet.symbols) {
                if (it.second.component->refdes.find(q.query) != std::string::npos) {
                    results.emplace_back(ObjectType::SCHEMATIC_SYMBOL, it.first);
                    auto &x = results.back();
                    x.location = it.second.placement.shift;
                    x.sheet = sheet.uuid;
                    x.selectable = true;
                }
            }
        }
        if (q.types.count(ObjectType::NET_LABEL)) {
            for (const auto &it : sheet.net_labels) {
                if (it.second.junction->net && it.second.junction->net->name.find(q.query) != std::string::npos) {
                    results.emplace_back(ObjectType::NET_LABEL, it.first);
                    auto &x = results.back();
                    x.location = it.second.junction->position;
                    x.sheet = sheet.uuid;
                    x.selectable = true;
                }
            }
        }
        if (q.types.count(ObjectType::BUS_RIPPER)) {
            for (const auto &it : sheet.bus_rippers) {
                if (it.second.bus_member->net && it.second.bus_member->net->name.find(q.query) != std::string::npos) {
                    results.emplace_back(ObjectType::BUS_RIPPER, it.first);
                    auto &x = results.back();
                    x.location = it.second.get_connector_pos();
                    x.sheet = sheet.uuid;
                    x.selectable = true;
                }
            }
        }
        if (q.types.count(ObjectType::POWER_SYMBOL)) {
            for (const auto &it : sheet.power_symbols) {
                if (it.second.junction->net && it.second.junction->net->name.find(q.query) != std::string::npos) {
                    results.emplace_back(ObjectType::POWER_SYMBOL, it.first);
                    auto &x = results.back();
                    x.location = it.second.junction->position;
                    x.sheet = sheet.uuid;
                    x.selectable = true;
                }
            }
        }
    }
    sort_search_results_schematic(results, q, this);

    return results;
}

void CoreSchematic::history_push()
{
    history.push_back(std::make_unique<CoreSchematic::HistoryItem>(block, sch));
    auto x = dynamic_cast<CoreSchematic::HistoryItem *>(history.back().get());
    x->sch.block = &x->block;
    x->sch.update_refs();
}

void CoreSchematic::history_load(unsigned int i)
{
    auto x = dynamic_cast<CoreSchematic::HistoryItem *>(history.at(history_current).get());
    sch.~Schematic();
    new (&sch) Schematic(x->sch);
    block = x->block;
    sch.block = &block;
    sch.update_refs();
    s_signal_rebuilt.emit();
}

std::pair<Coordi, Coordi> CoreSchematic::get_bbox()
{
    return get_sheet()->frame.get_bbox();
}

const std::string &CoreSchematic::get_filename() const
{
    return m_schematic_filename;
}

void CoreSchematic::save()
{
    sch.rules = rules;
    block.bom_export_settings = bom_export_settings;
    sch.pdf_export_settings = pdf_export_settings;
    save_json_to_file(m_schematic_filename, sch.serialize());
    save_json_to_file(m_block_filename, block.serialize());
    set_needs_save(false);

    // json j = block.serialize();
    // std::cout << std::setw(4) << j << std::endl;
}
} // namespace horizon
