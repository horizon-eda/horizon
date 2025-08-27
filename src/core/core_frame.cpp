#include "core_frame.hpp"
#include "core_properties.hpp"
#include "util/util.hpp"
#include <nlohmann/json.hpp>
#include <giomm/file.h>
#include <glibmm/fileutils.h>

namespace horizon {
CoreFrame::CoreFrame(const std::string &frame_filename, IPool &pool)
    : Core(pool, nullptr), frame(Frame::new_from_file(frame_filename)), m_frame_filename(frame_filename)
{
    rebuild("init");
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

bool CoreFrame::get_property_meta(ObjectType type, const UUID &uu, ObjectProperty::ID property, PropertyMeta &meta)
{
    if (Core::get_property_meta(type, uu, property, meta))
        return true;
    switch (type) {
    case ObjectType::TEXT:
        switch (property) {
        case ObjectProperty::ID::ALLOW_UPSIDE_DOWN:
            meta.is_visible = false;
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

void CoreFrame::rebuild_internal(bool from_undo, const std::string &comment)
{
    frame.expand();
    rebuild_finish(from_undo, comment);
}

LayerProvider &CoreFrame::get_layer_provider()
{
    return frame;
}

const Frame &CoreFrame::get_canvas_data() const
{
    return frame;
}

Frame &CoreFrame::get_frame()
{
    return frame;
}

class HistoryItemFrame : public HistoryManager::HistoryItem {
public:
    HistoryItemFrame(const Frame &fr, const std::string &cm) : HistoryManager::HistoryItem(cm), frame(fr)
    {
    }
    Frame frame;
};

std::unique_ptr<HistoryManager::HistoryItem> CoreFrame::make_history_item(const std::string &comment)
{
    return std::make_unique<HistoryItemFrame>(frame, comment);
}

void CoreFrame::history_load(const HistoryManager::HistoryItem &it)
{
    const auto &x = dynamic_cast<const HistoryItemFrame &>(it);
    frame = x.frame;
    frame.expand();
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

void CoreFrame::set_temp_mode()
{
    Gio::File::create_for_path(m_frame_filename)->remove();
    m_frame_filename.clear();
}

void CoreFrame::set_filename(const std::string &filename)
{
    m_frame_filename = filename;
}
} // namespace horizon
