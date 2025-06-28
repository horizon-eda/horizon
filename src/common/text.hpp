#pragma once
#include "util/uuid.hpp"
#include <nlohmann/json_fwd.hpp>
#include "common.hpp"
#include "util/placement.hpp"
#include "util/text_data.hpp"

namespace horizon {
using json = nlohmann::json;
enum class TextOrigin { BASELINE, CENTER, BOTTOM };

/**
 * Used wherever a user-editable text is needed.
 */
class Text {
public:
    Text(const UUID &uu, const json &j);
    Text(const UUID &uu);

    UUID uuid;

    TextOrigin origin = TextOrigin::CENTER;
    TextData::Font font = TextData::Font::SIMPLEX;
    Placement placement;
    std::string text;
    uint64_t size = 1.5_mm;
    uint64_t width = 0;
    int layer = 0;
    bool allow_upside_down = false;
    std::string text_override;

    /**
     * When set, the renderer will render text_override instead of text.
     * Used for Text that are the result of a smash operation.
     */
    bool overridden = false;

    /**
     * true if this is the result of a smash operation.
     * Used to track it's lifecycle on unsmash.
     */
    bool from_smash = false;

    UUID get_uuid() const;
    json serialize() const;
};
} // namespace horizon
