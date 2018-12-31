#pragma once
#include "frame/frame.hpp"
#include "core.hpp"
#include <iostream>
#include <memory>
#include "nlohmann/json.hpp"

namespace horizon {
class CoreFrame : public Core {
public:
    CoreFrame(const std::string &frame_filename);
    bool has_object_type(ObjectType ty) const override;

    Frame *get_frame();
    const Frame *get_canvas_data() const;
    class LayerProvider *get_layer_provider() override;

    void rebuild(bool from_undo = false) override;
    void commit() override;
    void revert() override;
    void save() override;

    std::pair<Coordi, Coordi> get_bbox() override;

private:
    std::map<UUID, Polygon> *get_polygon_map(bool work = true) override;
    std::map<UUID, Junction> *get_junction_map(bool work = true) override;
    std::map<UUID, Text> *get_text_map(bool work = true) override;
    std::map<UUID, Line> *get_line_map(bool work = true) override;
    std::map<UUID, Arc> *get_arc_map(bool work = true) override;

    Frame frame;

    std::string m_frame_filename;

    class HistoryItem : public Core::HistoryItem {
    public:
        HistoryItem(const Frame &r);
        Frame frame;
    };
    void history_push() override;
    void history_load(unsigned int i) override;
};
} // namespace horizon
