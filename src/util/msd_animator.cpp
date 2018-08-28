#include "msd_animator.hpp"

namespace horizon {
bool MSDAnimator::step(double frame_time)
{
    if (running == false)
        return false;

    if (start_time == 0) {
        start_time = frame_time;
    }
    double t = (frame_time - start_time);
    msd.target = target;
    msd.run_to(t + (1. / 60.), 5e-3);

    return true;
}


void MSDAnimator::stop()
{
    running = false;
}

double MSDAnimator::get_s() const
{
    return msd.get_s();
}

bool MSDAnimator::is_running() const
{
    return running;
}

void MSDAnimator::start()
{
    if (running)
        return;
    msd.reset();
    running = true;
    start_time = 0;
    target = 0;
}

} // namespace horizon
