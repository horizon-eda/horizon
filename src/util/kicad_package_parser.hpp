#pragma once
#include "common/common.hpp"

namespace SEXPR {
class SEXPR;
}

namespace horizon {
class KiCadPackageParser {
public:
    KiCadPackageParser(class Package &p, class Pool *po);
    void parse(const SEXPR::SEXPR *data);

private:
    void parse_line(const SEXPR::SEXPR *data);
    void parse_pad(const SEXPR::SEXPR *data);
    static int get_layer(const std::string &l);
    static Coordi get_coord(const SEXPR::SEXPR *data, size_t offset = 1);
    static Coordi get_size(const SEXPR::SEXPR *data, size_t offset = 1);
    std::map<Coordi, class Junction *> junctions;
    Junction *get_or_create_junction(const Coordi &c);

    Package &package;
    Pool *pool;
};
} // namespace horizon
