#pragma once
#include <sigc++/sigc++.h>

namespace horizon {
class ScopedBlock {
public:
    ScopedBlock(std::vector<sigc::connection> &conns) : connections(conns)
    {
        for (auto &conn : connections) {
            conn.block();
        }
    }

    ~ScopedBlock()
    {
        for (auto &conn : connections) {
            conn.unblock();
        }
    }

private:
    std::vector<sigc::connection> &connections;
};
} // namespace horizon
