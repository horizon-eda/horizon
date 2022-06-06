#pragma once
#include <map>
#include <string>

namespace horizon {
class InputDevicesPrefs {
public:
    class Device {
    public:
        enum class Type { AUTO, TOUCHPAD, TRACKPOINT, MOUSE };

        Type type = Type::AUTO;
    };
    std::map<std::string, Device> devices;

    class DeviceType {
    public:
        bool invert_zoom = false;
        bool invert_pan = false;
    };
    std::map<Device::Type, DeviceType> device_types;
};
} // namespace horizon
