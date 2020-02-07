#include "core_frame.hpp"
#include "util/util.hpp"
#include <algorithm>
#include <memory>
#include "nlohmann/json.hpp"
#include <giomm/file.h>
#include <glibmm/fileutils.h>

namespace horizon {
CoreFrame::CoreFrame(const std::string &frame_filename)
    : frame(Frame::new_from_file(frame_filename)), m_frame_filename(frame_filename)
{
    rebuild();
}


bool CoreFrame::has_object_type(ObjectType ty) const
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

std::map<UUID, Polygon> *CoreFrame::get_polygon_map()
{
    return &frame.polygons;
}
std::map<UUID, Junction> *CoreFrame::get_junction_map()
{
    return &frame.junctions;
}
std::map<UUID, Text> *CoreFrame::get_text_map()
{
    return &frame.texts;
}
std::map<UUID, Line> *CoreFrame::get_line_map()
{
    return &frame.lines;
}
std::map<UUID, Arc> *CoreFrame::get_arc_map()
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

const std::string &CoreFrame::get_filename() const
{
    return m_frame_filename;
}

void CoreFrame::save(const std::string &suffix)
{
    s_signal_save.emit();
    auto j = frame.serialize();
    save_json_to_file(m_frame_filename + suffix, j);
}

void CoreFrame::delete_autosave()
{
    if (Glib::file_test(m_frame_filename + autosave_suffix, Glib::FILE_TEST_IS_REGULAR))
        Gio::File::create_for_path(m_frame_filename + autosave_suffix)->remove();
}
} // namespace horizon
