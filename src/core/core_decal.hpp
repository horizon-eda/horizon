#pragma once
#include "pool/decal.hpp"
#include "core.hpp"
#include "nlohmann/json.hpp"
#include "document/idocument_decal.hpp"

namespace horizon {
class CoreDecal : public Core, public IDocumentDecal {
public:
    CoreDecal(const std::string &decal_filename, IPool &pool);
    bool has_object_type(ObjectType ty) const override;

    Decal &get_decal() override;
    const Decal &get_canvas_data() const;
    class LayerProvider &get_layer_provider() override;

    std::pair<Coordi, Coordi> get_bbox() override;

    const std::string &get_filename() const override;

    ObjectType get_object_type() const override
    {
        return ObjectType::DECAL;
    }

    const FileVersion &get_version() const override
    {
        return decal.version;
    }

private:
    std::map<UUID, Polygon> *get_polygon_map() override;
    std::map<UUID, Junction> *get_junction_map() override;
    std::map<UUID, Text> *get_text_map() override;
    std::map<UUID, Line> *get_line_map() override;
    std::map<UUID, Arc> *get_arc_map() override;

    Decal decal;

    std::string m_decal_filename;

    class HistoryItem : public Core::HistoryItem {
    public:
        HistoryItem(const Decal &r, const std::string &cm);
        Decal decal;
    };
    void rebuild_internal(bool from_undo, const std::string &comment) override;
    void history_push(const std::string &comment) override;
    void history_load(unsigned int i) override;
    void save(const std::string &suffix) override;
    void delete_autosave() override;
};
} // namespace horizon
