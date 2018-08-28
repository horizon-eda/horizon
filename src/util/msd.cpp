#include "msd.hpp"

namespace horizon {
MSD::MSD() : mass(0.002), damping(.2), springyness(.15)
{
}

void MSD::reset()
{
    t = 0;
    a = 0;
    v = 0;
    s = 0;
    target = 0;
}

void MSD::step(double ts)
{
    auto f_friction = -v * damping;
    auto f_spring = (target - s) / springyness;
    a = (f_spring + f_friction) / mass;
    v += a * ts;
    s += v * ts;
    t += ts;
}

void MSD::run_to(double time, double ts)
{
    while (t < time) {
        step(ts);
    }
}


double MSD::get_s() const
{
    return s;
}
double MSD::get_t() const
{
    return t;
}

} // namespace horizon
