#pragma once
#include <nlohmann/json_fwd.hpp>
#include "util/uuid.hpp"
#include "common/lut.hpp"
#include <map>
#include <vector>
#include "util/file_version.hpp"

namespace horizon {
using json = nlohmann::json;

/**
 * A Pin represents a logical pin of a Unit.
 */

class Pin {
public:
    enum class Direction {
        INPUT,
        OUTPUT,
        BIDIRECTIONAL,
        OPEN_COLLECTOR,
        POWER_INPUT,
        POWER_OUTPUT,
        PASSIVE,
        NOT_CONNECTED
    };

    Pin(const UUID &uu, const json &j);
    Pin(const UUID &uu);

    UUID uuid;
    /**
     * The Pin's primary name. i.e. PB0 on an MCU.
     */
    std::string primary_name;
    Direction direction = Direction::INPUT;
    static const LutEnumStr<Pin::Direction> direction_lut;
    static const std::vector<std::pair<Pin::Direction, std::string>> direction_names;
    static const std::map<Pin::Direction, std::string> direction_abbreviations;
    /**
     * Pins of the same swap_group can be pinswapped.
     * The swap group 0 is for unswappable pins.
     */
    unsigned int swap_group = 0;
    /**
     * The Pin's alternate names. i.e. UART_RX or I2C_SDA on an MCU.
     */
    class AlternateName {
    public:
        explicit AlternateName() = default;
        explicit AlternateName(const std::string &name, Direction dir);
        explicit AlternateName(const json &j);
        std::string name;
        Direction direction = Direction::INPUT;

        json serialize() const;
    };
    std::map<UUID, AlternateName> names;

    static UUID alternate_name_uuid_from_index(int idx);

    json serialize() const;
    UUID get_uuid() const;
};
/**
 * A Unit is the template for a Gate inside of an Entity.
 * An example for a Unit may be a "single-ended NAND gate".
 * \ref Unit "Units" are stored in an Entity.
 */
class Unit {
private:
    Unit(const UUID &uu, const json &j);

public:
    static Unit new_from_file(const std::string &filename);
    static unsigned int get_app_version();
    Unit(const UUID &uu);
    UUID uuid;
    std::string name;
    std::string manufacturer;
    std::map<UUID, Pin> pins;
    FileVersion version;

    unsigned int get_required_version() const;

    json serialize() const;
    UUID get_uuid() const;
};
} // namespace horizon
