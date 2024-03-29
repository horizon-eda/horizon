#pragma once
#include "imp_layer.hpp"
#include "core/core_padstack.hpp"

namespace horizon {
class ImpPadstack : public ImpLayer {
public:
    ImpPadstack(const std::string &symbol_filename, const std::string &pool_path, TempMode temp_mode);

protected:
    void construct() override;

    ActionCatalogItem::Availability get_editor_type_for_action() const override
    {
        return ActionCatalogItem::AVAILABLE_IN_PADSTACK;
    };
    ActionToolID get_doubleclick_action(ObjectType type, const UUID &uu) override;

    std::map<ObjectType, SelectionFilterInfo> get_selection_filter_info() const override;

private:
    void canvas_update() override;
    CorePadstack core_padstack;
    Padstack &padstack;

    class HeaderButton *header_button = nullptr;
    Gtk::Entry *name_entry = nullptr;

    class ParameterWindow *parameter_window = nullptr;

    void update_header();

    bool set_filename() override;
};
} // namespace horizon
