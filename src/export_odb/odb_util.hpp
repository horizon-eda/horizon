#pragma once
#include "common/common.hpp"
#include "util/placement.hpp"
#include <ostream>

namespace horizon {
class LayerRange;
}
namespace horizon::ODB {

extern const char *endl;

std::ostream &operator<<(std::ostream &os, const Coordi &c);

struct Angle {
    explicit Angle(int a) : angle((((65536 - a) % 65536) * (360. / 65536.)))
    {
    }
    explicit Angle(const Placement &pl) : Angle(pl.get_angle())
    {
    }
    const double angle;
};

std::ostream &operator<<(std::ostream &os, Angle a);

struct Dim {
    explicit Dim(int64_t x) : dim(x / 1e6)
    {
    }
    explicit Dim(uint64_t x) : dim(x / 1e6)
    {
    }
    explicit Dim(double x) : dim(x / 1e6)
    {
    }
    const double dim;
};

std::ostream &operator<<(std::ostream &os, Dim d);

struct DimUm {
    explicit DimUm(int64_t x) : dim(x / 1e3)
    {
    }
    explicit DimUm(uint64_t x) : dim(x / 1e3)
    {
    }
    const double dim;
};

std::ostream &operator<<(std::ostream &os, DimUm d);

std::string utf8_to_ascii(const std::string &s);
std::string make_legal_name(const std::string &n);
std::string make_legal_entity_name(const std::string &s);
std::string get_layer_name(int id);
std::string get_drills_layer_name(const LayerRange &span);

std::string make_symbol_circle(uint64_t diameter);
std::string make_symbol_rect(uint64_t w, uint64_t h);
std::string make_symbol_oval(uint64_t w, uint64_t h);


} // namespace horizon::ODB
