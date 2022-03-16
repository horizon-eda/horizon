#include "db.hpp"
#include "board/board_layers.hpp"
#include "export_util/tree_writer.hpp"
#include "structured_text_writer.hpp"
#include "pool/padstack.hpp"
#include "export_util/padstack_hash.hpp"
#include "board/plane.hpp"
#include "util/once.hpp"
#include "util/geom_util.hpp"
#include "util/version.hpp"
#include "block/net_class.hpp"
#include "pool/package.hpp"
#include "board/board_package.hpp"
#include "pool/part.hpp"
#include "util/util.hpp"
#include "odb_util.hpp"

namespace horizon::ODB {

#define MAKE_ENUM_TO_STRING(n)                                                                                         \
    std::string enum_to_string(n value)                                                                                \
    {                                                                                                                  \
        using N = n;                                                                                                   \
        const std::map<n, std::string> items = {ITEMS};                                                                \
        return items.at(value);                                                                                        \
    }

#define X(a)                                                                                                           \
    {                                                                                                                  \
        N::a, #a                                                                                                       \
    }

#define ITEMS X(POSITIVE), X(NEGATIVE)
MAKE_ENUM_TO_STRING(Polarity)
#undef ITEMS

#define ITEMS X(SIGNAL), X(SOLDER_MASK), X(SILK_SCREEN), X(SOLDER_PASTE), X(DRILL), X(DOCUMENT), X(ROUT), X(COMPONENT)
MAKE_ENUM_TO_STRING(Matrix::Layer::Type)
#undef ITEMS


#define ITEMS X(MISC), X(BOARD)
MAKE_ENUM_TO_STRING(Matrix::Layer::Context)
#undef ITEMS


Components::Component &Step::add_component(const BoardPackage &bpkg)
{
    auto &comps = bpkg.flip ? comp_bot.value() : comp_top.value();
    auto &pkg = eda_data.get_package(bpkg.package.uuid);

    auto &comp = comps.components.emplace_back(comps.components.size(), pkg.index);
    comp.placement = bpkg.placement;
    if (bpkg.flip)
        comp.placement.invert_angle();
    comp.comp_name = make_legal_name(bpkg.component->refdes);
    comp.part_name =
            make_legal_name(bpkg.component->part ? bpkg.component->part->get_MPN() : bpkg.component->entity->name);

    return comp;
}

Matrix::Layer &Matrix::add_layer(const std::string &name)
{
    return layers.emplace_back(row++, name);
}

void Matrix::add_step(const std::string &name)
{
    steps.emplace(name, col++);
}


Matrix::Layer &Job::add_matrix_layer(const std::string &name)
{
    for (auto &[step_name, col] : matrix.steps) {
        steps.at(step_name).layer_features[name];
    }

    return matrix.add_layer(name);
}

Step &Job::add_step(const std::string &name)
{
    matrix.add_step(name);
    return steps[name];
}

void Step::write(TreeWriter &writer) const
{
    {
        auto file = writer.create_file("stephdr");
        StructuredTextWriter txt_writer(file.stream);
        txt_writer.write_line("X_DATUM", 0);
        txt_writer.write_line("Y_DATUM", 0);
        txt_writer.write_line("X_ORIGIN", 0);
        txt_writer.write_line("Y_ORIGIN", 0);
    }
    for (const auto &[layer_name, feats] : layer_features) {
        auto file = writer.create_file(fs::path("layers") / layer_name / "features");
        feats.write(file.stream);
    }
    if (comp_top) {
        auto file = writer.create_file("layers/comp_+_top/components");
        comp_top->write(file.stream);
    }
    if (comp_bot) {
        auto file = writer.create_file("layers/comp_+_bot/components");
        comp_bot->write(file.stream);
    }
    if (profile) {
        auto file = writer.create_file("profile");
        profile->write(file.stream);
    }
    {
        auto file = writer.create_file("eda/data");
        eda_data.write(file.stream);
    }
}

std::string Job::get_or_create_symbol(const Padstack &ps, int layer)
{
    // try to use built-in symbol first
    {
        std::vector<const Polygon *> layer_polys;
        std::vector<const Shape *> layer_shapes;
        for (const auto &[uu, it] : ps.polygons) {
            if (it.layer == layer)
                layer_polys.push_back(&it);
        }
        for (const auto &[uu, it] : ps.shapes) {
            if (it.layer == layer)
                layer_shapes.push_back(&it);
        }
        if (layer_shapes.size() == 1 && layer_polys.size() == 0) {
            auto &sh = *layer_shapes.front();
            if (sh.form == Shape::Form::CIRCLE && sh.placement.shift == Coordi())
                return make_symbol_circle(sh.params.at(0));
            else if (sh.form == Shape::Form::RECTANGLE && sh.placement.shift == Coordi()
                     && sh.placement.get_angle() == 0)
                return make_symbol_rect(sh.params.at(0), sh.params.at(1));
            else if (sh.form == Shape::Form::OBROUND && sh.placement.shift == Coordi() && sh.placement.get_angle() == 0)
                return make_symbol_oval(sh.params.at(0), sh.params.at(1));
        }
    }

    const auto hash = PadstackHash::hash(ps);
    const auto key = std::make_tuple(ps.uuid, layer, hash);
    if (symbols.count(key)) {
        return symbols.at(key).name;
    }
    else {
        auto &sym =
                symbols.emplace(std::piecewise_construct, std::forward_as_tuple(key), std::forward_as_tuple(ps, layer))
                        .first->second;
        if (symbol_names.count(sym.name)) {
            const std::string name_orig = sym.name;
            unsigned int i = 1;
            do {
                sym.name = name_orig + "_" + std::to_string(i++);
            } while (symbol_names.count(sym.name));
            symbol_names.insert(sym.name);
        }
        else {
            symbol_names.insert(sym.name);
        }
        return sym.name;
    }
}

void Job::write(TreeWriter &top_writer) const
{
    TreeWriterPrefixed writer{top_writer, job_name};
    {
        auto file = writer.create_file("matrix/matrix");
        matrix.write(file.stream);
    }
    for (const auto &[key, sym] : symbols) {
        TreeWriterPrefixed wr_prefixed(writer, fs::path("symbols") / sym.name);
        sym.write(wr_prefixed);
    }
    for (const auto &[step_name, step] : steps) {
        TreeWriterPrefixed wr_prefixed(writer, fs::path("steps") / step_name);
        step.write(wr_prefixed);
    }
    {
        auto file = writer.create_file("misc/info");
        StructuredTextWriter twriter(file.stream);
        twriter.write_line("UNITS", "MM");
        twriter.write_line("ODB_VERSION_MAJOR", 8);
        twriter.write_line("ODB_VERSION_MINOR", 1);
        twriter.write_line("CREATION_DATE", "20220309.133742");
        twriter.write_line("SAVE_DATE", "20220309.133742");
        twriter.write_line("ODB_SOURCE", "Horizon EDA");
        twriter.write_line("JOB_NAME", job_name);
        twriter.write_line("SAVE_APP", "Horizon EDA Version " + Version::get_string());
    }
}

void Matrix::write(std::ostream &ost) const
{
    StructuredTextWriter writer(ost);
    for (const auto &[step_name, column] : steps) {
        const auto a = writer.make_array_proxy("STEP");
        writer.write_line("COL", column);
        writer.write_line("NAME", step_name);
    }
    for (const auto &layer : layers) {
        const auto a = writer.make_array_proxy("LAYER");
        writer.write_line("ROW", layer.row);
        writer.write_line_enum("CONTEXT", layer.context);
        writer.write_line_enum("TYPE", layer.type);
        writer.write_line("NAME", layer.name);
        writer.write_line_enum("POLARITY", layer.polarity);
        if (layer.span) {
            writer.write_line("START_NAME", layer.span->start);
            writer.write_line("END_NAME", layer.span->end);
        }
    }
}

} // namespace horizon::ODB
