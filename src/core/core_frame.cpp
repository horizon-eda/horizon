#include "core_frame.hpp"
#include "util/util.hpp"
#include <algorithm>
#include <memory>
#include "nlohmann/json.hpp"

namespace horizon {
CoreFrame::CoreFrame(const std::string &frame_filename)
    : frame(Frame::new_from_file(frame_filename)), m_frame_filename(frame_filename)
{
    rebuild();
}


bool CoreFrame::has_object_type(ObjectType ty)
{
    switch (ty) {
    case ObjectType::JUNCTION:
    case ObjectType::POLYGON:
    case ObjectType::POLYGON_EDGE:
    case ObjectType::POLYGON_VERTEX:
    case ObjectType::POLYGON_ARC_CENTER:
    case ObjectType::TEXT:
    case ObjectType::LINE:
    case ObjectType::ARC:
        return true;
        break;
    default:;
    }

    return false;
}

std::map<UUID, Polygon> *CoreFrame::get_polygon_map(bool work)
{
    return &frame.polygons;
}
std::map<UUID, Junction> *CoreFrame::get_junction_map(bool work)
{
    return &frame.junctions;
}
std::map<UUID, Text> *CoreFrame::get_text_map(bool work)
{
    return &frame.texts;
}
std::map<UUID, Line> *CoreFrame::get_line_map(bool work)
{
    return &frame.lines;
}
std::map<UUID, Arc> *CoreFrame::get_arc_map(bool work)
{
    return &frame.arcs;
}

void CoreFrame::rebuild(bool from_undo)
{
    frame.expand();
    Core::rebuild(from_undo);
}

LayerProvider *CoreFrame::get_layer_provider()
{
    return &frame;
}

const Frame *CoreFrame::get_canvas_data() const
{
    return &frame;
}

Frame *CoreFrame::get_frame()
{
    return &frame;
}

void CoreFrame::commit()
{
    set_needs_save(true);
}

void CoreFrame::revert()
{
    history_load(history_current);
    reverted = true;
}

CoreFrame::HistoryItem::HistoryItem(const Frame &fr) : frame(fr)
{
}

void CoreFrame::history_push()
{
    history.push_back(std::make_unique<CoreFrame::HistoryItem>(frame));
}

void CoreFrame::history_load(unsigned int i)
{
    auto x = dynamic_cast<CoreFrame::HistoryItem *>(history.at(history_current).get());
    frame = x->frame;
    frame.expand();
    s_signal_rebuilt.emit();
}

std::pair<Coordi, Coordi> CoreFrame::get_bbox()
{
    return frame.get_bbox();
}

void CoreFrame::save()
{
    s_signal_save.emit();
    auto j = frame.serialize();
    save_json_to_file(m_frame_filename, j);

    set_needs_save(false);
}
} // namespace horizon
