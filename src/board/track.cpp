#include "track.hpp"
#include "board.hpp"
#include "board_layers.hpp"
#include "board_package.hpp"
#include "common/lut.hpp"
#include "nlohmann/json.hpp"
#include "util/geom_util.hpp"

namespace horizon {

Track::Connection::Connection(const json &j, Board *brd)
{
    if (!j.at("junc").is_null()) {
        if (brd)
            junc = &brd->junctions.at(j.at("junc").get<std::string>());
        else
            junc.uuid = j.at("junc").get<std::string>();
    }
    else if (!j.at("pad").is_null()) {
        UUIDPath<2> pad_path(j.at("pad").get<std::string>());
        if (brd) {
            package = &brd->packages.at(pad_path.at(0));
            pad = &package->package.pads.at(pad_path.at(1));
        }
        else {
            package.uuid = pad_path.at(0);
            pad.uuid = pad_path.at(1);
        }
        if (j.count("offset"))
            offset = j.at("offset").get<std::vector<int64_t>>();
    }
    else {
        assert(false);
    }
}

Track::Connection::Connection(BoardJunction *j)
{
    connect(j);
}
Track::Connection::Connection(BoardPackage *pkg, Pad *pa)
{
    connect(pkg, pa);
}

UUIDPath<2> Track::Connection::get_pad_path() const
{
    assert(junc == nullptr);
    return UUIDPath<2>(package->uuid, pad->uuid);
}

bool Track::Connection::is_junc() const
{
    if (junc) {
#ifdef CONNECTION_CHECK
        assert(!is_pad());
#endif
        return true;
    }
    return false;
}

bool Track::Connection::is_pad() const
{
    if (package) {
#ifdef CONNECTION_CHECK
        assert(pad);
        assert(!is_junc());
#endif
        return true;
    }
    return false;
}

void Track::Connection::connect(BoardJunction *j)
{
    junc = j;
    package = nullptr;
    pad = nullptr;
    offset = Coordi();
}

void Track::Connection::connect(BoardPackage *pkg, Pad *pa)
{
    junc = nullptr;
    package = pkg;
    pad = pa;
    offset = Coordi();
}

void Track::Connection::update_refs(Board &brd)
{
    junc.update(brd.junctions);
    package.update(brd.packages);
    if (package)
        pad.update(package->package.pads);
}

Coordi Track::Connection::get_position() const
{
    if (is_junc()) {
        return junc->position;
    }
    else if (is_pad()) {
        auto tr = package->placement;
        if (package->flip)
            tr.invert_angle();
        return tr.transform(pad->placement.shift + offset);
    }
    else {
        assert(false);
    }
}

LayerRange Track::Connection::get_layer() const
{
    if (is_junc()) {
        return junc->layer;
    }
    else if (is_pad()) {
        if (pad->padstack.type == Padstack::Type::TOP) {
            return BoardLayers::TOP_COPPER;
        }
        else if (pad->padstack.type == Padstack::Type::BOTTOM) {
            return BoardLayers::BOTTOM_COPPER;
        }
        else if (pad->padstack.type == Padstack::Type::THROUGH) {
            return LayerRange(BoardLayers::TOP_COPPER, BoardLayers::BOTTOM_COPPER);
        }
    }
    assert(false);
    return 10000;
}

json Track::Connection::serialize() const
{
    json j;
    j["junc"] = nullptr;
    j["pad"] = nullptr;
    if (is_junc()) {
        j["junc"] = (std::string)junc->uuid;
    }
    else if (is_pad()) {
        j["pad"] = (std::string)get_pad_path();
        if (offset.x || offset.y)
            j["offset"] = offset.as_array();
    }
    else {
        assert(false);
    }
    return j;
}

Net *Track::Connection::get_net()
{
    if (is_junc()) {
        return junc->net;
    }
    else if (is_pad()) {
        return pad->net;
    }
    else {
        assert(false);
        return nullptr;
    }
}

bool Track::Connection::operator<(const Track::Connection &other) const
{
    if (junc < other.junc)
        return true;
    if (junc > other.junc)
        return false;
    return pad < other.pad;
}

Track::Track(const UUID &uu, const json &j, Board *brd)
    : uuid(uu), layer(j.value("layer", 0)), width(j.value("width", 0)),
      width_from_rules(j.value("width_from_net_class", true)), locked(j.value("locked", false)),
      from(j.at("from"), brd), to(j.at("to"), brd)
{
    if (j.count("center"))
        center = j.at("center").get<std::vector<int64_t>>();
}

void Track::update_refs(Board &brd)
{
    to.update_refs(brd);
    from.update_refs(brd);
    net.update(brd.block->nets);
}

UUID Track::get_uuid() const
{
    return uuid;
}

Track::Track(const UUID &uu) : uuid(uu)
{
}

json Track::serialize() const
{
    json j;
    j["from"] = from.serialize();
    j["to"] = to.serialize();
    j["layer"] = layer;
    j["width"] = width;
    j["width_from_net_class"] = width_from_rules;
    j["locked"] = locked;
    if (center.has_value())
        j["center"] = center->as_array();

    return j;
}

bool Track::is_parallel_to(const Track &other) const
{
    if (is_arc() || other.is_arc())
        return false;
    const auto zero = Coordi();
    const auto v1 = to.get_position() - from.get_position();
    const auto v2 = other.to.get_position() - other.from.get_position();
    if (v1 == zero || v2 == zero)
        return false; // Direction undefined
    return v1.cross(v2) == 0;
}

bool Track::is_arc() const
{
    return center.has_value();
}

double Track::get_length() const
{
    if (is_arc()) {
        Coordf a(from.get_position()); // ,b,c;
        Coordf b(to.get_position());   // ,b,c;
        const auto real_center = project_onto_perp_bisector(a, b, center.value());
        const auto radius = (real_center - a).mag();
        const auto arc_a0 = c2pi(atan2f(a.y - real_center.y, a.x - real_center.x));
        const auto arc_a1 = c2pi(atan2f(b.y - real_center.y, b.x - real_center.x));
        const auto dphi = c2pi(arc_a1 - arc_a0);
        return radius * dphi;
    }
    else {
        return (from.get_position() - to.get_position()).magd();
    }
}

} // namespace horizon
