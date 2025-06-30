#pragma once
#include "frame/frame.hpp"
#include "core.hpp"
#include <nlohmann/json.hpp>
#include "document/idocument_frame.hpp"

namespace horizon {
class CoreFrame : public Core, public IDocumentFrame {
public:
    CoreFrame(const std::string &frame_filename, IPool &pool);
    bool has_object_type(ObjectType ty) const override;

    Frame &get_frame() override;
    const Frame &get_canvas_data() const;
    class LayerProvider &get_layer_provider() override;

    std::pair<Coordi, Coordi> get_bbox() override;

    const std::string &get_filename() const override;

    ObjectType get_object_type() const override
    {
        return ObjectType::FRAME;
    }

    const FileVersion &get_version() const override
    {
        return frame.version;
    }

    void set_temp_mode();
    void set_filename(const std::string &filename);

    bool get_property_meta(ObjectType type, const UUID &uu, ObjectProperty::ID property,
                           class PropertyMeta &meta) override;

private:
    std::map<UUID, Polygon> *get_polygon_map() override;
    std::map<UUID, Junction> *get_junction_map() override;
    std::map<UUID, Text> *get_text_map() override;
    std::map<UUID, Line> *get_line_map() override;
    std::map<UUID, Arc> *get_arc_map() override;

    Frame frame;

    std::string m_frame_filename;

    void rebuild_internal(bool from_undo, const std::string &comment) override;
    std::unique_ptr<HistoryManager::HistoryItem> make_history_item(const std::string &comment) override;
    void history_load(const HistoryManager::HistoryItem &it) override;
    void save(const std::string &suffix) override;
    void delete_autosave() override;
};
} // namespace horizon
