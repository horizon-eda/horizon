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

template <typename T> T c2pi(T x)
{
    while (x < 0)
        x += 2 * M_PI;

    while (x > 2 * M_PI)
        x -= 2 * M_PI;
    return x;
}

template float c2pi<float>(float);
template double c2pi<double>(double);


Placement transform_package_placement_to_new_reference(Placement pl, Placement old_ref, Placement new_ref)
{

    if (old_ref.mirror) {
        old_ref.invert_angle();
        pl.invert_angle();
    }

    pl.make_relative(old_ref);
    Placement out = new_ref;

    if (new_ref.mirror) {
        pl.invert_angle();
        pl.shift.x = -pl.shift.x;
        pl.mirror = !pl.mirror;
        out.mirror = !out.mirror;
    }

    out.accumulate(pl);

    return out;
}

Placement transform_text_placement_to_new_reference(Placement pl, Placement old_ref, Placement new_ref)
{
    if (old_ref.mirror) {
        old_ref.invert_angle();
    }

    pl.make_relative(old_ref);
    Placement out = new_ref;

    if (new_ref.mirror) {
        out.invert_angle();
    }

    out.accumulate(pl);

    return out;
}


/**
 * Determine original point that the arc rounded off. If the parameters are
 * invalid or it cannot determine the proper value return false.
 */
bool revert_arc(Coordd &output, const Coordd &p0, const Coordd &p1, const Coordd &c)
{
    // The original center position is found by computing the intersection of
    // lines defined by each arc point and their tangents. Tangents are of an
    // arc perpendicular to vector to center.
    const Coordd v0 = (p0 - c);
    const Coordd v1 = (p1 - c);
    double a = v0.cross(v1) > 0 ? M_PI / 2 : -M_PI / 2; // CW / CCW
    const Coordd t0 = v0.rotate(a);
    const Coordd t1 = v1.rotate(-a);
    Segment<double> l0 = Segment<double>(p0, t0 + p0);
    Segment<double> l1 = Segment<double>(p1, t1 + p1);
    return l0.intersect(l1, output);
}

} // namespace horizon
