#pragma once
#include <string>

namespace horizon {
class Layer {
public:
    Layer(int i, const std::string &n, bool r = false, bool cop = false)
        : index(i), color_layer(i), position(i), name(n), reverse(r), copper(cop)
    {
    }
    int index;
    int color_layer;
    double position;
    std::string name;
    bool reverse;
    bool copper;

    inline bool operator==(const Layer &other) const
    {
        return position == other.position && index == other.index && copper == other.copper && name == other.name
               && reverse == other.reverse && color_layer == other.color_layer;
    }
};
} // namespace horizon
