#pragma once

namespace horizon {
class Once {
public:
    bool operator()()
    {
        if (first) {
            first = false;
            return true;
        }
        return false;
    }

private:
    bool first = true;
};
} // namespace horizon
