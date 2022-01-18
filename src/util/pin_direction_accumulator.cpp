#include "pin_direction_accumulator.hpp"

namespace horizon {

void PinDirectionAccumulator::accumulate(Pin::Direction dir)
{
    if (!acc.has_value()) {
        acc = dir;
    }
    else if (dir != *acc) {
        using D = Pin::Direction;
        switch (*acc) {
        case D::BIDIRECTIONAL:
            // stays bidirectional
            break;

        case D::INPUT:
            switch (dir) {
            case D::BIDIRECTIONAL:
            case D::OPEN_COLLECTOR:
                acc = dir;
                break;

            case D::INPUT:
            case D::NOT_CONNECTED:
            case D::PASSIVE:
            case D::POWER_INPUT:

                break;

            case D::OUTPUT:
            case D::POWER_OUTPUT:;
                acc = D::BIDIRECTIONAL;
                break;
            }
            break;

        case D::NOT_CONNECTED:
        case D::PASSIVE:
            acc = dir;
            break;

        case D::OPEN_COLLECTOR:
            switch (dir) {
            case D::INPUT:
            case D::NOT_CONNECTED:
            case D::OPEN_COLLECTOR:
            case D::POWER_INPUT:
            case D::PASSIVE:
                break;

            case D::OUTPUT:
            case D::POWER_OUTPUT:
            case D::BIDIRECTIONAL:
                acc = D::BIDIRECTIONAL;
                break;
            }
            break;

        case D::OUTPUT:
            switch (dir) {
            case D::BIDIRECTIONAL:
            case D::INPUT:
            case D::OPEN_COLLECTOR:
            case D::POWER_INPUT:
                acc = D::BIDIRECTIONAL;
                break;

            case D::NOT_CONNECTED:
            case D::OUTPUT:
            case D::PASSIVE:
            case D::POWER_OUTPUT:
                break;
            }
            break;

        case D::POWER_INPUT:
            switch (dir) {
            case D::BIDIRECTIONAL:
            case D::OUTPUT:
            case D::POWER_OUTPUT:
                acc = D::BIDIRECTIONAL;
                break;

            case D::INPUT:
                acc = D::INPUT;
                break;

            case D::NOT_CONNECTED:
            case D::PASSIVE:
            case D::POWER_INPUT:
                break;

            case D::OPEN_COLLECTOR:
                acc = D::OPEN_COLLECTOR;
                break;
            }
            break;

        case D::POWER_OUTPUT:
            switch (dir) {
            case D::BIDIRECTIONAL:
            case D::INPUT:
            case D::OPEN_COLLECTOR:
            case D::POWER_INPUT:
                acc = D::BIDIRECTIONAL;
                break;

            case D::NOT_CONNECTED:
            case D::PASSIVE:
            case D::POWER_OUTPUT:
                break;

            case D::OUTPUT:
                acc = D::OUTPUT;
                break;
            }
            break;
        }
    }
}

std::optional<Pin::Direction> PinDirectionAccumulator::get() const
{
    return acc;
}

} // namespace horizon
