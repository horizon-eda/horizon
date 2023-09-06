#pragma once
#include <stdint.h>
#include <vector>
#include <algorithm>
#include <type_traits>
#include <math.h>
#include <array>
#include <sstream>
#include "lut.hpp"

namespace horizon {
enum class Orientation { LEFT, RIGHT, UP, DOWN };
/**
 * Used in conjunction with a UUID/UUIDPath to identify an object.
 */
enum class ObjectType {
    INVALID,
    JUNCTION,
    LINE,
    SYMBOL_PIN,
    ARC,
    SCHEMATIC_SYMBOL,
    TEXT,
    LINE_NET,
    COMPONENT,
    NET,
    NET_LABEL,
    POWER_SYMBOL,
    BUS,
    BUS_LABEL,
    BUS_RIPPER,
    POLYGON,
    POLYGON_VERTEX,
    POLYGON_EDGE,
    POLYGON_ARC_CENTER,
    HOLE,
    PAD,
    BOARD_PACKAGE,
    TRACK,
    VIA,
    SHAPE,
    BOARD,
    SCHEMATIC,
    UNIT,
    ENTITY,
    SYMBOL,
    PACKAGE,
    PADSTACK,
    PART,
    PLANE,
    DIMENSION,
    NET_CLASS,
    BOARD_HOLE,
    MODEL_3D,
    FRAME,
    KEEPOUT,
    CONNECTION_LINE,
    AIRWIRE,
    BOARD_PANEL,
    PICTURE,
    DECAL,
    BOARD_DECAL,
    PROJECT,
    BLOCK,
    BLOCKS,
    BLOCK_INSTANCE,
    BLOCK_SYMBOL,
    BLOCK_SYMBOL_PORT,
    SCHEMATIC_BLOCK_SYMBOL,
    POOL,
    NET_TIE,
    SCHEMATIC_NET_TIE,
    BOARD_NET_TIE,
};
enum class PatchType { OTHER, TRACK, PAD, PAD_TH, VIA, PLANE, HOLE_PTH, HOLE_NPTH, BOARD_EDGE, TEXT, NET_TIE, N_TYPES };

extern const LutEnumStr<PatchType> patch_type_lut;
extern const LutEnumStr<ObjectType> object_type_lut;
extern const LutEnumStr<Orientation> orientation_lut;

/**
 * Your typical coordinate class.
 * Supports some mathematical operators as required.
 * Unless otherwise noted, 1 equals 1 nm (that is nanometer, not nautical mile)
 * Instead of instantiating the template on your own, you want to use Coordf
 * (float) for calculations
 * that will end up only on screen and Coordi (int64_t) for everything else.
 */
template <typename T> class Coord {
public:
    T x;
    T y;

    using type = T;

    // WTF, but works
    // template<typename U = T>
    // Coord(double ix, double iy, typename std::enable_if<std::is_same<U,
    // float>::value>::type* = 0) : x((float)ix), y((float)iy) { }


    Coord(T ix, T iy) : x(ix), y(iy)
    {
    }
    Coord() : x(0), y(0)
    {
    }
    Coord(std::vector<T> v) : x(v.at(0)), y(v.at(1))
    {
    }
    operator Coord<float>() const
    {
        return Coord<float>(x, y);
    }
    operator Coord<double>() const
    {
        return Coord<double>(x, y);
    }
    Coord<T> operator+(const Coord<T> &a) const
    {
        return Coord<T>(x + a.x, y + a.y);
    }
    Coord<T> operator-(const Coord<T> &a) const
    {
        return Coord<T>(x - a.x, y - a.y);
    }
    Coord<T> operator*(const Coord<T> &a) const
    {
        return Coord<T>(x * a.x, y * a.y);
    }
    Coord<T> operator*(T r) const
    {
        return Coord<T>(x * r, y * r);
    }
    Coord<T> operator/(T r) const
    {
        return Coord<T>(x / r, y / r);
    }
    bool operator==(const Coord<T> &a) const
    {
        return a.x == x && a.y == y;
    }
    bool operator!=(const Coord<T> &a) const
    {
        return !(a == *this);
    }
    bool operator<(const Coord<T> &a) const
    {
        if (x < a.x)
            return true;
        if (x > a.x)
            return false;
        return y < a.y;
    }

    /**
     * @returns element-wise minimum of \p a and \p b
     */
    static Coord<T> min(const Coord<T> &a, const Coord<T> &b)
    {
        return Coord<T>(std::min(a.x, b.x), std::min(a.y, b.y));
    }

    /**
     * @returns element-wise maximum of \p a and \p b
     */
    static Coord<T> max(const Coord<T> &a, const Coord<T> &b)
    {
        return Coord<T>(std::max(a.x, b.x), std::max(a.y, b.y));
    }

    /**
     * @param r magnitude
     * @param phi angle in radians
     * @returns coordinate specified by \p r and \p phi
     */
    static Coord<T> euler(T r, T phi)
    {
        static_assert(std::is_floating_point_v<T>);
        return {r * cos(phi), r * sin(phi)};
    }

    Coord<T> rotate(T a) const
    {
        static_assert(std::is_floating_point_v<T>);
        const T x2 = x * cos(a) - y * sin(a);
        const T y2 = x * sin(a) + y * cos(a);
        return {x2, y2};
    }

    Coord<int64_t> to_coordi() const
    {
        static_assert(std::is_floating_point_v<T>);
        return Coord<int64_t>(x, y);
    }

    /**
     * @param a other coordinate
     * @returns dot product of \p a and this
     */
    T dot(const Coord<T> &a) const
    {
        return x * a.x + y * a.y;
    }

    T cross(const Coord<T> &other) const
    {
        return (x * other.y) - (y * other.x);
    }

    /**
     * @returns squared magnitude of this
     */
    T mag_sq() const
    {
        return x * x + y * y;
    }

    T mag() const
    {
        static_assert(std::is_floating_point_v<T>);
        return sqrt(mag_sq());
    }

    Coord<T> normalize() const
    {
        static_assert(std::is_floating_point_v<T>);
        return *this / mag();
    }

    double magd() const
    {
        static_assert(std::is_integral_v<T>);
        return sqrt(mag_sq());
    }

    std::conditional_t<std::is_same_v<T, float>, float, double> angle() const
    {
        if constexpr (std::is_same_v<T, float>)
            return atan2f(y, x);
        else
            return atan2(y, x);
    }

    bool in_range(const Coord<T> &a, const Coord<T> &b) const
    {
        return x > a.x && y > a.y && x < b.x && y < b.y;
    }

    void operator+=(const Coord<T> a)
    {
        x += a.x;
        y += a.y;
    }
    void operator-=(const Coord<T> a)
    {
        x -= a.x;
        y -= a.y;
    }
    void operator*=(T a)
    {
        x *= a;
        y *= a;
    }
    /*json serialize() {
            return {x,y};
    }*/
    std::array<T, 2> as_array() const
    {
        return {x, y};
    }

    std::string str() const
    {
        std::stringstream ss;
        ss << "(" << x << ", " << y << ")";
        return ss.str();
    }
};


typedef Coord<float> Coordf;
typedef Coord<int64_t> Coordi;
typedef Coord<double> Coordd;

class Color {
public:
    float r;
    float g;
    float b;
    Color(double ir, double ig, double ib) : r(ir), g(ig), b(ib)
    {
    }
    // Color(unsigned int ir, unsigned ig, unsigned ib): r(ir/255.), g(ig/255.),
    // b(ib/255.) {}
    static Color new_from_int(unsigned int ir, unsigned ig, unsigned ib)
    {
        return Color(ir / 255.0, ig / 255.0, ib / 255.0);
    }
    Color() : r(0), g(0), b(0)
    {
    }
};

struct ColorI {
    uint8_t r;
    uint8_t g;
    uint8_t b;

