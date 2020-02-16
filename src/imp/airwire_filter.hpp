#pragma once
#include "canvas/iairwire_filter.hpp"
#include <map>
#include <set>
#include "util/uuid.hpp"
#include "util/changeable.hpp"

namespace horizon {
class AirwireFilter : public IAirwireFilter, public Changeable {
public:
    AirwireFilter(const class Board &brd);
    bool airwire_is_visible(const class UUID &net) const override;

    void update_from_board();
    void set_visible(const UUID &net, bool v);
    void set_all(bool v);
    void set_only(const std::set<UUID> &nets);

    class AirwireInfo {
    public:
        bool visible = true;
        unsigned int n = 0;
    };

    const std::map<UUID, AirwireInfo> &get_airwires() const
    {
        return airwires;
    }

private:
    const class Board &board;
    std::map<UUID, AirwireInfo> airwires;
};
} // namespace horizon
