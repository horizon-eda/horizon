#include "via.hpp"
#include "board.hpp"
#include "parameter/set.hpp"
#include "nlohmann/json.hpp"
#include "board/board_layers.hpp"
#include "pool/ipool.hpp"
#include "util/util.hpp"

namespace horizon {

static const LutEnumStr<Via::Source> source_lut = {
        {"rules", Via::Source::RULES},
        {"local", Via::Source::LOCAL},
};

Via::Via(const UUID &uu, const json &j, IPool &pool, Board *brd)
    : uuid(uu), pool_padstack(pool.get_padstack(j.at("padstack").get<std::string>())), padstack(*pool_padstack),
      parameter_set(parameter_set_from_json(j.at("parameter_set"))), locked(j.value("locked", false)),
      span(BoardLayers::layer_range_through)
{
    if (brd)
        junction = &brd->junctions.at(j.at("junction").get<std::string>());
    else
        junction.uuid = j.at("junction").get<std::string>();

    if (j.count("net_set")) {
        if (brd)
            net_set = &brd->block->nets.at(j.at("net_set").get<std::string>());
        else
            net_set.uuid = j.at("net_set").get<std::string>();
    }

    if (j.count("span"))
        span = LayerRange(j.at("span"));

    if (j.count("source")) {
        source = source_lut.lookup(j.at("source"));
    }
    else {
        const auto from_rules = j.at("from_rules").get<bool>();
        if (from_rules)
            source = Source::RULES;
        else
            source = Source::LOCAL;
    }
}

Via::Via(const UUID &uu, std::shared_ptr<const Padstack> ps)
    : uuid(uu), pool_padstack(ps), padstack(*pool_padstack), span(BoardLayers::layer_range_through)
{
    parameter_set[ParameterID::VIA_DIAMETER] = .5_mm;
    parameter_set[ParameterID::HOLE_DIAMETER] = .2_mm;
    padstack.apply_parameter_set(parameter_set);
}

Via::Via(shallow_copy_t sh, const Via &other)
    : uuid(other.uuid), net_set(other.net_set), junction(other.junction), pool_padstack(other.pool_padstack),
      padstack(other.padstack.uuid), parameter_set(other.parameter_set), source(other.source), locked(other.locked),
      span(other.span)
{
}

void Via::expand(const Board &brd)
{
    padstack = *pool_padstack;
    ParameterSet ps_via = parameter_set;
    ps_via.emplace(ParameterID::VIA_SOLDER_MASK_EXPANSION, brd.rules.get_parameters().via_solder_mask_expansion);
    padstack.apply_parameter_set(ps_via);
    padstack.expand_inner(brd.get_n_inner_layers(), span);
}

json Via::serialize() const
{
    json j;
    j["junction"] = (std::string)junction->uuid;
    j["padstack"] = (std::string)pool_padstack->uuid;
    j["parameter_set"] = parameter_set_serialize(parameter_set);
    j["from_rules"] = source == Source::RULES;
    j["source"] = source_lut.lookup_reverse(source);
    j["locked"] = locked;
    if (net_set)
        j["net_set"] = (std::string)net_set->uuid;
    if (span != BoardLayers::layer_range_through)
        j["span"] = span.serialize();
    return j;
}
} // namespace horizon
