#include "eda_data.hpp"
#include "block/net.hpp"
#include "block/net_class.hpp"
#include "common/polygon.hpp"
#include "pool/package.hpp"
#include "board/board_layers.hpp"
#include "util/util.hpp"
#include "odb_util.hpp"

namespace horizon::ODB {

EDAData::EDAData()
{
    auto &x = nets_map.emplace(std::piecewise_construct, std::forward_as_tuple(UUID{}),
                               std::forward_as_tuple(nets.size(), "$NONE$"))
                      .first->second;
    nets.push_back(&x);
}

EDAData::Net::Net(unsigned int i, const std::string &n) : index(i), name(make_legal_name(n))
{
}

void EDAData::Net::write(std::ostream &ost) const
{
    ost << "NET " << name;
    write_attributes(ost);
    ost << endl;

    for (const auto &subnet : subnets) {
        subnet->write(ost);
    }
}

static std::string get_net_name(const horizon::Net &net)
{
    std::string net_name;
    if (net.is_named())
        return net.name;
    else
        return "$" + static_cast<std::string>(net.uuid);
}

EDAData::Net &EDAData::add_net(const horizon::Net &net)
{
    const auto net_name = get_net_name(net);
    auto &x = nets_map.emplace(std::piecewise_construct, std::forward_as_tuple(net.uuid),
                               std::forward_as_tuple(nets.size(), net_name))
                      .first->second;
    nets.push_back(&x);
    add_attribute(x, attribute::net_type{net.net_class->name});
    if (net.diffpair) {
        // ensure attribute value is shorter than 64 chars
        auto n1 = make_legal_name(net_name).substr(0, 31);
        auto n2 = make_legal_name(get_net_name(*net.diffpair)).substr(0, 31);
        if (n1 > n2)
            std::swap(n1, n2);
        add_attribute(x, attribute::diff_pair{n1 + "," + n2});
    }

    return x;
}

unsigned int EDAData::get_or_create_layer(const std::string &l)
{
    if (layers_map.count(l)) {
        return layers_map.at(l);
    }
    else {
        auto n = layers_map.size();
        layers_map.emplace(l, n);
        layers.push_back(l);
        assert(layers.size() == layers_map.size());
        return n;
    }
}

void EDAData::Subnet::write(std::ostream &ost) const
{
    ost << "SNT ";
    write_subnet(ost);
    ost << endl;
    for (const auto &fid : feature_ids) {
        fid.write(ost);
    }
}

void EDAData::FeatureID::write(std::ostream &ost) const
{
    static const std::map<Type, std::string> type_map = {
            {Type::COPPER, "C"},
            {Type::HOLE, "H"},
    };
    ost << "FID " << type_map.at(type) << " " << layer << " " << feature_id << endl;
}

void EDAData::SubnetVia::write_subnet(std::ostream &ost) const
{
    ost << "VIA";
}

void EDAData::SubnetTrace::write_subnet(std::ostream &ost) const
{
    ost << "TRC";
}

void EDAData::SubnetPlane::write_subnet(std::ostream &ost) const
{
    static const std::map<FillType, std::string> fill_type_map = {
            {FillType::SOLID, "S"},
    };
    static const std::map<CutoutType, std::string> cutout_type_map = {
            {CutoutType::CIRCLE, "C"},
    };
    ost << "PLN " << fill_type_map.at(fill_type) << " " << cutout_type_map.at(cutout_type) << " " << Dim{fill_size};
}

void EDAData::SubnetToeprint::write_subnet(std::ostream &ost) const
{
    static const std::map<Side, std::string> side_map = {
            {Side::BOTTOM, "B"},
            {Side::TOP, "T"},
    };
    ost << "TOP " << side_map.at(side) << " " << comp_num << " " << toep_num;
}

void EDAData::add_feature_id(Subnet &subnet, FeatureID::Type type, const std::string &layer, unsigned int feature_id)
{
    subnet.feature_ids.emplace_back(type, get_or_create_layer(layer), feature_id);
}


static std::unique_ptr<EDAData::Outline> poly_as_rectangle_or_square(const Polygon &poly)
{
    if (!poly.is_rect())
        return nullptr;
    const auto &p0 = poly.vertices.at(0).position;
    const auto &p1 = poly.vertices.at(1).position;
    const auto &p2 = poly.vertices.at(2).position;
    const auto &p3 = poly.vertices.at(3).position;
    const auto v0 = p1 - p0;
    const auto v1 = p2 - p1;
    const Coordi bottom_left = Coordi::min(p0, Coordi::min(p1, Coordi::min(p2, p3)));
    uint64_t w;
    uint64_t h;
    if (v0.y == 0) {
        assert(v1.x == 0);
        w = std::abs(v0.x);
        h = std::abs(v1.y);
    }
    else if (v0.x == 0) {
        assert(v1.y == 0);
        w = std::abs(v1.x);
        h = std::abs(v0.y);
    }
    else {
        assert(false);
    }
    if (w == h) {
        const auto hs = w / 2;
        return std::make_unique<EDAData::OutlineSquare>(bottom_left + Coordi(hs, hs), hs);
    }
    else {
        return std::make_unique<EDAData::OutlineRectangle>(bottom_left, w, h);
    }
}

static std::unique_ptr<EDAData::Outline> outline_contour_from_polygon(const Polygon &poly)
{
    auto r = std::make_unique<EDAData::OutlineContour>();
    r->data.append_polygon_auto_orientation(poly);
    return r;
}

static std::unique_ptr<EDAData::Outline> outline_from_polygon(const Polygon &poly)
{
    if (auto x = poly_as_rectangle_or_square(poly))
        return x;
    return outline_contour_from_polygon(poly);
}

void EDAData::OutlineSquare::write(std::ostream &ost) const
{
    ost << "SQ " << center << " " << Dim{half_side} << endl;
}

void EDAData::OutlineCircle::write(std::ostream &ost) const
{
    ost << "CR " << center << " " << Dim{radius} << endl;
}

void EDAData::OutlineRectangle::write(std::ostream &ost) const
{
    ost << "RC " << lower << " " << Dim{width} << " " << Dim{height} << endl;
}

void EDAData::OutlineContour::write(std::ostream &ost) const
{
    ost << "CT" << endl;
    data.write(ost);
    ost << "CE" << endl;
}

EDAData::Package::Package(const unsigned int i, const std::string &n) : index(i), name(make_legal_name(n))
{
}

EDAData::Package &EDAData::add_package(const horizon::Package &pkg)
{
    auto &x = packages_map
                      .emplace(std::piecewise_construct, std::forward_as_tuple(pkg.uuid),
                               std::forward_as_tuple(packages.size(), pkg.name))
                      .first->second;
    packages.push_back(&x);
    const auto bb = pkg.get_bbox();
    x.xmin = bb.first.x;
    x.ymin = bb.first.y;
    x.xmax = bb.second.x;
    x.ymax = bb.second.y;
    x.pitch = UINT64_MAX;
    if (pkg.pads.size() < 2)
        x.pitch = 1_mm; // placeholder value to not break anything

    for (auto it = pkg.pads.begin(); it != pkg.pads.end(); it++) {
        auto it2 = it;
        it2++;
        for (; it2 != pkg.pads.end(); it2++) {
            const uint64_t pin_dist = Coordd(it->second.placement.shift - it2->second.placement.shift).mag();
            x.pitch = std::min(x.pitch, pin_dist);
        }
    }

    std::set<const Polygon *> outline_polys;
    for (const auto &[uu, poly] : pkg.polygons) {
        // check both layers since we might be looking at a flipped package
        if ((poly.layer == BoardLayers::TOP_PACKAGE) || (poly.layer == BoardLayers::BOTTOM_PACKAGE)) {
            outline_polys.insert(&poly);
        }
    }

    if (outline_polys.size() == 1) {
        x.outline.push_back(outline_from_polygon(**outline_polys.begin()));
    }
    else if (outline_polys.size() > 1) {
        for (auto poly : outline_polys)
            x.outline.push_back(outline_contour_from_polygon(*poly));
    }
    else {
        x.outline.push_back(std::make_unique<OutlineRectangle>(bb));
    }

    {
        auto pads_sorted = pkg.get_pads_sorted();
        for (const auto pad : pads_sorted) {
            x.add_pin(*pad);
        }
    }

    return x;
}

EDAData::Pin::Pin(unsigned int i, const std::string &n) : name(make_legal_name(n)), index(i)
{
}

EDAData::Pin &EDAData::Package::add_pin(const horizon::Pad &pad)
{
    auto &pin = pins_map.emplace(std::piecewise_construct, std::forward_as_tuple(pad.uuid),
                                 std::forward_as_tuple(pins.size(), pad.name))
                        .first->second;
    pins.push_back(&pin);
    pin.center = pad.placement.shift;
    switch (pad.padstack.type) {
    case Padstack::Type::THROUGH:
        pin.type = Pin::Type::THROUGH_HOLE;
        pin.mtype = Pin::MountType::THROUGH_HOLE;
        pin.etype = Pin::ElectricalType::ELECTRICAL;
        break;

    case Padstack::Type::TOP:
    case Padstack::Type::BOTTOM:
        pin.type = Pin::Type::SURFACE;
        pin.mtype = Pin::MountType::SMT;
        pin.etype = Pin::ElectricalType::ELECTRICAL;
        break;

    case Padstack::Type::MECHANICAL:
        pin.type = Pin::Type::THROUGH_HOLE;
        pin.mtype = Pin::MountType::THROUGH_HOLE;
        pin.etype = Pin::ElectricalType::MECHANICAL;
        break;

    default:;
    }

    const auto &ps = pad.padstack;
    std::set<const Shape *> shapes_top;
    for (const auto &[uu, sh] : ps.shapes) {
        if (sh.layer == BoardLayers::TOP_COPPER)
            shapes_top.insert(&sh);
    }
    const auto n_polys_top = std::count_if(ps.polygons.begin(), ps.polygons.end(),
                                           [](auto &it) { return it.second.layer == BoardLayers::TOP_COPPER; });
    if (shapes_top.size() == 1 && n_polys_top == 0 && (*shapes_top.begin())->form == Shape::Form::CIRCLE) {
        const auto &sh = **shapes_top.begin();
        pin.outline.push_back(
                std::make_unique<OutlineCircle>(pad.placement.transform(sh.placement.shift), sh.params.at(0) / 2));
    }
    else {
        const auto bb = ps.get_bbox(true);
        pin.outline.push_back(std::make_unique<OutlineRectangle>(pad.placement.transform_bb(bb)));
    }
    return pin;
}

void EDAData::Pin::write(std::ostream &ost) const
{
    static const std::map<Type, std::string> type_map = {
            {Type::SURFACE, "S"},
            {Type::THROUGH_HOLE, "T"},
    };
    static const std::map<ElectricalType, std::string> etype_map = {
            {ElectricalType::ELECTRICAL, "E"},
            {ElectricalType::MECHANICAL, "M"},
            {ElectricalType::UNDEFINED, "U"},
    };
    static const std::map<MountType, std::string> mtype_map = {
            {MountType::THROUGH_HOLE, "T"},
            {MountType::SMT, "S"},
            {MountType::UNDEFINED, "U"},
    };
    ost << "PIN " << name << " " << type_map.at(type) << " " << center << " 0 " << etype_map.at(etype) << " "
        << mtype_map.at(mtype) << endl;
    for (const auto &ol : outline) {
        ol->write(ost);
    }
}


void EDAData::Package::write(std::ostream &ost) const
{
    ost << "PKG " << name << " " << Dim{pitch} << " " << Dim{xmin} << " " << Dim{ymin} << " " << Dim{xmax} << " "
        << Dim{ymax} << endl;

    for (const auto &ol : outline) {
        ol->write(ost);
    }
    for (const auto &pin : pins) {
        pin->write(ost);
    }
}

void EDAData::write(std::ostream &ost) const
{
    ost << "HDR Horizon EDA" << endl;
    ost << "UNITS=MM" << endl;
    ost << "LYR";

    for (const auto &layer : layers) {

        ost << " " << layer;
    }

    ost << endl;

    write_attributes(ost, "#");

    for (const auto &net : nets) {
        net->write(ost);
    }
    for (const auto pkg : packages) {
        pkg->write(ost);
    }
}
} // namespace horizon::ODB
