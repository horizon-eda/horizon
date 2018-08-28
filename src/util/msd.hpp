#pragma once

namespace horizon {
class MSD { // mass spring damper system simulation
public:
    MSD();
    void run_to(double time, double ts);
    void step(double ts);
    void reset();
    double get_s() const;
    double get_t() const;

    double mass = 0.003;      // kg
    double damping = .21;     // kg/s
    double springyness = .25; // newton/m

    double target = 0; // m
private:
    double a = 0; // m/s²
    double v = 0; // m/s
    double s = 0; // m
    double t = 0; // s
};
} // namespace horizon
