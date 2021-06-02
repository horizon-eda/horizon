#include "geom_util.hpp"
#include <sstream>
#include <iomanip>
#include <cmath>
#include "util.hpp"

namespace horizon {
Coordd project_onto_perp_bisector(const Coordd &a, const Coordd &b, const Coordd &p)
{
    const auto c = (a + b) / 2;
    const auto d = b - a;
    if (d.mag_sq() == 0)
        return p;
    const auto u = (d.dot(c) - d.dot(p)) / d.mag_sq();
    return p + d * u;
}


std::string coord_to_string(const Coordf &pos, bool delta)
{
    std::ostringstream ss;
    ss.imbue(get_locale());
    if (delta) {
        ss << "Δ";
    }
    ss << "X:";
    if (pos.x >= 0) {
        ss << "+";
    }
    else {
        ss << "−"; // this is U+2212 MINUS SIGN, has same width as +
    }
    ss << std::fixed << std::setprecision(3) << std::setw(7) << std::setfill('0') << std::internal
       << std::abs(pos.x / 1e6) << " mm "; // NBSP
    if (delta) {
        ss << "Δ";
    }
    ss << "Y:";
    if (pos.y >= 0) {
        ss << "+";
    }
    else {
        ss << "−";
    }
    ss << std::setw(7) << std::abs(pos.y / 1e6) << " mm"; // NBSP
    return ss.str();
}

std::string dim_to_string(int64_t x, bool with_sign)
{
    std::ostringstream ss;
    ss.imbue(get_locale());
    if (with_sign) {
        if (x >= 0) {
            ss << "+";
        }
        else {
            ss << "−"; // this is U+2212 MINUS SIGN, has same width as +
        }
    }
    ss << std::fixed << std::setprecision(3) << std::setw(7) << std::setfill('0') << std::internal << std::abs(x / 1e6)
       << " mm"; // NBSP
    return ss.str();
}

std::string angle_to_string(int x, bool pos_only)
{
    x = wrap_angle(x);
    if (!pos_only && x > 32768)
        x -= 65536;

    std::ostringstream ss;
    ss.imbue(get_locale());
    if (x >= 0) {
        ss << "+";
    }
    else {
        ss << "−"; // this is U+2212 MINUS SIGN, has same width as +
    }
    ss << std::fixed << std::setprecision(3) << std::setw(7) << std::setfill('0') << std::internal
       << std::abs((x / 65536.0) * 360) << " °"; // NBSP
    return ss.str();
}

int orientation_to_angle(Orientation o)
{
    int angle = 0;
    switch (o) {
    case Orientation::RIGHT:
        angle = 0;
        break;
    case Orientation::LEFT:
        angle = 32768;
        break;
    case Orientation::UP:
        angle = 16384;
        break;
    case Orientation::DOWN:
        angle = 49152;
        break;
    }
    return angle;
}

int64_t round_multiple(int64_t x, int64_t mul)
{
    return ((x + sgn(x) * mul / 2) / mul) * mul;
}

double angle_to_rad(int angle)
{
    return (angle / 32768.) * M_PI;
}

int angle_from_rad(double rad)
{
    return std::round(rad / M_PI * 32768.);
}

int wrap_angle(int angle)
{
    while (angle < 0) {
        angle += 65536;
    }
    angle %= 65536;
    return angle;
}

float c2pi(float x)
{
    while (x < 0)
        x += 2 * M_PI;

    while (x > 2 * M_PI)
        x -= 2 * M_PI;
    return x;
}

} // namespace horizon
