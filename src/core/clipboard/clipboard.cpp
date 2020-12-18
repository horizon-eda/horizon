#include "clipboard.hpp"
#include "clipboard_padstack.hpp"
#include "clipboard_package.hpp"
#include "clipboard_schematic.hpp"
#include "clipboard_board.hpp"
#include "nlohmann/json.hpp"
#include "common/line.hpp"
#include "common/polygon.hpp"
#include "common/arc.hpp"
#include "common/text.hpp"
#include "document/idocument.hpp"
#include "document/idocument_padstack.hpp"
#include "document/idocument_package.hpp"
#include "document/idocument_schematic.hpp"
#include "document/idocument_board.hpp"
#include "common/dimension.hpp"

namespace horizon {

std::unique_ptr<ClipboardBase> ClipboardBase::create(IDocument &doc)
{
    auto pd = &doc;

    if (auto d = dynamic_cast<IDocumentPadstack *>(pd))
        return std::make_unique<ClipboardPadstack>(*d);
    if (auto d = dynamic_cast<IDocumentPackage *>(pd))
        return std::make_unique<ClipboardPackage>(*d);
    if (auto d = dynamic_cast<IDocumentSchematic *>(pd))
        return std::make_unique<ClipboardSchematic>(*d);
    if (auto d = dynamic_cast<IDocumentBoard *>(pd))
        return std::make_unique<ClipboardBoard>(*d);

    return std::make_unique<ClipboardGeneric>(doc);
}

json ClipboardBase::process(const std::set<SelectableRef> &sel)
{
    selection = sel;
    expand_selection();
    json j;
    serialize(j);
    return j;
}

void ClipboardBase::expand_selection()
{
    std::set<SelectableRef> new_sel;
    for (const auto &it : selection) {
        switch (it.type) {
        case ObjectType::LINE: {
            Line *line = get_doc().get_line(it.uuid);
            new_sel.emplace(line->from.uuid, ObjectType::JUNCTION);
            new_sel.emplace(line->to.uuid, ObjectType::JUNCTION);
        } break;
        case ObjectType::POLYGON_EDGE: {
            Polygon *poly = get_doc().get_polygon(it.uuid);
            new_sel.emplace(poly->uuid, ObjectType::POLYGON);
        } break;
        case ObjectType::POLYGON_VERTEX: {
            Polygon *poly = get_doc().get_polygon(it.uuid);
            new_sel.emplace(poly->uuid, ObjectType::POLYGON);
        } break;
        case ObjectType::ARC: {
            Arc *arc = get_doc().get_arc(it.uuid);
            new_sel.emplace(arc->from.uuid, ObjectType::JUNCTION);
            new_sel.emplace(arc->to.uuid, ObjectType::JUNCTION);
            new_sel.emplace(arc->center.uuid, ObjectType::JUNCTION);
        } break;
        default:;
        }
    }
    selection.insert(new_sel.begin(), new_sel.end());
}

void ClipboardBase::serialize(json &j)
{
    j["texts"] = json::object();
    j["junctions"] = json::object();
    j["lines"] = json::object();
    j["arcs"] = json::object();
    j["polygons"] = json::object();
    j["dimensions"] = json::object();

    for (const auto &it : selection) {
        switch (it.type) {
        case ObjectType::TEXT:
            j["texts"][(std::string)it.uuid] = get_doc().get_text(it.uuid)->serialize();
            break;

        case ObjectType::JUNCTION:
            j["junctions"][(std::string)it.uuid] = get_doc().get_junction(it.uuid)->serialize();
            break;

        case ObjectType::LINE:
            j["lines"][(std::string)it.uuid] = get_doc().get_line(it.uuid)->serialize();
            break;

        case ObjectType::ARC:
            j["arcs"][(std::string)it.uuid] = get_doc().get_arc(it.uuid)->serialize();
            break;

        case ObjectType::POLYGON:
            j["polygons"][(std::string)it.uuid] = get_doc().get_polygon(it.uuid)->serialize();
            break;

        case ObjectType::DIMENSION:
            j["dimensions"][(std::string)it.uuid] = get_doc().get_dimension(it.uuid)->serialize();
            break;

        default:;
        }
    }
}

} // namespace horizon
