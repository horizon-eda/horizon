#pragma once
#include <deque>
#include <string>
#include <vector>
#include <tuple>

namespace horizon::STEPImporter {
class Color {
public:
    float r;
    float g;
    float b;
    Color(double ir, double ig, double ib) : r(ir), g(ig), b(ib)
    {
    }
    Color() : r(0), g(0), b(0)
    {
    }
};

template <typename T> class TVertex {
private:
    auto as_tuple() const
    {
        return std::make_tuple(x, y, z);
    }

public:
    TVertex(T ix, T iy, T iz) : x(ix), y(iy), z(iz)
    {
    }

    T x, y, z;

    bool operator==(const TVertex &other) const
    {
        return x == other.x && y == other.y && z == other.z;
    }

    bool operator<(const TVertex &other) const
    {
        return as_tuple() < other.as_tuple();
    }

    auto &operator+=(const TVertex &other)
    {
        x += other.x;
        y += other.y;
        z += other.z;
        return *this;
    }

    auto &operator/=(T other)
    {
        x /= other;
        y /= other;
        z /= other;
        return *this;
    }
};

using Vertex = TVertex<float>;

class Face {
public:
    Color color;
    std::vector<Vertex> vertices;
    std::vector<Vertex> normals;
    std::vector<std::tuple<size_t, size_t, size_t>> triangle_indices;
};

using Point = TVertex<double>;

class Result {
public:
    std::deque<Face> faces;
    std::deque<Point> points;
};

Result import(const std::string &filename);
} // namespace horizon::STEPImporter
