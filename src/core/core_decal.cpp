#include "core_decal.hpp"
#include "util/util.hpp"
#include "nlohmann/json.hpp"
#include <giomm/file.h>
#include <glibmm/fileutils.h>

namespace horizon {
CoreDecal::CoreDecal(const std::string &decal_filename, IPool &pool)
    : Core(pool, nullptr), decal(Decal::new_from_file(decal_filename)), m_decal_filename(decal_filename)
{
    rebuild("init");
}


bool CoreDecal::has_object_type(ObjectType ty) const
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

std::map<UUID, Polygon> *CoreDecal::get_polygon_map()
{
    return &decal.polygons;
}
std::map<UUID, Junction> *CoreDecal::get_junction_map()
{
    return &decal.junctions;
}
std::map<UUID, Text> *CoreDecal::get_text_map()
{
    return &decal.texts;
}
std::map<UUID, Line> *CoreDecal::get_line_map()
{
    return &decal.lines;
}
std::map<UUID, Arc> *CoreDecal::get_arc_map()
{
    return &decal.arcs;
}

void CoreDecal::rebuild_internal(bool from_undo, const std::string &comment)
{
    rebuild_finish(from_undo, comment);
}

LayerProvider &CoreDecal::get_layer_provider()
{
    return decal;
}

const Decal &CoreDecal::get_canvas_data() const
{
    return decal;
}

Decal &CoreDecal::get_decal()
{
    return decal;
}

CoreDecal::HistoryItem::HistoryItem(const Decal &dec, const std::string &cm) : Core::HistoryItem(cm), decal(dec)
{
}

void CoreDecal::history_push(const std::string &comment)
{
    history.push_back(std::make_unique<CoreDecal::HistoryItem>(decal, comment));
}

void CoreDecal::history_load(unsigned int i)
{
    const auto &x = dynamic_cast<CoreDecal::HistoryItem &>(*history.at(history_current));
    decal = x.decal;
    s_signal_rebuilt.emit();
}

std::pair<Coordi, Coordi> CoreDecal::get_bbox()
{
    return decal.get_bbox();
}

const std::string &CoreDecal::get_filename() const
{
    return m_decal_filename;
}

void CoreDecal::save(const std::string &suffix)
{
    s_signal_save.emit();
    auto j = decal.serialize();
    save_json_to_file(m_decal_filename + suffix, j);
}

void CoreDecal::delete_autosave()
{
    if (Glib::file_test(m_decal_filename + autosave_suffix, Glib::FILE_TEST_IS_REGULAR))
        Gio::File::create_for_path(m_decal_filename + autosave_suffix)->remove();
}
} // namespace horizon
