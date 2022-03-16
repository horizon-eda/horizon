#pragma once
#include "common/common.hpp"
#include "util/placement.hpp"
#include "util/uuid.hpp"
#include <list>
#include <set>
#include <variant>
#include "attributes.hpp"
#include "clipper/clipper.hpp"
#include "board/plane.hpp"
#include "eda_data.hpp"
#include "attribute_util.hpp"
#include "features.hpp"
#include "components.hpp"
#include "symbol.hpp"

namespace horizon {
class TreeWriter;
class Padstack;
class Shape;
class Polygon;
class Package;
class Pad;
class BoardPackage;
} // namespace horizon

namespace horizon::ODB {

class Symbol;

enum class Polarity { POSITIVE, NEGATIVE };

class Step {
public:
    std::map<std::string, Features> layer_features;
    std::optional<Features> profile;

    std::optional<Components> comp_top;
    std::optional<Components> comp_bot;

    EDAData eda_data;

    Components::Component &add_component(const BoardPackage &pkg);

    void write(TreeWriter &writer) const;
};

class Matrix {
public:
    std::map<std::string, unsigned int> steps; // step name -> column

    class Layer {
    public:
        Layer(unsigned int r, const std::string &n) : row(r), name(n)
        {
        }

        const unsigned int row;
        const std::string name;

        enum class Context { BOARD, MISC };
        Context context;

        enum class Type {
            SIGNAL,
            SOLDER_MASK,
            SILK_SCREEN,
            SOLDER_PASTE,
            DRILL,
            ROUT,
            DOCUMENT,
            COMPONENT,
        };
        Type type;
        struct Span {
            std::string start;
            std::string end;
        };
        std::optional<Span> span;

        Polarity polarity = Polarity::POSITIVE;
    };
    std::vector<Layer> layers;

    Layer &add_layer(const std::string &name);
    void add_step(const std::string &name);

    void write(std::ostream &ost) const;

private:
    unsigned int row = 1;
    unsigned int col = 1;
};

class Job {
public:
    Matrix matrix;
    Matrix::Layer &add_matrix_layer(const std::string &name);
    Step &add_step(const std::string &name);

    std::string job_name = "board";
    std::map<std::string, Step> steps;

    using SymbolKey = std::tuple<UUID, int, std::string>; // padstack UUID, layer, content hash
    std::map<SymbolKey, Symbol> symbols;
    std::set<std::string> symbol_names;

    std::string get_or_create_symbol(const Padstack &ps, int layer);

    void write(TreeWriter &writer) const;
};

std::string enum_to_string(Polarity);
std::string enum_to_string(Matrix::Layer::Type);
std::string enum_to_string(Matrix::Layer::Context);


} // namespace horizon::ODB
