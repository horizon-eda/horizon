#pragma once
#include <map>
#include <string>

namespace horizon {
class InputDevicesPrefs {
public:
    class Device {
    public:
        enum class Type { AUTO, TOUCHPAD, TRACKPOINT };

        Type type = Type::AUTO;
        bool invert_zoom = false;
        bool invert_pan = false;
    };
    std::map<std::string, Device> devices;
};
} // namespace horizon
