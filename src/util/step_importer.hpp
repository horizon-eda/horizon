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

class Vertex {
private:
    auto as_tuple() const
    {
        return std::make_tuple(x, y, z);
    }

public:
    Vertex(float ix, float iy, float iz) : x(ix), y(iy), z(iz)
    {
    }

    float x, y, z;

    bool operator==(const Vertex &other) const
    {
        return x == other.x && y == other.y && z == other.z;
    }

    bool operator<(const Vertex &other) const
    {
        return as_tuple() < other.as_tuple();
    }

    auto &operator+=(const Vertex &other)
    {
        x += other.x;
        y += other.y;
        z += other.z;
        return *this;
    }

    auto &operator/=(float other)
    {
        x /= other;
        y /= other;
        z /= other;
        return *this;
    }
};

class Face {
public:
    Color color;
    std::vector<Vertex> vertices;
    std::vector<Vertex> normals;
    std::vector<std::tuple<size_t, size_t, size_t>> triangle_indices;
};

std::deque<Face> import(const std::string &filename);
} // namespace horizon::STEPImporter
