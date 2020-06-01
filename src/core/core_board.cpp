#include "core_board.hpp"
#include "board/board_layers.hpp"
#include "core_properties.hpp"
#include "pool/part.hpp"
#include "util/util.hpp"
#include <algorithm>
#include "nlohmann/json.hpp"
#include <giomm/file.h>
#include <glibmm/fileutils.h>

namespace horizon {
CoreBoard::CoreBoard(const std::string &board_filename, const std::string &block_filename, const std::string &via_dir,
                     const std::string &pictures_dir, Pool &pool)
    : via_padstack_provider(via_dir, pool), block(Block::new_from_file(block_filename, pool)),
      brd(Board::new_from_file(board_filename, block, pool, via_padstack_provider)), rules(brd.rules),
      fab_output_settings(brd.fab_output_settings), pdf_export_settings(brd.pdf_export_settings),
      step_export_settings(brd.step_export_settings), pnp_export_settings(brd.pnp_export_settings), colors(brd.colors),
      m_board_filename(board_filename), m_block_filename(block_filename), m_pictures_dir(pictures_dir)
{
    m_pool = &pool;
    brd.load_pictures(pictures_dir);
    rebuild();
}

void CoreBoard::reload_netlist()
{
    block = Block::new_from_file(m_block_filename, *m_pool);
    brd.block = &block;
    brd.update_refs();
    for (auto it = brd.packages.begin(); it != brd.packages.end();) {
        if (it->second.component == nullptr || it->second.component->part == nullptr) {
            for (auto &it_txt : it->second.texts) {
                brd.texts.erase(it_txt->uuid);
            }
            brd.packages.erase(it++);
        }
        else {
            it++;
        }
    }
    brd.update_refs();
    for (auto it = brd.tracks.begin(); it != brd.tracks.end();) {
        bool del = false;
        for (auto &it_ft : {it->second.from, it->second.to}) {
            if (it_ft.package.uuid) {
                if (brd.packages.count(it_ft.package.uuid) == 0) {
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
            brd.tracks.erase(it++);
        }
        else {
            it++;
        }
    }
    brd.update_refs();
    rules.cleanup(&block);

    rebuild();
}

bool CoreBoard::get_property(ObjectType type, const UUID &uu, ObjectProperty::ID property, PropertyValue &value)
{
    if (Core::get_property(type, uu, property, value))
        return true;
    switch (type) {
    case ObjectType::BOARD_PACKAGE: {
        auto pkg = &brd.packages.at(uu);
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
        auto &panel = brd.board_panels.at(uu);
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
        auto track = &brd.tracks.at(uu);
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

    case ObjectType::VIA: {
        auto via = &brd.vias.at(uu);
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

        default:
            return false;
        }
    } break;

    case ObjectType::PLANE: {
        auto plane = &brd.planes.at(uu);
        switch (property) {
        case ObjectProperty::ID::FROM_RULES:
            dynamic_cast<PropertyValueBool &>(value).value = plane->from_rules;
            return true;

        case ObjectProperty::ID::WIDTH:
            dynamic_cast<PropertyValueInt &>(value).value = plane->settings.min_width;
            return true;

        case ObjectProperty::ID::NAME:
            dynamic_cast<PropertyValueString &>(value).value = plane->net ? (plane->net->name) : "<no net>";
            return true;

        default:
            return false;
        }
    } break;

    case ObjectType::BOARD_HOLE: {
        auto hole = &brd.holes.at(uu);
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
            const auto ps = brd.holes.at(uu).pool_padstack;
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
        auto pkg = &brd.packages.at(uu);
        switch (property) {
        case ObjectProperty::ID::FLIPPED:
            pkg->flip = dynamic_cast<const PropertyValueBool &>(value).value;
            break;

        case ObjectProperty::ID::FIXED:
            pkg->fixed = dynamic_cast<const PropertyValueBool &>(value).value;
            break;

        case ObjectProperty::ID::ALTERNATE_PACKAGE: {
            const auto alt_uuid = dynamic_cast<const PropertyValueUUID &>(value).value;
            if (!alt_uuid) {
                pkg->alternate_package = nullptr;
            }
            else if (m_pool->get_alternate_packages(pkg->pool_package->uuid).count(alt_uuid)
                     != 0) { // see if really an alt package for pkg
                pkg->alternate_package = m_pool->get_package(alt_uuid);
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
        auto &panel = brd.board_panels.at(uu);
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
        auto track = &brd.tracks.at(uu);
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

    case ObjectType::VIA: {
        auto via = &brd.vias.at(uu);
        switch (property) {
        case ObjectProperty::ID::FROM_RULES:
            via->from_rules = dynamic_cast<const PropertyValueBool &>(value).value;
            break;

        case ObjectProperty::ID::LOCKED:
            via->locked = dynamic_cast<const PropertyValueBool &>(value).value;
            break;

        default:
            return false;
        }
    } break;

    case ObjectType::PLANE: {
        auto plane = &brd.planes.at(uu);
        switch (property) {
        case ObjectProperty::ID::FROM_RULES:
            plane->from_rules = dynamic_cast<const PropertyValueBool &>(value).value;
            break;

        case ObjectProperty::ID::WIDTH:
            if (plane->from_rules)
                return false;
            plane->settings.min_width = dynamic_cast<const PropertyValueInt &>(value).value;
            break;

        default:
            return false;
        }
    } break;

    case ObjectType::BOARD_HOLE: {
        auto hole = &brd.holes.at(uu);
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

    default:
        return false;
    }
    if (!property_transaction) {
        rebuild(false);
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
        auto pkg = &brd.packages.at(uu);
        switch (property) {
        case ObjectProperty::ID::ALTERNATE_PACKAGE: {
            PropertyMetaNetClasses &m = dynamic_cast<PropertyMetaNetClasses &>(meta);
            m.net_classes.clear();
            m.net_classes.emplace(UUID(), pkg->pool_package->name + " (default)");
            for (const auto &it : m_pool->get_alternate_packages(pkg->pool_package->uuid)) {
                m.net_classes.emplace(it, m_pool->get_package(it)->name);
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
        auto track = &brd.tracks.at(uu);
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

    case ObjectType::PLANE: {
        auto plane = &brd.planes.at(uu);
        switch (property) {
        case ObjectProperty::ID::WIDTH:
            meta.is_settable = !plane->from_rules;
            return true;

        default:
            return false;
        }
    } break;

    default:
        return false;
    }
}

void CoreBoard::rebuild(bool from_undo)
{
    clock_t begin = clock();
    brd.expand();
    Core::rebuild(from_undo);
    clock_t end = clock();
    double elapsed_secs = double(end - begin) / CLOCKS_PER_SEC;
    std::cout << "rebuild took " << elapsed_secs << std::endl;
}

LayerProvider *CoreBoard::get_layer_provider()
{
    return &brd;
}

const Board *CoreBoard::get_canvas_data()
{
    return &brd;
}

Board *CoreBoard::get_board()
{
    return &brd;
}

const Board *CoreBoard::get_board() const
{
    return &brd;
}

Block *CoreBoard::get_block()
{
    return get_board()->block;
}

Rules *CoreBoard::get_rules()
{
    return &rules;
}

void CoreBoard::update_rules()
{
    brd.rules = rules;
}

ViaPadstackProvider *CoreBoard::get_via_padstack_provider()
{
    return &via_padstack_provider;
}

CoreBoard::HistoryItem::HistoryItem(const Block &b, const Board &r) : block(b), brd(shallow_copy, r)
{
}

void CoreBoard::history_push()
{
    history.push_back(std::make_unique<CoreBoard::HistoryItem>(block, brd));
}

void CoreBoard::history_load(unsigned int i)
{
    auto x = dynamic_cast<CoreBoard::HistoryItem *>(history.at(history_current).get());
    std::map<UUID, unsigned int> plane_revs;
    for (const auto &it : brd.planes) {
        plane_revs[it.first] = it.second.revision;
    }
    brd.~Board(); // reconstruct board from json
    new (&brd) Board(x->brd);
    block = x->block;
    brd.update_refs();
    brd.expand();
}

json CoreBoard::get_meta()
{
    return get_meta_from_file(m_board_filename);
}

std::pair<Coordi, Coordi> CoreBoard::get_bbox()
{
    return brd.get_bbox();
}

const std::string &CoreBoard::get_filename() const
{
    return m_board_filename;
}

void CoreBoard::save(const std::string &suffix)
{
    brd.rules = rules;
    brd.fab_output_settings = fab_output_settings;
    brd.pdf_export_settings = pdf_export_settings;
    brd.step_export_settings = step_export_settings;
    brd.pnp_export_settings = pnp_export_settings;
    brd.colors = colors;
    auto j = brd.serialize();
    save_json_to_file(m_board_filename + suffix, j);
    brd.save_pictures(m_pictures_dir);
}

void CoreBoard::delete_autosave()
{
    if (Glib::file_test(m_board_filename + autosave_suffix, Glib::FILE_TEST_IS_REGULAR))
        Gio::File::create_for_path(m_board_filename + autosave_suffix)->remove();
}

} // namespace horizon