    bool operator<(const ColorI &other) const
    {
        return hashify() < other.hashify();
    }

    Color to_color() const
    {
        return Color::new_from_int(r, g, b);
    }

private:
    uint32_t hashify() const
    {
        return r | (g << 8) | (b << 16);
    }
};

constexpr int64_t operator"" _mm(long double i)
{
    return i * 1e6;
}
constexpr int64_t operator"" _mm(unsigned long long int i)
{
    return i * 1000000;
}

struct shallow_copy_t {
    explicit shallow_copy_t() = default;
};

constexpr shallow_copy_t shallow_copy = shallow_copy_t();

enum class CopyMode { DEEP, SHALLOW };


template <typename T> class Segment {
public:
    Coord<T> from;
    Coord<T> to;

    Segment(Coord<T> a, Coord<T> b) : from(a), to(b)
    {
    }

    bool intersect(const Segment<T> &other, Coord<T> &output) const
    {
        const T &x1 = from.x;
        const T &y1 = from.y;
        const T &x2 = to.x;
        const T &y2 = to.y;
        const T &x3 = other.from.x;
        const T &y3 = other.from.y;
        const T &x4 = other.to.x;
        const T &y4 = other.to.y;

        // See https://en.wikipedia.org/wiki/Line-line_intersection
        double d = (x1 - x2) * (y3 - y4) - (y1 - y2) * (x3 - x4);
        if (fabs(d) < 1e-6) {
            return false; // Parallel or coincident
        }
        T a = (x1 * y2 - y1 * x2);
        T b = (x3 * y4 - y3 * x4);
        double x = (a * (x3 - x4) - (x1 - x2) * b) / d;
        double y = (a * (y3 - y4) - (y1 - y2) * b) / d;
        output.x = x;
        output.y = y;
        return true;
    }

    std::string str() const
    {
        std::stringstream ss;
        ss << "Segment(from=" << from.str() << ", to=" << to.str() << ")";
        return ss.str();
    }
};

} // namespace horizon
