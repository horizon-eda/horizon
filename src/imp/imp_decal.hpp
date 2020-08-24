#pragma once
#include "imp_layer.hpp"
#include "core/core_decal.hpp"

namespace horizon {
class ImpDecal : public ImpLayer {
public:
    ImpDecal(const std::string &decal_filename, const std::string &pool_path);

protected:
    void construct() override;

    ActionCatalogItem::Availability get_editor_type_for_action() const override
    {
        return ActionCatalogItem::AVAILABLE_IN_DECAL;
    };

    std::map<ObjectType, SelectionFilterInfo> get_selection_filter_info() const override;
    void load_default_layers() override;

private:
    void canvas_update() override;
    CoreDecal core_decal;
    Decal &decal;

    class HeaderButton *header_button = nullptr;
    Gtk::Entry *name_entry = nullptr;

    void update_header();
};
} // namespace horizon
