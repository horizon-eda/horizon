#include "airwire_filter.hpp"
#include "board/board.hpp"
#include "util/util.hpp"

namespace horizon {
AirwireFilter::AirwireFilter(const class Board &brd) : board(brd)
{
}

bool AirwireFilter::airwire_is_visible(const class UUID &net) const
{
    if (airwires.count(net))
        return airwires.at(net).visible;
    else
        return true;
}

void AirwireFilter::update_from_board()
{
    for (auto &it : airwires) {
        it.second.n = 0;
    }
    for (const auto &it : board.airwires) {
        airwires[it.first].n = it.second.size();
    }
    map_erase_if(airwires, [](const auto &x) { return x.second.n == 0; });
}

void AirwireFilter::set_visible(const UUID &net, bool v)
{
    if (airwires.count(net)) {
        airwires.at(net).visible = v;
        s_signal_changed.emit();
    }
}

void AirwireFilter::set_all(bool v)
{
    for (auto &it : airwires)
        it.second.visible = v;
    s_signal_changed.emit();
}

void AirwireFilter::set_only(const std::set<UUID> &nets)
{
    for (auto &it : airwires) {
        it.second.visible = nets.count(it.first);
    }
    s_signal_changed.emit();
}

} // namespace horizon
