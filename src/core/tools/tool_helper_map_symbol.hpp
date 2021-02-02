#pragma once
#include "core/tool.hpp"

namespace horizon {
class ToolHelperMapSymbol : public virtual ToolBase {
public:
    using ToolBase::ToolBase;

    class Settings : public ToolSettings {
    public:
        json serialize() const override;
        void load_from_json(const json &j) override;
        std::map<UUID, UUID> selected_symbols;
    };

    ToolSettings *get_settings() override
    {
        return &settings;
    }

    ToolID get_tool_id_for_settings() const override;

protected:
    class SchematicSymbol *map_symbol(class Component *c, const class Gate *g);
    const class Symbol *get_symbol_for_unit(const UUID &unit_uu, bool *auto_selected = nullptr);
    void change_symbol(class SchematicSymbol *schsym);

private:
    Settings settings;
};
} // namespace horizon
