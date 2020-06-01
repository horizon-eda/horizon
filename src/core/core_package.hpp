#pragma once
#include "common/layer.hpp"
#include "core.hpp"
#include "pool/package.hpp"
#include "pool/pool.hpp"
#include <deque>
#include <iostream>
#include <memory>
#include "document/idocument_package.hpp"

namespace horizon {
class CorePackage : public Core, public virtual IDocumentPackage {
public:
    CorePackage(const std::string &filename, Pool &pool);
    bool has_object_type(ObjectType ty) const override;

    Package *get_package() override;

    /*Polygon *insert_polygon(const UUID &uu);
    Polygon *get_polygon(const UUID &uu=true);
    void delete_polygon(const UUID &uu);
    Hole *insert_hole(const UUID &uu);
    Hole *get_hole(const UUID &uu=true);
    void delete_hole(const UUID &uu);*/

    class LayerProvider *get_layer_provider() override;

    bool set_property(ObjectType type, const UUID &uu, ObjectProperty::ID property,
                      const class PropertyValue &value) override;
    bool get_property(ObjectType type, const UUID &uu, ObjectProperty::ID property,
                      class PropertyValue &value) override;
    bool get_property_meta(ObjectType type, const UUID &uu, ObjectProperty::ID property,
                           class PropertyMeta &meta) override;

    std::string get_display_name(ObjectType type, const UUID &uu) override;
    class Rules *get_rules() override;

    void rebuild(bool from_undo = false) override;

    const Package *get_canvas_data();
    std::pair<Coordi, Coordi> get_bbox() override;
    json get_meta() override;

    void reload_pool() override;

    const std::string &get_filename() const override;

private:
    std::map<UUID, Junction> *get_junction_map() override;
    std::map<UUID, Line> *get_line_map() override;
    std::map<UUID, Arc> *get_arc_map() override;
    std::map<UUID, Text> *get_text_map() override;
    std::map<UUID, Polygon> *get_polygon_map() override;
    std::map<UUID, Hole> *get_hole_map() override;
    std::map<UUID, Keepout> *get_keepout_map() override;
    std::map<UUID, Dimension> *get_dimension_map() override;
    std::map<UUID, Picture> *get_picture_map() override;

    Package package;
    std::string m_filename;
    std::string m_pictures_dir;

    PackageRules rules;

    class HistoryItem : public Core::HistoryItem {
    public:
        HistoryItem(const Package &k) : package(k)
        {
        }
        Package package;
    };
    void history_push() override;
    void history_load(unsigned int i) override;
    void save(const std::string &suffix) override;
    void delete_autosave() override;

public:
    std::string parameter_program_code;
    ParameterSet parameter_set;

    std::map<UUID, Package::Model> models;
    UUID default_model;
};
} // namespace horizon
