#pragma once
#include "imp_layer.hpp"
#include "core/core_padstack.hpp"

namespace horizon {
class ImpPadstack : public ImpLayer {
public:
    ImpPadstack(const std::string &symbol_filename, const std::string &pool_path);

protected:
    void construct() override;

    ActionCatalogItem::Availability get_editor_type_for_action() const override
    {
        return ActionCatalogItem::AVAILABLE_IN_PADSTACK;
    };
    ObjectType get_editor_type() const override
    {
        return ObjectType::PADSTACK;
    }
    std::pair<ActionID, ToolID> get_doubleclick_action(ObjectType type, const UUID &uu) override;

private:
    void canvas_update() override;
    CorePadstack core_padstack;
};
} // namespace horizon
