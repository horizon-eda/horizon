#pragma once
#include <string>

namespace horizon {
class Layer {
public:
    Layer(int i, const std::string &n, bool r = false, bool cop = false) : index(i), name(n), reverse(r), copper(cop)
    {
    }
    int index;
    std::string name;
    bool reverse;
    bool copper;
};
} // namespace horizon
