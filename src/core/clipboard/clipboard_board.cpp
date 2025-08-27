#include "clipboard_board.hpp"
#include "document/idocument_board.hpp"
#include "board/board.hpp"
#include <nlohmann/json.hpp>

namespace horizon {

void ClipboardBoard::expand_selection()
{
    ClipboardBase::expand_selection();

    const auto &brd = *doc.get_board();
    {
        std::set<SelectableRef> new_sel;
        for (const auto &it : selection) {
            switch (it.type) {
            case ObjectType::VIA: {
                auto &via = brd.vias.at(it.uuid);
                new_sel.emplace(via.junction->uuid, ObjectType::JUNCTION);

            } break;

            case ObjectType::TRACK: {
                const auto &track = brd.tracks.at(it.uuid);
                if (track.from.is_junc())
                    new_sel.emplace(track.from.junc->uuid, ObjectType::JUNCTION);
                if (track.to.is_junc())
                    new_sel.emplace(track.to.junc->uuid, ObjectType::JUNCTION);
            } break;

            case ObjectType::BOARD_PACKAGE: {
                const auto &pkg = brd.packages.at(it.uuid);
                for (const auto &txt : pkg.texts) {
                    new_sel.emplace(txt.uuid, ObjectType::TEXT);
                }
            } break;

            default:;
            }
        }
        selection.insert(new_sel.begin(), new_sel.end());
    }
}

void ClipboardBoard::serialize(json &j)
{
    ClipboardBase::serialize(j);

    j["vias"] = json::object();
    j["tracks"] = json::object();
    j["board_holes"] = json::object();
    j["board_panels"] = json::object();
    j["decals"] = json::object();
    j["planes"] = json::object();


    const auto &brd = *doc.get_board();
    for (const auto &it : selection) {
        switch (it.type) {
        case ObjectType::VIA:
            j["vias"][(std::string)it.uuid] = brd.vias.at(it.uuid).serialize();
            break;

        case ObjectType::TRACK: {
            const auto &brd_track = brd.tracks.at(it.uuid);
            auto track = brd_track;
            std::map<UUID, BoardJunction> extra_junctions;
            for (auto &it_ft : {&track.from, &track.to}) {
                if (it_ft->is_pad()) {
                    auto uu = UUID::random();
                    auto &ju = extra_junctions.emplace(uu, uu).first->second;
                    ju.position = it_ft->get_position();
                    it_ft->connect(&ju);
                }
            }
            auto o = track.serialize();
            if (brd_track.from.is_pad())
                o["from_pad"] = brd_track.from.serialize();
            if (brd_track.to.is_pad())
                o["to_pad"] = brd_track.to.serialize();
            j["tracks"][(std::string)it.uuid] = o;
            for (const auto &[uu, ju] : extra_junctions) {
                j["junctions"][(std::string)uu] = ju.serialize();
            }
        } break;

        case ObjectType::BOARD_HOLE:
            j["board_holes"][(std::string)it.uuid] = brd.holes.at(it.uuid).serialize();
            break;

        case ObjectType::BOARD_PANEL:
            j["board_panels"][(std::string)it.uuid] = brd.board_panels.at(it.uuid).serialize();
            break;

        case ObjectType::BOARD_DECAL:
            j["decals"][(std::string)it.uuid] = brd.decals.at(it.uuid).serialize();
            break;

        case ObjectType::BOARD_PACKAGE: {
            const auto &pkg = brd.packages.at(it.uuid);
            auto o = pkg.serialize();
            o["group"] = (std::string)pkg.component->group;
            o["tag"] = (std::string)pkg.component->tag;
            o["package"] = (std::string)pkg.package.uuid;
            auto connections = json::object();
            for (const auto &[uu, pad] : pkg.package.pads) {
                if (pad.net)
                    connections[(std::string)uu] = (std::string)pad.net.uuid;
            }
            o["connections"] = connections;
            j["packages"][(std::string)it.uuid] = o;
        } break;

        case ObjectType::POLYGON: {
            const auto &poly = brd.polygons.at(it.uuid);
            if (auto plane = dynamic_cast<const Plane *>(poly.usage.ptr)) {
                j["planes"][(std::string)plane->uuid] = plane->serialize();
            }
        } break;

        default:;
        }
    }
}

IDocument &ClipboardBoard::get_doc()
{
    return doc;
}
} // namespace horizon
