#pragma once
#include <string>
#include <list>
#include <vector>
#include "common/common.hpp"
#include "pool/unit.hpp"
#include "pool/symbol.hpp"
#include "pool/entity.hpp"
#include "pool/part.hpp"

namespace horizon {


class KiCadSymbol {
public:
    std::string name;
    std::string prefix;
    std::string footprint;
    std::string datasheet;
    std::string description;
    std::vector<std::string> fplist;
    class SPart {
    public:
        class SRect {
        public:
            Coordi from;
            Coordi to;
        };
        std::list<SRect> rects;
        class SPoly {
        public:
            std::vector<Coordi> vertices;
            bool is_closed = false;
        };
        std::list<SPoly> polys;

        class SPin {
        public:
            std::string name;
            std::string pad;
            Coordi pos;
            int64_t length;
            Orientation orientation;
            Pin::Direction direction;
            SymbolPin::Decoration decoration;
        };
        std::list<SPin> pins;
    };
    std::vector<SPart> parts;
    unsigned int get_n_pins() const;

    class SPartIterProxy {
    public:
        SPartIterProxy(std::vector<SPart> &p, int i) : parts(p), idx(i)
        {
        }
        auto begin()
        {
            if (idx == 0) {
                return parts.begin();
            }
            else if (idx > parts.size()) {
                return parts.begin();
            }
            else {
                return parts.begin() + (idx - 1);
            }
        }

        auto end()
        {
            if (idx == 0) {
                return parts.end();
            }
            else if (idx > parts.size()) {
                return parts.begin();
            }
            else {
                return begin() + 1;
            }
        }

    private:
        std::vector<SPart> &parts;
        const size_t idx;
    };

    SPartIterProxy get_parts(int idx);
};

std::list<KiCadSymbol> parse_kicad_library(const std::string &filename);
class KiCadSymbolImporter {
public:
    KiCadSymbolImporter(const KiCadSymbol &sym, const class Package *pkg, bool merge_pins);

    const Entity &get_entity();
    const Part *get_part();
    const std::list<Unit> &get_units();
    const std::list<Symbol> &get_symbols();

private:
    Entity entity;
    Part part;
    std::list<Unit> units;
    std::list<Symbol> symbols;
    // const KiCadSymbol &symbol;
    // const class Package *package;
};

} // namespace horizon
