#pragma once
#include <stdexcept>

namespace horizon {
template <typename T> class XYZContainer {
public:
    T x;
    T y;
    T z;

    void set(unsigned int ax, const T &v)
    {
        switch (ax) {
        case 0:
            x = v;
            break;

        case 1:
            y = v;
            break;

        case 2:
            z = v;
            break;

        default:
            throw std::domain_error("axis out of range");
        }
    }

    T &get(unsigned int ax)
    {
        switch (ax) {
        case 0:
            return x;

        case 1:
            return y;

        case 2:
            return z;

        default:
            throw std::domain_error("axis out of range");
        }
    }

    void set_all(const T &v)
    {
        for (unsigned int i = 0; i < 3; i++) {
            set(i, v);
        }
    }
};
} // namespace horizon
