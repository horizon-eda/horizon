#include "symbol.hpp"
#include "odb_util.hpp"
#include "pool/padstack.hpp"
#include "export_util/tree_writer.hpp"

namespace horizon::ODB {
Symbol::Symbol(const Padstack &ps, int layer)
{
    name = "_" + make_legal_entity_name(ps.name) + "_" + get_layer_name(layer);
    for (const auto &[uu, shape] : ps.shapes) {
        if (layer == shape.layer)
            features.draw_shape(shape);
    }
    for (const auto &[uu, ipoly] : ps.polygons) {
        if (layer == ipoly.layer) {
            auto poly = ipoly;
            if (poly.is_ccw())
                poly.reverse();

            auto &surf = features.add_surface();
            surf.data.append_polygon(poly);
        }
    }
}

void Symbol::write(TreeWriter &writer) const
{
    auto file = writer.create_file("features");
    features.write(file.stream);
}

} // namespace horizon::ODB
