#pragma once
#include "track.hpp"

namespace horizon {
class Airwire {
public:
    Airwire(const Track::Connection &f, const Track::Connection &t) : from(f), to(t)
    {
    }
    Track::Connection from;
    Track::Connection to;
    void update_refs(class Board &brd);
};
} // namespace horizon
