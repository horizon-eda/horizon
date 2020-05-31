#include "placement.hpp"
#include "nlohmann/json.hpp"

namespace horizon {
Placement::Placement(const json &j)
    : shift(j.at("shift").get<std::vector<int64_t>>()), mirror(j.at("mirror").get<bool>()),
      angle(j.at("angle").get<int>())
{
    set_angle(angle);
}
json Placement::serialize() const
{
    json j;
    j["shift"] = shift.as_array();
    j["angle"] = angle;
    j["mirror"] = mirror;
    return j;
}

void Placement::make_relative(const Placement &to)
{
    mirror ^= to.mirror;
    shift -= to.shift;

    if (to.mirror) {
        shift.x = -shift.x;
    }
    angle -= to.angle;
    while (angle < 0) {
        angle += 65536;
    }
    angle %= 65536;

    Coordi s = shift;
    if (to.angle == 0) {
        // nop
    }
    else if (to.angle == 16384) {
        shift.y = -s.x;
        shift.x = s.y;
    }
    else if (to.angle == 32768) {
        shift.y = -s.y;
        shift.x = -s.x;
    }
    else if (to.angle == 49152) {
        shift.y = s.x;
        shift.x = -s.y;
    }
    else {
        double af = -(to.angle / 65536.0) * 2 * M_PI;
        shift.x = s.x * cos(af) - s.y * sin(af);
        shift.y = s.x * sin(af) + s.y * cos(af);
    }
}

void Placement::accumulate(const Placement &p)
{
    Placement q = p;

    if (angle == 0) {
        // nop
    }
    else if (angle == 16384) {
        q.shift.y = p.shift.x;
        q.shift.x = -p.shift.y;
    }
    else if (angle == 32768) {
        q.shift.y = -p.shift.y;
        q.shift.x = -p.shift.x;
    }
    else if (angle == 49152) {
        q.shift.y = -p.shift.x;
        q.shift.x = p.shift.y;
    }
    else {
        double af = (angle / 65536.0) * 2 * M_PI;
        q.shift.x = p.shift.x * cos(af) - p.shift.y * sin(af);
        q.shift.y = p.shift.x * sin(af) + p.shift.y * cos(af);
    }

    if (mirror) {
        q.shift.x = -q.shift.x;
    }

    shift += q.shift;
    angle += p.angle;
    while (angle < 0) {
        angle += 65536;
    }
    angle %= 65536;
    mirror ^= q.mirror;
}

void Placement::invert_angle()
{
    set_angle(-angle);
}

void Placement::set_angle(int a)
{
    angle = a;
    while (angle < 0)
        angle += 65536;
    angle = angle % 65536;
}

void Placement::inc_angle(int a)
{
    set_angle(angle + a);
}

void Placement::set_angle_deg(int a)
{
    set_angle((a * 65536) / 360);
}

void Placement::set_angle_rad(double a)
{
    set_angle((a * 32768) / M_PI);
}

void Placement::inc_angle_deg(int a)
{
    inc_angle((a * 65536) / 360);
}

int Placement::get_angle() const
{
    return angle;
}

int Placement::get_angle_deg() const
{
    return (angle * 360) / 65536;
}

double Placement::get_angle_rad() const
{
    return (angle / 32768.0) * M_PI;
}
} // namespace horizon
