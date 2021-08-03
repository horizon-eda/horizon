#pragma once
#include "core/tool.hpp"

namespace horizon {
class ToolSettingsRectangleMode : public ToolSettings {
public:
    json serialize() const override;
    void load_from_json(const json &j) override;
    enum class Mode { CENTER, CORNER };
    Mode mode = Mode::CENTER;
    void cycle_mode();
};
} // namespace horizon
