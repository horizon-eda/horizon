#pragma once
#include "msd.hpp"

namespace horizon {
class MSDAnimator {
public:
    bool step(double time);
    double get_s() const;
    void stop();
    void start();
    float target = 0;
    bool is_running() const;

private:
    MSD msd;
    bool running = false;
    double start_time = 0;
};
} // namespace horizon
