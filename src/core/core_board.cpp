#include "core_board.hpp"
#include "board/board_layers.hpp"
#include "core_properties.hpp"
#include "util/util.hpp"
#include "nlohmann/json.hpp"
#include <giomm/file.h>
#include <glibmm/fileutils.h>
#include <glibmm/miscutils.h>
#include "pool/ipool.hpp"
#include "pool/part.hpp"
#include "util/picture_load.hpp"
#include "blocks/iblock_provider.hpp"
#include "blocks/blocks.hpp"
#include <iostream>

namespace horizon {

class NoneBlockProvider : public IBlockProvider {
public:
    Block &get_block(const UUID &uu) override
    {
        throw std::runtime_error("no blocks");
    }

    Block &get_top_block() override
    {
        throw std::runtime_error("no blocks");
    }

    std::map<UUID, Block *> get_blocks() override
    {
        throw std::runtime_error("no blocks");
    }

    static auto &get()
    {
        static NoneBlockProvider inst;
        return inst;
    }
};

static Block get_flattend_block(const std::string &blocks_filename, IPool &pool)
{
    auto blocks = Blocks::new_from_file(blocks_filename, pool);
    return blocks.get_top_block_item().block.flatten();
}

CoreBoard::CoreBoard(const Filenames &fns, IPool &pool, IPool &pool_caching)
    : Core(pool, &pool_caching), block(get_flattend_block(fns.blocks, pool_caching)),
      brd(Board::new_from_file(fns.board, *block, pool_caching)), rules(brd->rules),
      gerber_output_settings(brd->gerber_output_settings), odb_output_settings(brd->odb_output_settings),
      pdf_export_settings(brd->pdf_export_settings), step_export_settings(brd->step_export_settings),
      pnp_export_settings(brd->pnp_export_settings), grid_settings(brd->grid_settings), colors(brd->colors),
      filenames(fns)
{
    brd->load_pictures(filenames.pictures_dir);
    if (Glib::file_test(filenames.planes, Glib::FILE_TEST_IS_REGULAR))
        brd->load_planes_from_file(filenames.planes);
    rebuild("init");
}

void CoreBoard::reload_netlist()
{
    block = get_flattend_block(filenames.blocks, m_pool_caching);
    brd->block = &*block;
    brd->update_refs();
    for (auto it = brd->packages.begin(); it != brd->packages.end();) {
        if (it->second.component == nullptr || it->second.component->part == nullptr) {
            for (auto &it_txt : it->second.texts) {
                brd->texts.erase(it_txt->uuid);
            }
            brd->packages.erase(it++);
        }
        else {
            it++;
        }
    }

    for (auto it = brd->net_ties.begin(); it != brd->net_ties.end();) {
        if (it->second.net_tie == nullptr) {
            brd->net_ties.erase(it++);
        }
        else {
            it++;
        }
    }

    // delete planes with deleted nets
    for (auto it = brd->planes.begin(); it != brd->planes.end();) {
        const auto &plane = it->second;
        if (!block->nets.count(plane.net.uuid)) { // net is gone
            plane.polygon->usage = nullptr;
            brd->planes.erase(it++);
        }
        else {
            it++;
        }
    }
    brd->update_refs();

    for (auto it = brd->tracks.begin(); it != brd->tracks.end();) {
        bool del = false;
        for (auto &it_ft : {it->second.from, it->second.to}) {
            if (it_ft.package.uuid) {
                if (brd->packages.count(it_ft.package.uuid) == 0) {
                    del = true;
                }
                else {
                    if (it_ft.package->component->part->pad_map.count(it_ft.pad.uuid) == 0) {
                        del = true;
                    }
                }
            }
        }

        if (del) {
            brd->tracks.erase(it++);
        }
        else {
            it++;
        }
    }
    brd->update_refs();
    rules.cleanup(&*block);
    brd->expand_flags = Board::EXPAND_PROPAGATE_NETS | Board::EXPAND_ALL_AIRWIRES | Board::EXPAND_PACKAGES;
    rebuild("reload netlist");
}

bool CoreBoard::get_property(ObjectType type, const UUID &uu, ObjectProperty::ID property, PropertyValue &value)
{
    if (Core::get_property(type, uu, property, value))
        return true;
    switch (type) {
    case ObjectType::BOARD_PACKAGE: {
        auto pkg = &brd->packages.at(uu);
        switch (property) {
        case ObjectProperty::ID::FLIPPED:
            dynamic_cast<PropertyValueBool &>(value).value = pkg->flip;
            return true;

        case ObjectProperty::ID::FIXED:
            dynamic_cast<PropertyValueBool &>(value).value = pkg->fixed;
            return true;

        case ObjectProperty::ID::REFDES:
            dynamic_cast<PropertyValueString &>(value).value = pkg->component->refdes;
            return true;

        case ObjectProperty::ID::NAME:
            dynamic_cast<PropertyValueString &>(value).value = pkg->package.name;
            return true;

        case ObjectProperty::ID::VALUE:
            dynamic_cast<PropertyValueString &>(value).value = pkg->component->part->get_value();
            return true;

        case ObjectProperty::ID::MPN:
            dynamic_cast<PropertyValueString &>(value).value = pkg->component->part->get_MPN();
            return true;

        case ObjectProperty::ID::ALTERNATE_PACKAGE:
            dynamic_cast<PropertyValueUUID &>(value).value =
                    pkg->alternate_package ? pkg->alternate_package->uuid : UUID();
            return true;

        case ObjectProperty::ID::POSITION_X:
        case ObjectProperty::ID::POSITION_Y:
        case ObjectProperty::ID::ANGLE:
            get_placement(pkg->placement, value, property);
            return true;

        case ObjectProperty::ID::OMIT_SILKSCREEN:
            dynamic_cast<PropertyValueBool &>(value).value = pkg->omit_silkscreen;
            return true;

        case ObjectProperty::ID::OMIT_OUTLINE:
            dynamic_cast<PropertyValueBool &>(value).value = pkg->omit_outline;
            return true;

        default:
            return false;
        }

    } break;

    case ObjectType::BOARD_PANEL: {
        auto &panel = brd->board_panels.at(uu);
        switch (property) {
        case ObjectProperty::ID::OMIT_OUTLINE:
            dynamic_cast<PropertyValueBool &>(value).value = panel.omit_outline;
            return true;

        case ObjectProperty::ID::NAME:
            dynamic_cast<PropertyValueString &>(value).value = panel.included_board->get_name();
            return true;

        case ObjectProperty::ID::POSITION_X:
        case ObjectProperty::ID::POSITION_Y:
        case ObjectProperty::ID::ANGLE:
            get_placement(panel.placement, value, property);
            return true;

        default:
            return false;
        }

    } break;

    case ObjectType::TRACK: {
        auto track = &brd->tracks.at(uu);
        switch (property) {
        case ObjectProperty::ID::WIDTH_FROM_RULES:
            dynamic_cast<PropertyValueBool &>(value).value = track->width_from_rules;
            return true;

        case ObjectProperty::ID::LOCKED:
            dynamic_cast<PropertyValueBool &>(value).value = track->locked;
            return true;

        case ObjectProperty::ID::WIDTH:
            dynamic_cast<PropertyValueInt &>(value).value = track->width;
            return true;

        case ObjectProperty::ID::LAYER:
            dynamic_cast<PropertyValueInt &>(value).value = track->layer;
            return true;

        case ObjectProperty::ID::NAME:
            dynamic_cast<PropertyValueString &>(value).value = track->net ? (track->net->name) : "<no net>";
            return true;

        case ObjectProperty::ID::NET_CLASS:
            dynamic_cast<PropertyValueString &>(value).value = track->net ? (track->net->net_class->name) : "<no net>";
            return true;

        default:
            return false;
        }
    } break;

    case ObjectType::BOARD_NET_TIE: {
        const auto &tie = brd->net_ties.at(uu);
        switch (property) {
        case ObjectProperty::ID::WIDTH_FROM_RULES:
            dynamic_cast<PropertyValueBool &>(value).value = tie.width_from_rules;
            return true;

        case ObjectProperty::ID::WIDTH:
            dynamic_cast<PropertyValueInt &>(value).value = tie.width;
            return true;

        case ObjectProperty::ID::LAYER:
            dynamic_cast<PropertyValueInt &>(value).value = tie.layer;
            return true;

        case ObjectProperty::ID::NAME:
            dynamic_cast<PropertyValueString &>(value).value =
                    tie.net_tie->net_primary->name + "\n" + tie.net_tie->net_secondary->name;
            return true;


        default:
            return false;
        }
    } break;

    case ObjectType::VIA: {
        auto via = &brd->vias.at(uu);
        switch (property) {
        case ObjectProperty::ID::FROM_RULES:
            dynamic_cast<PropertyValueBool &>(value).value = via->from_rules;
            return true;

        case ObjectProperty::ID::LOCKED:
            dynamic_cast<PropertyValueBool &>(value).value = via->locked;
            return true;

        case ObjectProperty::ID::NAME: {
            std::string s;
            if (via->junction->net) {
                s = via->junction->net->name;
                if (via->net_set)
                    s += " (set)";
            }
            else {
                s = "<no net>";
            }
            dynamic_cast<PropertyValueString &>(value).value = s;
            return true;
        }

        case ObjectProperty::ID::SPAN:
            dynamic_cast<PropertyValueLayerRange &>(value).value = via->span;
            return true;

        case ObjectProperty::ID::POSITION_X:
            dynamic_cast<PropertyValueInt &>(value).value = via->junction->position.x;
            return true;

        case ObjectProperty::ID::POSITION_Y:
            dynamic_cast<PropertyValueInt &>(value).value = via->junction->position.y;
            return true;

        default:
            return false;
        }
    } break;

    case ObjectType::PLANE: {
        auto plane = &brd->planes.at(uu);
        switch (property) {
        case ObjectProperty::ID::FROM_RULES:
            dynamic_cast<PropertyValueBool &>(value).value = plane->from_rules;
            return true;

        case ObjectProperty::ID::WIDTH:
            dynamic_cast<PropertyValueInt &>(value).value = plane->settings.min_width;
            return true;

        case ObjectProperty::ID::NAME:
            dynamic_cast<PropertyValueString &>(value).value =
                    plane->net ? brd->block->get_net_name(plane->net->uuid) : "<no net>";
            return true;

        case ObjectProperty::ID::PRIORITY:
            dynamic_cast<PropertyValueInt &>(value).value = plane->priority;
            return true;

        default:
            return false;
        }
    } break;

    case ObjectType::BOARD_HOLE: {
        auto hole = &brd->holes.at(uu);
        switch (property) {
        case ObjectProperty::ID::NAME:
            dynamic_cast<PropertyValueString &>(value).value = hole->pool_padstack->name;
            return true;

        case ObjectProperty::ID::VALUE: {
            std::string net = "<no net>";
            if (hole->net)
                net = hole->net->name;
            dynamic_cast<PropertyValueString &>(value).value = net;
            return true;
        }

        case ObjectProperty::ID::POSITION_X:
        case ObjectProperty::ID::POSITION_Y:
        case ObjectProperty::ID::ANGLE:
            get_placement(hole->placement, value, property);
            return true;

        case ObjectProperty::ID::PAD_TYPE: {
            const auto ps = brd->holes.at(uu).pool_padstack;
            std::string pad_type;
            switch (ps->type) {
            case Padstack::Type::MECHANICAL:
                pad_type = "Mechanical (NPTH)";
                break;
            case Padstack::Type::HOLE:
                pad_type = "Hole (PTH)";
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

    case ObjectType::BOARD_DECAL: {
        const auto &decal = brd->decals.at(uu);
        switch (property) {
        case ObjectProperty::ID::NAME:
            dynamic_cast<PropertyValueString &>(value).value = decal.get_decal().name;
            return true;

        case ObjectProperty::ID::POSITION_X:
        case ObjectProperty::ID::POSITION_Y:
        case ObjectProperty::ID::ANGLE:
            get_placement(decal.placement, value, property);
            return true;

        case ObjectProperty::ID::FLIPPED:
            dynamic_cast<PropertyValueBool &>(value).value = decal.get_flip();
            return true;

        case ObjectProperty::ID::SCALE:
            dynamic_cast<PropertyValueDouble &>(value).value = decal.get_scale();
            return true;

        default:
            return false;
        }
    } break;

    default:
        return false;
    }
}

bool CoreBoard::set_property(ObjectType type, const UUID &uu, ObjectProperty::ID property, const PropertyValue &value)
{
    if (Core::set_property(type, uu, property, value))
        return true;
    switch (type) {
    case ObjectType::BOARD_PACKAGE: {
        auto pkg = &brd->packages.at(uu);
        switch (property) {
        case ObjectProperty::ID::FLIPPED:
            pkg->flip = dynamic_cast<const PropertyValueBool &>(value).value;
            pkg->update(*brd);
            brd->update_refs();
            brd->expand_flags |= Board::EXPAND_AIRWIRES;
            {
                const auto n = pkg->get_nets();
                brd->airwires_expand.insert(n.begin(), n.end());
            }
            break;

        case ObjectProperty::ID::FIXED:
            pkg->fixed = dynamic_cast<const PropertyValueBool &>(value).value;
            break;

        case ObjectProperty::ID::ALTERNATE_PACKAGE: {
            const auto alt_uuid = dynamic_cast<const PropertyValueUUID &>(value).value;
            if (!alt_uuid) {
                pkg->alternate_package = nullptr;
            }
            else if (m_pool.get_alternate_packages(pkg->pool_package->uuid).count(alt_uuid)
                     != 0) { // see if really an alt package for pkg
                pkg->alternate_package = m_pool_caching.get_package(alt_uuid);
            }
            pkg->update(*brd);
            brd->update_refs();
            brd->expand_flags |= Board::EXPAND_AIRWIRES;
            {
                const auto n = pkg->get_nets();
                brd->airwires_expand.insert(n.begin(), n.end());
            }
        } break;

        case ObjectProperty::ID::POSITION_X:
        case ObjectProperty::ID::POSITION_Y:
        case ObjectProperty::ID::ANGLE: {
            if (pkg->fixed)
                return false;
            const auto shift_before = pkg->placement.shift;
            set_placement(pkg->placement, value, property);
            auto delta = pkg->placement.shift - shift_before;
            for (auto &text : pkg->texts) {
                text->placement.shift += delta;
            }
            brd->expand_flags |= Board::EXPAND_AIRWIRES;
            {
                const auto n = pkg->get_nets();
                brd->airwires_expand.insert(n.begin(), n.end());
            }
        } break;

        case ObjectProperty::ID::OMIT_SILKSCREEN:
            pkg->omit_silkscreen = dynamic_cast<const PropertyValueBool &>(value).value;
            break;

        case ObjectProperty::ID::OMIT_OUTLINE:
            pkg->omit_outline = dynamic_cast<const PropertyValueBool &>(value).value;
            break;

        default:
            return false;
        }
    } break;

    case ObjectType::BOARD_PANEL: {
        auto &panel = brd->board_panels.at(uu);
        switch (property) {

        case ObjectProperty::ID::OMIT_OUTLINE:
            panel.omit_outline = dynamic_cast<const PropertyValueBool &>(value).value;
            break;

        case ObjectProperty::ID::POSITION_X:
        case ObjectProperty::ID::POSITION_Y:
        case ObjectProperty::ID::ANGLE: {
            set_placement(panel.placement, value, property);
        } break;

        default:
            return false;
        }
    } break;

    case ObjectType::TRACK: {
        auto track = &brd->tracks.at(uu);
        switch (property) {
        case ObjectProperty::ID::WIDTH_FROM_RULES:
            track->width_from_rules = dynamic_cast<const PropertyValueBool &>(value).value;
            break;

        case ObjectProperty::ID::LOCKED:
            track->locked = dynamic_cast<const PropertyValueBool &>(value).value;
            break;

        case ObjectProperty::ID::WIDTH:
            if (track->width_from_rules)
                return false;
            track->width = dynamic_cast<const PropertyValueInt &>(value).value;
            break;

        case ObjectProperty::ID::LAYER:
            track->layer = dynamic_cast<const PropertyValueInt &>(value).value;
            break;

        default:
            return false;
        }
    } break;

    case ObjectType::BOARD_NET_TIE: {
        auto &tie = brd->net_ties.at(uu);
        switch (property) {
        case ObjectProperty::ID::WIDTH_FROM_RULES:
            tie.width_from_rules = dynamic_cast<const PropertyValueBool &>(value).value;
            break;

        case ObjectProperty::ID::WIDTH:
            if (tie.width_from_rules)
                return false;
            tie.width = dynamic_cast<const PropertyValueInt &>(value).value;
            break;

        case ObjectProperty::ID::LAYER:
            tie.layer = dynamic_cast<const PropertyValueInt &>(value).value;
            break;

        default:
            return false;
        }
    } break;

    case ObjectType::VIA: {
        auto via = &brd->vias.at(uu);
        switch (property) {
        case ObjectProperty::ID::FROM_RULES:
            via->from_rules = dynamic_cast<const PropertyValueBool &>(value).value;
            break;

        case ObjectProperty::ID::LOCKED:
            via->locked = dynamic_cast<const PropertyValueBool &>(value).value;
            break;

        case ObjectProperty::ID::SPAN:
            via->span = dynamic_cast<const PropertyValueLayerRange &>(value).value;
            via->expand(*brd);
            break;

        case ObjectProperty::ID::POSITION_X:
            via->junction->position.x = dynamic_cast<const PropertyValueInt &>(value).value;
            return true;

        case ObjectProperty::ID::POSITION_Y:
            via->junction->position.y = dynamic_cast<const PropertyValueInt &>(value).value;
            return true;

        default:
            return false;
        }
    } break;

    case ObjectType::PLANE: {
        auto plane = &brd->planes.at(uu);
        switch (property) {
        case ObjectProperty::ID::FROM_RULES:
            plane->from_rules = dynamic_cast<const PropertyValueBool &>(value).value;
            break;

        case ObjectProperty::ID::WIDTH:
            if (plane->from_rules)
                return false;
            plane->settings.min_width = dynamic_cast<const PropertyValueInt &>(value).value;
            break;

        case ObjectProperty::ID::PRIORITY:
            plane->priority = dynamic_cast<const PropertyValueInt &>(value).value;
            break;

        default:
            return false;
        }
    } break;

    case ObjectType::BOARD_HOLE: {
        auto hole = &brd->holes.at(uu);
        switch (property) {
        case ObjectProperty::ID::POSITION_X:
        case ObjectProperty::ID::POSITION_Y:
        case ObjectProperty::ID::ANGLE:
            set_placement(hole->placement, value, property);
            break;

        default:
            return false;
        }
    } break;

    case ObjectType::BOARD_DECAL: {
        auto &decal = brd->decals.at(uu);
        switch (property) {
        case ObjectProperty::ID::POSITION_X:
        case ObjectProperty::ID::POSITION_Y:
        case ObjectProperty::ID::ANGLE:
            set_placement(decal.placement, value, property);
            break;

        case ObjectProperty::ID::FLIPPED:
            decal.set_flip(dynamic_cast<const PropertyValueBool &>(value).value, *brd);
            break;

        case ObjectProperty::ID::SCALE:
            decal.set_scale(dynamic_cast<const PropertyValueDouble &>(value).value);
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

bool CoreBoard::get_property_meta(ObjectType type, const UUID &uu, ObjectProperty::ID property, PropertyMeta &meta)
{
    if (Core::get_property_meta(type, uu, property, meta))
        return true;
    switch (type) {
    case ObjectType::BOARD_PACKAGE: {
        auto pkg = &brd->packages.at(uu);
        switch (property) {
        case ObjectProperty::ID::ALTERNATE_PACKAGE: {
            PropertyMetaNetClasses &m = dynamic_cast<PropertyMetaNetClasses &>(meta);
            m.net_classes.clear();
            m.net_classes.emplace(UUID(), pkg->pool_package->name + " (default)");
            for (const auto &it : m_pool.get_alternate_packages(pkg->pool_package->uuid)) {
                m.net_classes.emplace(it, m_pool.get_package(it)->name);
            }
            return true;
        }

        case ObjectProperty::ID::POSITION_X:
        case ObjectProperty::ID::POSITION_Y:
        case ObjectProperty::ID::ANGLE:
        case ObjectProperty::ID::FLIPPED:
            meta.is_settable = !pkg->fixed;
            return true;

        default:
            return false;
        }
    } break;

    case ObjectType::TRACK: {
        auto track = &brd->tracks.at(uu);
        switch (property) {
        case ObjectProperty::ID::WIDTH:
            meta.is_settable = !track->width_from_rules;
            return true;

        case ObjectProperty::ID::LAYER:
            layers_to_meta(meta);
            return true;

        default:
            return false;
        }
    } break;

    case ObjectType::BOARD_NET_TIE: {
        auto &tie = brd->net_ties.at(uu);
        switch (property) {
        case ObjectProperty::ID::WIDTH:
            meta.is_settable = !tie.width_from_rules;
            return true;

        case ObjectProperty::ID::LAYER:
            layers_to_meta(meta);
            return true;

        default:
            return false;
        }
    } break;


    case ObjectType::PLANE: {
        auto plane = &brd->planes.at(uu);
        switch (property) {
        case ObjectProperty::ID::WIDTH:
            meta.is_settable = !plane->from_rules;
            return true;

        default:
            return false;
        }
    } break;

    case ObjectType::VIA: {
        switch (property) {
        case ObjectProperty::ID::SPAN:
            layers_to_meta(meta);
            return true;

        default:
            return false;
        }
    } break;

    default:
        return false;
    }
}

void CoreBoard::rebuild_internal(bool from_undo, const std::string &comment)
{
    clock_t begin = clock();
    brd->expand_some();
    rebuild_finish(from_undo, comment);
    clock_t end = clock();
    double elapsed_secs = double(end - begin) / CLOCKS_PER_SEC;
    std::cout << "rebuild took " << elapsed_secs << std::endl;
}

LayerProvider &CoreBoard::get_layer_provider()
{
    return *brd;
}

const Board &CoreBoard::get_canvas_data()
{
    return *brd;
}

Board *CoreBoard::get_board()
{
    return &*brd;
}

const Board *CoreBoard::get_board() const
{
    return &*brd;
}

Block *CoreBoard::get_top_block()
{
    return get_board()->block;
}

Rules *CoreBoard::get_rules()
{
    return &rules;
}

void CoreBoard::update_rules()
{
    brd->rules = rules;
}

class HistoryItemBoard : public HistoryManager::HistoryItem {
public:
    HistoryItemBoard(const Block &b, const Board &r, const std::string &cm)
        : HistoryManager::HistoryItem(cm), block(b), brd(shallow_copy, r)
    {
    }
    Block block;
    Board brd;
};

std::unique_ptr<HistoryManager::HistoryItem> CoreBoard::make_history_item(const std::string &comment)
{
    return std::make_unique<HistoryItemBoard>(*block, *brd, comment);
}

void CoreBoard::history_load(const HistoryManager::HistoryItem &it)
{
    const auto &x = dynamic_cast<const HistoryItemBoard &>(it);
    brd.emplace(shallow_copy, x.brd);
    block.emplace(x.block);
    brd->block = &*block;
    brd->update_refs();
    brd->expand_flags = Board::EXPAND_PACKAGES | Board::EXPAND_VIAS;
    brd->expand_some();
}

void CoreBoard::reload_pool()
{
    PictureKeeper keeper;
    keeper.save(brd->pictures);
    const auto brd_j = brd->serialize();
    const auto block_j = block->serialize();
    const auto planes_j = brd->serialize_planes();
    m_pool.clear();
    m_pool_caching.clear();
    block.emplace(block->uuid, block_j, m_pool_caching, NoneBlockProvider::get());
    brd.emplace(brd->uuid, brd_j, *block, m_pool_caching, Glib::path_get_dirname(filenames.board));
    brd->load_planes(planes_j);
    keeper.restore(brd->pictures);
    rebuild("reload pool");
    s_signal_can_undo_redo.emit();
}

json CoreBoard::get_meta()
{
    return get_meta_from_file(filenames.board);
}

std::pair<Coordi, Coordi> CoreBoard::get_bbox()
{
    return brd->get_bbox();
}

const std::string &CoreBoard::get_filename() const
{
    return filenames.board;
}

void CoreBoard::save(const std::string &suffix)
{
    brd->rules = rules;
    brd->gerber_output_settings = gerber_output_settings;
    brd->odb_output_settings = odb_output_settings;
    brd->pdf_export_settings = pdf_export_settings;
    brd->step_export_settings = step_export_settings;
    brd->pnp_export_settings = pnp_export_settings;
    brd->grid_settings = grid_settings;
    brd->colors = colors;
    auto j = brd->serialize();
    save_json_to_file(filenames.board + suffix, j);
    save_json_to_file(filenames.planes + suffix, brd->serialize_planes());
    brd->save_pictures(filenames.pictures_dir);
}

void CoreBoard::delete_autosave()
{
    if (Glib::file_test(filenames.board + autosave_suffix, Glib::FILE_TEST_IS_REGULAR))
        Gio::File::create_for_path(filenames.board + autosave_suffix)->remove();
}

} // namespace horizon
