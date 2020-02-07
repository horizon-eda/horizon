#pragma once
#include "frame/frame.hpp"
#include "core.hpp"
#include <iostream>
#include <memory>
#include "nlohmann/json.hpp"
#include "document/idocument_frame.hpp"

namespace horizon {
class CoreFrame : public Core, public IDocumentFrame {
public:
    CoreFrame(const std::string &frame_filename);
    bool has_object_type(ObjectType ty) const override;

    Frame *get_frame() override;
    const Frame *get_canvas_data() const;
    class LayerProvider *get_layer_provider() override;

    void rebuild(bool from_undo = false) override;

    std::pair<Coordi, Coordi> get_bbox() override;

    const std::string &get_filename() const override;

private:
    std::map<UUID, Polygon> *get_polygon_map() override;
    std::map<UUID, Junction> *get_junction_map() override;
    std::map<UUID, Text> *get_text_map() override;
    std::map<UUID, Line> *get_line_map() override;
    std::map<UUID, Arc> *get_arc_map() override;

    Frame frame;

    std::string m_frame_filename;

    class HistoryItem : public Core::HistoryItem {
    public:
        HistoryItem(const Frame &r);
        Frame frame;
    };
    void history_push() override;
    void history_load(unsigned int i) override;
    void save(const std::string &suffix) override;
    void delete_autosave() override;
};
} // namespace horizon
