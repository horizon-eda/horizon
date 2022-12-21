#include "pns_horizon_iface.hpp"
#include "board/board.hpp"
#include "board/board_layers.hpp"
#include "canvas/canvas_gl.hpp"
#include "clipper/clipper.hpp"
#include "geometry/shape_simple.h"
#include "geometry/shape_rect.h"
#include "router/pns_debug_decorator.h"
#include "router/pns_solid.h"
#include "router/pns_topology.h"
#include "router/pns_via.h"
#include "router/pns_arc.h"
#include "util/geom_util.hpp"
#include "logger/logger.hpp"
#include "pool/ipool.hpp"
#include "router/layer_ids.h"

namespace PNS {

int PNS_HORIZON_IFACE::layer_to_router(int l)
{
    switch (l) {
    case horizon::BoardLayers::TOP_COPPER:
        return F_Cu;
    case horizon::BoardLayers::BOTTOM_COPPER:
        return B_Cu;
    case horizon::BoardLayers::IN1_COPPER:
        return In1_Cu;
    case horizon::BoardLayers::IN2_COPPER:
        return In2_Cu;
    case horizon::BoardLayers::IN3_COPPER:
        return In3_Cu;
    case horizon::BoardLayers::IN4_COPPER:
        return In4_Cu;
    default:
        return UNDEFINED_LAYER;
    }

    return F_Cu;
}

int PNS_HORIZON_IFACE::layer_from_router(int l)
{
    int lo = 0;
    switch (l) {
    case F_Cu:
        lo = horizon::BoardLayers::TOP_COPPER;
        break;
    case B_Cu:
        lo = horizon::BoardLayers::BOTTOM_COPPER;
        break;
    case In1_Cu:
        lo = horizon::BoardLayers::IN1_COPPER;
        break;
    case In2_Cu:
        lo = horizon::BoardLayers::IN2_COPPER;
        break;
    case In3_Cu:
        lo = horizon::BoardLayers::IN3_COPPER;
        break;
    case In4_Cu:
        lo = horizon::BoardLayers::IN4_COPPER;
        break;
    case UNDEFINED_LAYER:
        lo = 10000;
        break;
    default:
        assert(false);
    }
    assert(l == layer_to_router(lo));
    return lo;
}

class PNS_HORIZON_RULE_RESOLVER : public PNS::RULE_RESOLVER {
public:
    PNS_HORIZON_RULE_RESOLVER(const horizon::Board *aBoard, const horizon::BoardRules *aRules,
                              PNS_HORIZON_IFACE *aIface);
    virtual ~PNS_HORIZON_RULE_RESOLVER();

    int Clearance(const PNS::ITEM *aA, const PNS::ITEM *aB) override;
    int HoleClearance(const PNS::ITEM *aA, const PNS::ITEM *aB) override;
    int HoleToHoleClearance(const PNS::ITEM *aA, const PNS::ITEM *aB) override;
    int DpCoupledNet(int aNet) override;
    int DpNetPolarity(int aNet) override;
    bool DpNetPair(const PNS::ITEM *aItem, int &aNetP, int &aNetN) override;
    wxString NetName(int aNet) override;

    bool IsDiffPair(const PNS::ITEM *aA, const PNS::ITEM *aB) override;

    bool QueryConstraint(CONSTRAINT_TYPE aType, const PNS::ITEM *aItemA, const PNS::ITEM *aItemB, int aLayer,
                         PNS::CONSTRAINT *aConstraint) override;

private:
    const horizon::BoardRules *m_rules;
    PNS_HORIZON_IFACE *m_iface = nullptr;
    std::vector<const horizon::RuleClearanceCopperKeepout *> m_rules_keepout;
};

PNS_HORIZON_RULE_RESOLVER::PNS_HORIZON_RULE_RESOLVER(const horizon::Board *aBoard, const horizon::BoardRules *rules,
                                                     PNS_HORIZON_IFACE *aIface)
    : m_rules(rules), m_iface(aIface)
{
    PNS::NODE *world = m_iface->GetWorld();

    PNS::TOPOLOGY topo(world);

    m_rules_keepout = m_rules->get_rules_sorted<horizon::RuleClearanceCopperKeepout>();
}

PNS_HORIZON_RULE_RESOLVER::~PNS_HORIZON_RULE_RESOLVER()
{
}

static horizon::PatchType patch_type_from_kind(PNS::ITEM::PnsKind kind)
{
    switch (kind) {
    case PNS::ITEM::VIA_T:
        return horizon::PatchType::VIA;
    case PNS::ITEM::SOLID_T:
        return horizon::PatchType::PAD;
    case PNS::ITEM::LINE_T:
    case PNS::ITEM::SEGMENT_T:
    case PNS::ITEM::ARC_T:
        return horizon::PatchType::TRACK;
    default:;
    }
    assert(false);
    return horizon::PatchType::OTHER;
}

static const PNS_HORIZON_PARENT_ITEM parent_dummy_outline;

int PNS_HORIZON_RULE_RESOLVER::Clearance(const PNS::ITEM *aA, const PNS::ITEM *aB)
{
    if (aB == nullptr)
        return 1e6;

    auto net_a = m_iface->get_net_for_code(aA->Net());
    auto net_b = m_iface->get_net_for_code(aB->Net());

    auto pt_a = patch_type_from_kind(aA->Kind());
    auto pt_b = patch_type_from_kind(aB->Kind());

    auto parent_a = aA->Parent();
    auto parent_b = aB->Parent();

    auto layers_a = aA->Layers();
    auto layers_b = aB->Layers();

    if (parent_a && parent_a->pad && parent_a->pad->padstack.type == horizon::Padstack::Type::THROUGH)
        pt_a = horizon::PatchType::PAD_TH;
    else if (parent_a && parent_a->hole && parent_a->hole->padstack.type == horizon::Padstack::Type::HOLE)
        pt_a = horizon::PatchType::HOLE_PTH;
    else if (parent_a && parent_a->hole && parent_a->hole->padstack.type == horizon::Padstack::Type::MECHANICAL)
        pt_a = horizon::PatchType::HOLE_NPTH;

    if (parent_b && parent_b->pad && parent_b->pad->padstack.type == horizon::Padstack::Type::THROUGH)
        pt_b = horizon::PatchType::PAD_TH;
    else if (parent_b && parent_b->hole && parent_b->hole->padstack.type == horizon::Padstack::Type::HOLE)
        pt_b = horizon::PatchType::HOLE_PTH;
    else if (parent_b && parent_b->hole && parent_b->hole->padstack.type == horizon::Padstack::Type::MECHANICAL)
        pt_b = horizon::PatchType::HOLE_NPTH;

    // std::cout << "pt " << static_cast<int>(pt_a) << " " << static_cast<int>(pt_b) << std::endl;

    if (parent_a == &parent_dummy_outline || parent_b == &parent_dummy_outline) { // one is the board edge
        auto a_is_edge = parent_a == &parent_dummy_outline;

        // use layers of non-edge thing
        auto layers = a_is_edge ? layers_b : layers_a;
        auto pt = a_is_edge ? pt_b : pt_a;
        auto net = net_a ? net_a : net_b; // only one has net

        // fixme: handle multiple layers for non-edge thing
        auto layer = layers.Start();

        const auto &clearance = m_rules->get_clearance_copper_other(net, PNS_HORIZON_IFACE::layer_from_router(layer));
        int64_t routing_offset = clearance.routing_offset;
        if (m_iface->get_override_routing_offset() >= 0)
            routing_offset = m_iface->get_override_routing_offset();
        return clearance.get_clearance(pt, horizon::PatchType::BOARD_EDGE) + routing_offset;
    }

    if ((parent_a && parent_a->pad && parent_a->pad->padstack.type == horizon::Padstack::Type::MECHANICAL)
        || (parent_b && parent_b->pad && parent_b->pad->padstack.type == horizon::Padstack::Type::MECHANICAL)
        || (parent_a && parent_a->hole && parent_a->hole->padstack.type == horizon::Padstack::Type::MECHANICAL)
        || (parent_b && parent_b->hole && parent_b->hole->padstack.type == horizon::Padstack::Type::MECHANICAL)) {
        // std::cout << "npth" << std::endl;
        // any is mechanical (NPTH)
        auto net = net_a ? net_a : net_b; // only one has net
        auto a_is_npth =
                (parent_a && parent_a->pad && parent_a->pad->padstack.type == horizon::Padstack::Type::MECHANICAL)
                || (parent_a && parent_a->hole && parent_a->hole->padstack.type == horizon::Padstack::Type::MECHANICAL);

        // use layers of non-npth ting
        auto layers = a_is_npth ? layers_b : layers_a;
        auto pt = a_is_npth ? pt_b : pt_a;

        // fixme: handle multiple layers for non-npth thing
        auto layer = layers.Start();

        const auto &clearance = m_rules->get_clearance_copper_other(net, PNS_HORIZON_IFACE::layer_from_router(layer));
        int64_t routing_offset = clearance.routing_offset;
        if (m_iface->get_override_routing_offset() >= 0)
            routing_offset = m_iface->get_override_routing_offset();
        return clearance.get_clearance(pt, horizon::PatchType::HOLE_NPTH) + routing_offset;
    }

    if ((parent_a && parent_a->keepout) || (parent_b && parent_b->keepout)) { // one is keepout
        bool a_is_keepout = parent_a && parent_a->keepout;
        auto net = net_a ? net_a : net_b; // only one has net
        horizon::KeepoutContour keepout_contour;
        keepout_contour.keepout = a_is_keepout ? parent_a->keepout : parent_b->keepout;
        keepout_contour.pkg = a_is_keepout ? parent_a->package : parent_b->package;
        for (const auto &rule : m_rules_keepout) {
            if (rule->enabled && rule->match.match(net) && rule->match_keepout.match(&keepout_contour)) {
                auto cl = rule->get_clearance(a_is_keepout ? pt_b : pt_a);
                int64_t routing_offset = rule->routing_offset;
                if (m_iface->get_override_routing_offset() >= 0)
                    routing_offset = m_iface->get_override_routing_offset();
                return routing_offset + cl;
            }
        }
        return 0;
    }

    int layer = UNDEFINED_LAYER;
    if (!layers_a.IsMultilayer() && !layers_b.IsMultilayer()) { // all on single layer
        if (layers_b.Start() != UNDEFINED_LAYER)
            layer = layers_b.Start();
        else
            layer = layers_a.Start();
    }
    else if (!layers_a.IsMultilayer() && layers_b.IsMultilayer()) // b is multi
        layer = layers_a.Start();
    else if (layers_a.IsMultilayer() && !layers_b.IsMultilayer()) // a is muli
        layer = layers_b.Start();
    else if (layers_a.IsMultilayer() && layers_b.IsMultilayer()) // both are multi
        layer = layers_b.Start();                                // fixme, good enough for now

    assert(layer != UNDEFINED_LAYER);

    const auto &clearance = m_rules->get_clearance_copper(net_a, net_b, PNS_HORIZON_IFACE::layer_from_router(layer));

    int64_t routing_offset = clearance.routing_offset;
    if (m_iface->get_override_routing_offset() >= 0)
        routing_offset = m_iface->get_override_routing_offset();

    return clearance.get_clearance(pt_a, pt_b) + routing_offset;
}

int PNS_HORIZON_RULE_RESOLVER::HoleClearance(const PNS::ITEM *aA, const PNS::ITEM *aB)
{
    return 0;
    if (!(aA && aB))
        return 0;
    // std::cout << "HoleClearance " << aA->KindStr() << " " << aB->KindStr() << std::endl;
    // throw std::runtime_error("HoleClearance not implemented");
    return 0;
}

int PNS_HORIZON_RULE_RESOLVER::HoleToHoleClearance(const PNS::ITEM *aA, const PNS::ITEM *aB)
{
    // throw std::runtime_error("HoleToHoleClearance not implemented");
    // good enough for now
    return 0;
}

bool PNS_HORIZON_RULE_RESOLVER::IsDiffPair(const PNS::ITEM *aA, const PNS::ITEM *aB)
{
    // not used anywhere
    throw std::runtime_error("IsDiffPair not implemented");
    return false;
}

bool PNS_HORIZON_RULE_RESOLVER::QueryConstraint(CONSTRAINT_TYPE aType, const PNS::ITEM *aItemA, const PNS::ITEM *aItemB,
                                                int aLayer, PNS::CONSTRAINT *aConstraint)
{
    if (aType == CONSTRAINT_TYPE::CT_CLEARANCE) {
        // only used in  MEANDER_PLACER_BASE::Clearance
        aConstraint->m_Value.SetMin(0);
        return true;
    }
    throw std::runtime_error("QueryConstraint not implemented");
    return false;
}

int PNS_HORIZON_RULE_RESOLVER::DpCoupledNet(int aNet)
{
    auto net = m_iface->get_net_for_code(aNet);
    if (net->diffpair) {
        return m_iface->get_net_code(net->diffpair->uuid);
    }
    return -1;
}

int PNS_HORIZON_RULE_RESOLVER::DpNetPolarity(int aNet)
{
    if (m_iface->get_net_for_code(aNet)->diffpair_primary)
        return 1;
    else
        return -1;
}

bool PNS_HORIZON_RULE_RESOLVER::DpNetPair(const PNS::ITEM *aItem, int &aNetP, int &aNetN)
{
    if (!aItem)
        return false;
    auto net = m_iface->get_net_for_code(aItem->Net());
    if (net->diffpair) {
        if (net->diffpair_primary && net->diffpair) {
            aNetP = m_iface->get_net_code(net->uuid);
            aNetN = m_iface->get_net_code(net->diffpair->uuid);
        }
        else {
            aNetN = m_iface->get_net_code(net->uuid);
            aNetP = m_iface->get_net_code(net->diffpair->uuid);
        }
        return true;
    }
    return false;
}

wxString PNS_HORIZON_RULE_RESOLVER::NetName(int aNet)
{
    auto net = m_iface->get_net_for_code(aNet);
    if (net)
        return net->name;
    else
        return "";
}

PNS_HORIZON_IFACE::PNS_HORIZON_IFACE()
{
}

void PNS_HORIZON_IFACE::SetBoard(horizon::Board *brd)
{
    board = brd;
}

void PNS_HORIZON_IFACE::SetCanvas(horizon::CanvasGL *ca)
{
    canvas = ca;
}

void PNS_HORIZON_IFACE::SetRules(const horizon::BoardRules *ru)
{
    rules = ru;
}

void PNS_HORIZON_IFACE::SetPool(horizon::IPool *p)
{
    pool = p;
}

int PNS_HORIZON_IFACE::get_net_code(const horizon::UUID &uu)
{
    if (net_code_map.count(uu)) {
        return net_code_map.at(uu);
    }
    else {
        net_code_map_r.emplace_back(&board->block->nets.at(uu));
        auto nc = net_code_map_r.size() - 1;
        net_code_map.emplace(uu, nc);
        return nc;
    }
}

horizon::Net *PNS_HORIZON_IFACE::get_net_for_code(int code)
{
    if (code == PNS::ITEM::UnusedNet)
        return nullptr;
    if (code < 0)
        return nullptr;
    if (code >= (int)net_code_map_r.size())
        return nullptr;
    return net_code_map_r.at(code);
}

const PNS_HORIZON_PARENT_ITEM *PNS_HORIZON_IFACE::get_parent(const horizon::Track *track)
{
    return get_or_create_parent(PNS_HORIZON_PARENT_ITEM(track));
}

const PNS_HORIZON_PARENT_ITEM *PNS_HORIZON_IFACE::get_parent(const horizon::BoardHole *hole)
{
    return get_or_create_parent(PNS_HORIZON_PARENT_ITEM(hole));
}

const PNS_HORIZON_PARENT_ITEM *PNS_HORIZON_IFACE::get_parent(const horizon::Via *via)
{
    return get_or_create_parent(PNS_HORIZON_PARENT_ITEM(via));
}

const PNS_HORIZON_PARENT_ITEM *PNS_HORIZON_IFACE::get_parent(const horizon::BoardPackage *pkg, const horizon::Pad *pad)
{
    return get_or_create_parent(PNS_HORIZON_PARENT_ITEM(pkg, pad));
}

const PNS_HORIZON_PARENT_ITEM *PNS_HORIZON_IFACE::get_parent(const horizon::Keepout *k,
                                                             const horizon::BoardPackage *pkg)
{
    return get_or_create_parent(PNS_HORIZON_PARENT_ITEM(k, pkg));
}

const PNS_HORIZON_PARENT_ITEM *PNS_HORIZON_IFACE::get_or_create_parent(const PNS_HORIZON_PARENT_ITEM &it)
{
    auto r = std::find(parents.begin(), parents.end(), it);
    if (r != parents.end())
        return &(*r);
    parents.emplace_back(it);
    return &parents.back();
}


std::unique_ptr<PNS::SEGMENT> PNS_HORIZON_IFACE::syncTrack(const horizon::Track *track)
{
    auto from = track->from.get_position();
    auto to = track->to.get_position();
    int net = PNS::ITEM::UnusedNet;
    if (track->net)
        net = get_net_code(track->net->uuid);
    std::unique_ptr<PNS::SEGMENT> segment(new PNS::SEGMENT(SEG(from.x, from.y, to.x, to.y), net));

    segment->SetWidth(track->width);
    segment->SetLayer(layer_to_router(track->layer));
    segment->SetParent(get_parent(track));

    if (track->locked)
        segment->Mark(PNS::MK_LOCKED);

    return segment;
}

std::unique_ptr<PNS::ARC> PNS_HORIZON_IFACE::syncTrackArc(const horizon::Track *track)
{
    auto from = track->from.get_position();
    auto to = track->to.get_position();
    int net = PNS::ITEM::UnusedNet;
    if (track->net)
        net = get_net_code(track->net->uuid);
    SHAPE_ARC sarc;
    sarc.ConstructFromStartEndCenter(VECTOR2I(from.x, from.y), VECTOR2I(to.x, to.y),
                                     VECTOR2I(track->center->x, track->center->y), false, track->width);
    std::unique_ptr<PNS::ARC> arc(new PNS::ARC(sarc, net));
    arc->SetWidth(track->width);

    arc->SetLayer(layer_to_router(track->layer));
    arc->SetParent(get_parent(track));

    if (track->locked)
        arc->Mark(PNS::MK_LOCKED);

    return arc;
}

void PNS_HORIZON_IFACE::syncOutline(const horizon::Polygon *ipoly, PNS::NODE *aWorld)
{
    auto poly = ipoly->remove_arcs();
    for (size_t i = 0; i < poly.vertices.size(); i++) {
        auto from = poly.vertices[i].position;
        auto to = poly.vertices[(i + 1) % poly.vertices.size()].position;

        std::set<int> layers = {horizon::BoardLayers::TOP_COPPER, horizon::BoardLayers::BOTTOM_COPPER};
        for (unsigned int j = 0; j < board->get_n_inner_layers(); j++) {
            layers.insert(-j - 1);
        }
        for (const auto layer : layers) {
            std::unique_ptr<PNS::SEGMENT> segment(
                    new PNS::SEGMENT(SEG(from.x, from.y, to.x, to.y), PNS::ITEM::UnusedNet));
            segment->SetWidth(10); // very small
            segment->SetLayers(layer_to_router(layer));
            segment->SetParent(&parent_dummy_outline);
            segment->Mark(PNS::MK_LOCKED);
            aWorld->Add(std::move(segment));
        }
    }
}

void PNS_HORIZON_IFACE::syncKeepout(const horizon::KeepoutContour *keepout_contour, PNS::NODE *aWorld)
{
    auto keepout = keepout_contour->keepout;
    if (!horizon::BoardLayers::is_copper(keepout->polygon->layer))
        return;
    if (!(keepout->patch_types_cu.count(horizon::PatchType::TRACK)))
        return;

    std::set<int> layers;
    if (keepout->all_cu_layers) {
        for (unsigned int j = 0; j < board->get_n_inner_layers(); j++) {
            layers.insert(-j - 1);
        }
        layers.insert(horizon::BoardLayers::TOP_COPPER);
        layers.insert(horizon::BoardLayers::BOTTOM_COPPER);
    }
    else {
        layers.insert(keepout->polygon->layer);
    }
    const auto &contour = keepout_contour->contour;
    for (auto layer : layers) {
        for (size_t i = 0; i < contour.size(); i++) {
            auto from = contour[i];
            auto to = contour[(i + 1) % contour.size()];

            std::unique_ptr<PNS::SEGMENT> segment(
                    new PNS::SEGMENT(SEG(from.X, from.Y, to.X, to.Y), PNS::ITEM::UnusedNet));
            segment->SetWidth(10); // very small
            segment->SetLayers(layer_to_router(layer));
            segment->SetParent(get_parent(keepout_contour->keepout, keepout_contour->pkg));
            segment->Mark(PNS::MK_LOCKED);
            aWorld->Add(std::move(segment));
        }
    }
}

std::unique_ptr<PNS::SOLID> PNS_HORIZON_IFACE::syncPad(const horizon::BoardPackage *pkg, const horizon::Pad *pad)
{
    horizon::Placement tr(pkg->placement);
    if (pkg->flip) {
        tr.invert_angle();
    }
    tr.accumulate(pad->placement);
    auto solid = syncPadstack(&pad->padstack, tr);
    if (!solid)
        return nullptr;

    if (pad->net)
        solid->SetNet(get_net_code(pad->net->uuid));
    solid->SetParent(get_parent(pkg, pad));

    return solid;
}

std::unique_ptr<PNS::SOLID> PNS_HORIZON_IFACE::syncHole(const horizon::BoardHole *hole)
{
    auto solid = syncPadstack(&hole->padstack, hole->placement);
    if (!solid)
        return nullptr;

    if (hole->net)
        solid->SetNet(get_net_code(hole->net->uuid));
    solid->SetParent(get_parent(hole));

    return solid;
}

bool shape_is_at_origin(const horizon::Shape &sh)
{
    return sh.placement.shift == horizon::Coordi();
}

bool angle_is_rect(int angle)
{
    return (angle % 16384) == 0; // multiple of 90°
}

bool angle_needs_swap(int angle)
{
    return angle == 16384 || angle == 49152; // 90° or 270°
}

std::unique_ptr<PNS::SOLID> PNS_HORIZON_IFACE::syncPadstack(const horizon::Padstack *padstack,
                                                            const horizon::Placement &tr)
{
    int layer_min = 10000;
    int layer_max = -10000;

    ClipperLib::Clipper clipper;

    if (padstack->type != horizon::Padstack::Type::MECHANICAL) { // normal pad

        auto n_shapes_cu = std::count_if(padstack->shapes.begin(), padstack->shapes.end(), [](const auto &it) {
            return horizon::BoardLayers::is_copper(it.second.layer);
        });

        auto n_polys_cu = std::count_if(padstack->polygons.begin(), padstack->polygons.end(), [](const auto &it) {
            return horizon::BoardLayers::is_copper(it.second.layer);
        });

        if (n_shapes_cu == 1 && n_polys_cu == 0) { // single SMD pad
            const auto &shape = std::find_if(padstack->shapes.begin(), padstack->shapes.end(), [](const auto &it) {
                                    return horizon::BoardLayers::is_copper(it.second.layer);
                                })->second;
            if (shape_is_at_origin(shape)) {
                const int angle = tr.get_angle() + shape.placement.get_angle();
                if (shape.form == horizon::Shape::Form::CIRCLE) {
                    std::unique_ptr<PNS::SOLID> solid(new PNS::SOLID);

                    solid->SetLayers(LAYER_RANGE(layer_to_router(shape.layer)));

                    solid->SetOffset(VECTOR2I(0, 0));
                    solid->SetPos(VECTOR2I(tr.shift.x, tr.shift.y));

                    auto *sshape = new SHAPE_CIRCLE(VECTOR2I(tr.shift.x, tr.shift.y), shape.params.at(0) / 2);

                    solid->SetShape(sshape);

                    return solid;
                }
                else if (shape.form == horizon::Shape::Form::RECTANGLE && angle_is_rect(angle)) {
                    std::unique_ptr<PNS::SOLID> solid(new PNS::SOLID);

                    solid->SetLayers(LAYER_RANGE(layer_to_router(shape.layer)));

                    solid->SetOffset(VECTOR2I(0, 0));
                    const auto c = VECTOR2I(tr.shift.x, tr.shift.y);
                    solid->SetPos(c);
                    auto sz = VECTOR2I(shape.params.at(0), shape.params.at(1));
                    if (angle_needs_swap(angle))
                        std::swap(sz.x, sz.y);

                    auto *sshape = new SHAPE_RECT(c - sz / 2, sz.x, sz.y);

                    solid->SetShape(sshape);

                    return solid;
                }
            }
        }


        auto add_polygon = [&clipper, &tr](const auto &poly) {
            ClipperLib::Path path;
            if (poly.vertices.size() == 0) {
                throw std::runtime_error("polygon without vertices in padstack at"
                                         + horizon::coord_to_string(tr.shift));
            }
            for (auto &v : poly.vertices) {
                auto p = tr.transform(v.position);
                path.emplace_back(p.x, p.y);
            }
            if (ClipperLib::Orientation(path)) {
                std::reverse(path.begin(), path.end());
            }
            clipper.AddPath(path, ClipperLib::ptSubject, true);
        };
        for (auto &it : padstack->shapes) {
            if (horizon::BoardLayers::is_copper(it.second.layer)) { // on copper layer
                layer_min = std::min(layer_min, it.second.layer);
                layer_max = std::max(layer_max, it.second.layer);
                add_polygon(it.second.to_polygon().remove_arcs());
            }
        }
        for (auto &it : padstack->polygons) {
            if (horizon::BoardLayers::is_copper(it.second.layer)) { // on copper layer
                layer_min = std::min(layer_min, it.second.layer);
                layer_max = std::max(layer_max, it.second.layer);
                add_polygon(it.second.remove_arcs());
            }
        }
    }
    else { // npth pad
        layer_min = horizon::BoardLayers::BOTTOM_COPPER;
        layer_max = horizon::BoardLayers::TOP_COPPER;
        for (auto &it : padstack->holes) {
            auto poly = it.second.to_polygon().remove_arcs();
            ClipperLib::Path path;
            for (auto &v : poly.vertices) {
                auto p = tr.transform(v.position);
                path.emplace_back(p.x, p.y);
            }
            if (ClipperLib::Orientation(path)) {
                std::reverse(path.begin(), path.end());
            }
            clipper.AddPath(path, ClipperLib::ptSubject, true);
        }
    }

    ClipperLib::PolyTree poly_tree;
    clipper.Execute(ClipperLib::ctUnion, poly_tree, ClipperLib::pftNonZero);
    if (poly_tree.ChildCount() == 0) // nothing we care about
        return nullptr;
    if (poly_tree.ChildCount() != 1) {
        throw std::runtime_error("invalid pad polygons: " + std::to_string(poly_tree.ChildCount()));
    }

    std::unique_ptr<PNS::SOLID> solid(new PNS::SOLID);

    solid->SetLayers(LAYER_RANGE(layer_to_router(layer_max), layer_to_router(layer_min)));

    solid->SetOffset(VECTOR2I(0, 0));
    solid->SetPos(VECTOR2I(tr.shift.x, tr.shift.y));

    SHAPE_SIMPLE *shape = new SHAPE_SIMPLE();

    for (auto &pt : poly_tree.Childs.front()->Contour) {
        shape->Append(pt.X, pt.Y);
    }
    solid->SetShape(shape);

    return solid;
}

std::unique_ptr<PNS::VIA> PNS_HORIZON_IFACE::syncVia(const horizon::Via *via)
{
    auto pos = via->junction->position;
    int net = PNS::ITEM::UnusedNet;
    if (via->junction->net)
        net = get_net_code(via->junction->net->uuid);
    std::unique_ptr<PNS::VIA> pvia(new PNS::VIA(VECTOR2I(pos.x, pos.y),
                                                LAYER_RANGE(layer_to_router(horizon::BoardLayers::TOP_COPPER),
                                                            layer_to_router(horizon::BoardLayers::BOTTOM_COPPER)),
                                                via->parameter_set.at(horizon::ParameterID::VIA_DIAMETER),
                                                via->parameter_set.at(horizon::ParameterID::HOLE_DIAMETER), net,
                                                VIATYPE::THROUGH));

    // via->SetParent( aVia );
    pvia->SetParent(get_parent(via));

    if (via->locked)
        pvia->Mark(PNS::MK_LOCKED);

    return pvia;
}

void PNS_HORIZON_IFACE::SyncWorld(PNS::NODE *aWorld)
{
    // std::cout << "!!!sync world" << std::endl;
    m_world = aWorld;
    if (!board) {
        wxLogTrace("PNS", "No board attached, aborting sync.");
        return;
    }
    parents.clear();
    junctions_maybe_erased.clear();

    for (const auto &it : board->tracks) {
        if (it.second.is_arc()) {
            auto arc = syncTrackArc(&it.second);
            if (arc) {
                aWorld->Add(std::move(arc));
            }
        }
        else {
            auto segment = syncTrack(&it.second);
            if (segment) {
                aWorld->Add(std::move(segment));
            }
        }
    }

    for (const auto &it : board->vias) {
        auto via = syncVia(&it.second);
        if (via) {
            aWorld->Add(std::move(via));
        }
    }

    for (const auto &it : board->holes) {
        auto hole = syncHole(&it.second);
        if (hole) {
            aWorld->Add(std::move(hole));
        }
    }

    for (const auto &it : board->packages) {
        for (auto &it_pad : it.second.package.pads) {
            auto pad = syncPad(&it.second, &it_pad.second);
            if (pad) {
                aWorld->Add(std::move(pad));
            }
        }
    }
    for (const auto &it : board->polygons) {
        if (it.second.layer == horizon::BoardLayers::L_OUTLINE) {
            syncOutline(&it.second, aWorld);
        }
    }

    auto keepout_contours = board->get_keepout_contours();
    for (const auto &it : keepout_contours) {
        if (horizon::BoardLayers::is_copper(it.keepout->polygon->layer))
            syncKeepout(&it, aWorld);
    }

    int worstClearance = rules->get_max_clearance();

    delete m_ruleResolver;
    m_ruleResolver = new PNS_HORIZON_RULE_RESOLVER(board, rules, this);

    aWorld->SetRuleResolver(m_ruleResolver);
    aWorld->SetMaxClearance(4 * worstClearance);
}

void PNS_HORIZON_IFACE::EraseView()
{
    // std::cout << "iface erase view" << std::endl;

    canvas->show_all_obj();

    for (const auto &it : m_preview_items) {
        canvas->remove_obj(it);
    }
    m_preview_items.clear();
}

void PNS_HORIZON_IFACE::DisplayItem(const PNS::ITEM *aItem, int aClearance, bool aEdit)
{
    if (aItem->IsVirtual())
        return;
    wxLogTrace("PNS", "DisplayItem %p %s", aItem, aItem->KindStr().c_str());
    if (aItem->Kind() == PNS::ITEM::LINE_T) {
        auto line_item = dynamic_cast<const PNS::LINE *>(aItem);
        auto npts = line_item->PointCount();
        std::deque<horizon::Coordi> pts;
        for (int i = 0; i < npts; i++) {
            auto pt = line_item->CPoint(i);
            pts.emplace_back(pt.x, pt.y);
        }
        int la = line_item->Layer();
        m_preview_items.insert(canvas->add_line(pts, line_item->Width(), horizon::ColorP::LAYER_HIGHLIGHT_LIGHTEN,
                                                layer_from_router(la)));
    }
    else if (aItem->Kind() == PNS::ITEM::SEGMENT_T) {
        auto seg_item = dynamic_cast<const PNS::SEGMENT *>(aItem);
        auto seg = seg_item->Seg();
        std::deque<horizon::Coordi> pts;
        pts.emplace_back(seg.A.x, seg.A.y);
        pts.emplace_back(seg.B.x, seg.B.y);
        int la = seg_item->Layer();
        m_preview_items.insert(
                canvas->add_line(pts, seg_item->Width(), horizon::ColorP::LAYER_HIGHLIGHT, layer_from_router(la)));
    }
    else if (aItem->Kind() == PNS::ITEM::VIA_T) {
        auto via_item = dynamic_cast<const PNS::VIA *>(aItem);
        auto pos = via_item->Pos();

        std::deque<horizon::Coordi> pts;
        pts.emplace_back(pos.x, pos.y);
        pts.emplace_back(pos.x + 10, pos.y);
        int la = via_item->Layers().Start();
        m_preview_items.insert(
                canvas->add_line(pts, via_item->Diameter(), horizon::ColorP::LAYER_HIGHLIGHT, layer_from_router(la)));
        la = via_item->Layers().End();
        m_preview_items.insert(
                canvas->add_line(pts, via_item->Diameter(), horizon::ColorP::LAYER_HIGHLIGHT, layer_from_router(la)));
    }
    else if (aItem->Kind() == PNS::ITEM::ARC_T) {
        auto arc_item = static_cast<const PNS::ARC *>(aItem);
        const auto &arc = arc_item->CArc();
        const auto p0 = arc.GetP0();
        const auto p1 = arc.GetP1();
        const auto c = arc.GetCenter();
        const auto center = horizon::Coordi(c.x, c.y);
        auto from = horizon::Coordi(p0.x, p0.y);
        auto to = horizon::Coordi(p1.x, p1.y);
        if (arc.IsClockwise())
            std::swap(from, to);

        int la = arc_item->Layer();
        m_preview_items.insert(canvas->add_arc(from, to, center, arc_item->Width(), horizon::ColorP::LAYER_HIGHLIGHT,
                                               layer_from_router(la)));
    }
    else if (aItem->Kind() == PNS::ITEM::SOLID_T) {
        // it's okay
    }
    else {
        assert(false);
    }
}

void PNS_HORIZON_IFACE::HideItem(PNS::ITEM *aItem)
{
    auto parent = aItem->Parent();
    if (parent) {
        if (parent->track) {
            horizon::ObjectRef ref(horizon::ObjectType::TRACK, parent->track->uuid);
            canvas->hide_obj(ref);
        }
        else if (parent->via) {
            horizon::ObjectRef ref(horizon::ObjectType::VIA, parent->via->uuid);
            canvas->hide_obj(ref);
        }
    }
}

void PNS_HORIZON_IFACE::RemoveItem(PNS::ITEM *aItem)
{

    auto parent = aItem->Parent();
    // std::cout << "!!!iface remove item " << parent << " " << aItem->KindStr() << std::endl;
    if (parent) {
        if (parent->track) {
            for (auto &it_ft : {parent->track->from, parent->track->to}) {
                if (it_ft.junc)
                    junctions_maybe_erased.insert(it_ft.junc);
            }
            board->tracks.erase(parent->track->uuid);
        }
        else if (parent->via) {
            board->vias.erase(parent->via->uuid);
        }
    }
    // board->expand(true);
}

std::pair<horizon::BoardPackage *, horizon::Pad *> PNS_HORIZON_IFACE::find_pad(int layer, const horizon::Coordi &c)
{
    for (auto &it : board->packages) {
        for (auto &it_pad : it.second.package.pads) {
            auto pt = it_pad.second.padstack.type;
            if ((pt == horizon::Padstack::Type::TOP && layer == horizon::BoardLayers::TOP_COPPER)
                || (pt == horizon::Padstack::Type::BOTTOM && layer == horizon::BoardLayers::BOTTOM_COPPER)
                || (pt == horizon::Padstack::Type::THROUGH)) {
                auto tr = it.second.placement;
                if (it.second.flip) {
                    tr.invert_angle();
                }
                auto pos = tr.transform(it_pad.second.placement.shift);
                if (pos == c) {
                    return {&it.second, &it_pad.second};
                }
            }
        }
    }
    return {nullptr, nullptr};
}

horizon::BoardJunction *PNS_HORIZON_IFACE::find_junction(int layer, const horizon::Coordi &c)
{
    for (auto &it : board->junctions) {
        if ((layer == 10000 || it.second.layer == layer || it.second.has_via) && it.second.position == c) {
            return &it.second;
        }
    }
    return nullptr;
}

std::set<horizon::BoardJunction *> PNS_HORIZON_IFACE::find_junctions(const horizon::Coordi &c)
{
    std::set<horizon::BoardJunction *> junctions;
    const auto cu_layers = horizon::LayerRange(horizon::BoardLayers::TOP_COPPER, horizon::BoardLayers::BOTTOM_COPPER);
    for (auto &[uu, ju] : board->junctions) {
        if (ju.net && ju.position == c && ju.layer.overlaps(cu_layers)) {
            junctions.insert(&ju);
        }
    }
    return junctions;
}

void PNS_HORIZON_IFACE::AddItem(PNS::ITEM *aItem)
{
    // std::cout << "!!!iface add item" << std::endl;
    switch (aItem->Kind()) {
    case PNS::ITEM::SEGMENT_T:
    case PNS::ITEM::ARC_T: {
        auto uu = horizon::UUID::random();
        auto track = &board->tracks.emplace(uu, uu).first->second;
        horizon::Coordi from;
        horizon::Coordi to;
        int layer;
        if (aItem->Kind() == PNS::ITEM::SEGMENT_T) {
            PNS::SEGMENT *seg = static_cast<PNS::SEGMENT *>(aItem);
            const SEG &s = seg->Seg();
            from = horizon::Coordi(s.A.x, s.A.y);
            to = horizon::Coordi(s.B.x, s.B.y);
            track->width = seg->Width();
            track->net = get_net_for_code(seg->Net());
            layer = layer_from_router(seg->Layer());
        }
        else {
            auto arc_item = static_cast<const PNS::ARC *>(aItem);
            const auto &arc = arc_item->CArc();
            const auto p0 = arc.GetP0();
            const auto p1 = arc.GetP1();
            const auto c = arc.GetCenter();
            from = horizon::Coordi(p0.x, p0.y);
            to = horizon::Coordi(p1.x, p1.y);
            if (arc.IsClockwise())
                std::swap(from, to);
            track->center = horizon::Coordi(c.x, c.y);
            track->width = arc.GetWidth();
            track->net = get_net_for_code(arc_item->Net());
            layer = layer_from_router(arc_item->Layer());
        }

        track->layer = layer;

        auto connect = [this, &layer, track](horizon::Track::Connection &conn, const horizon::Coordi &c) {
            auto p = find_pad(layer, c);
            if (p.first) {
                conn.connect(p.first, p.second);
            }
            else {
                auto j = find_junction(layer, c);
                if (j) {
                    conn.connect(j);
                }
                else {
                    auto juu = horizon::UUID::random();
                    auto ju = &board->junctions.emplace(juu, juu).first->second;
                    ju->layer = layer;
                    ju->position = c;
                    ju->net = track->net;
                    conn.connect(ju);
                }
                conn.junc->connected_tracks.push_back(track->uuid);
            }
        };
        connect(track->from, from);
        connect(track->to, to);
        track->width_from_rules = !m_router->Sizes().TrackWidthIsExplicit();
        aItem->SetParent(get_parent(track));

    } break;

    case PNS::ITEM::VIA_T: {
        PNS::VIA *pvia = static_cast<PNS::VIA *>(aItem);
        auto uu = horizon::UUID::random();
        auto net = get_net_for_code(pvia->Net());
        auto padstack = pool->get_padstack(rules->get_via_padstack_uuid(net));
        if (padstack) {
            auto ps = rules->get_via_parameter_set(net);
            auto via = &board->vias
                                .emplace(std::piecewise_construct, std::forward_as_tuple(uu),
                                         std::forward_as_tuple(uu, padstack))
                                .first->second;
            via->parameter_set = ps;

            horizon::Coordi c(pvia->Pos().x, pvia->Pos().y);
            auto junctions = find_junctions(c);
            if (junctions.size()) {
                auto new_junc = *junctions.begin();
                via->junction = new_junc;
                for (auto ju : junctions) {
                    if (ju == new_junc)
                        continue;
                    junctions_maybe_erased.insert(ju);
                    for (auto &track_uu : ju->connected_tracks) {
                        if (board->tracks.count(track_uu)) {
                            auto &track = board->tracks.at(track_uu);
                            for (auto &it_ft : {&track.from, &track.to}) {
                                if (it_ft->junc == ju) {
                                    it_ft->connect(new_junc);
                                }
                            }
                        }
                    }
                }
            }
            else {
                auto juu = horizon::UUID::random();
                auto ju = &board->junctions.emplace(juu, juu).first->second;
                ju->position = c;
                ju->net = net;
                via->junction = ju;
            }
            via->junction->has_via = true;
            via->expand(*board);
            aItem->SetParent(get_parent(via));
        }
    } break;

    default:;
        // std::cout << "!!!unhandled add " << aItem->KindStr() << std::endl;
    }
}

void PNS_HORIZON_IFACE::Commit()
{
    board->update_junction_connections();
    for (auto ju : junctions_maybe_erased) {
        if (!(ju->connected_arcs.size() || ju->connected_lines.size() || ju->connected_vias.size()
              || ju->connected_tracks.size())) {
            for (auto &it : ju->connected_connection_lines)
                board->connection_lines.erase(it);
            for (auto &[net, airwires] : board->airwires) {
                airwires.remove_if([ju](const auto &aw) { return aw.from.junc == ju || aw.to.junc == ju; });
            }
            board->junctions.erase(ju->uuid);
        }
    }
    junctions_maybe_erased.clear();
    EraseView();
}

void PNS_HORIZON_IFACE::UpdateNet(int aNetCode)
{
    if (const auto net = get_net_for_code(aNetCode)) {
        board->update_airwires(false, {net->uuid});
    }
    wxLogTrace("PNS", "Update-net %d", aNetCode);
}

PNS::RULE_RESOLVER *PNS_HORIZON_IFACE::GetRuleResolver()
{
    return m_ruleResolver;
}

PNS::DEBUG_DECORATOR *PNS_HORIZON_IFACE::GetDebugDecorator()
{
    return nullptr;
}

PNS_HORIZON_IFACE::~PNS_HORIZON_IFACE()
{
    delete m_ruleResolver;
}

void PNS_HORIZON_IFACE::SetRouter(PNS::ROUTER *aRouter)
{
    m_router = aRouter;
}

bool PNS_HORIZON_IFACE::IsAnyLayerVisible(const LAYER_RANGE &aLayer) const
{
    throw std::runtime_error("IsAnyLayerVisible not implemented");
    return true;
}

bool PNS_HORIZON_IFACE::IsItemVisible(const PNS::ITEM *aItem) const
{
    throw std::runtime_error("IsItemVisible not implemented");
    return true;
}


void PNS_HORIZON_IFACE::UpdateItem(ITEM *aItem)
{
    switch (aItem->Kind()) {
    case PNS::ITEM::VIA_T: {
        PNS::VIA *pvia = static_cast<PNS::VIA *>(aItem);
        auto via = aItem->Parent()->via;
        assert(via);
        via->junction->position.x = pvia->Pos().x;
        via->junction->position.y = pvia->Pos().y;
        // check if the via's position could merge junctions
        auto junctions = find_junctions(via->junction->position);
        for (auto ju : junctions) {
            if (ju == via->junction)
                continue;
            if (ju->net != via->junction->net)
                continue;
            junctions_maybe_erased.insert(ju);
            for (auto &track_uu : ju->connected_tracks) {
                if (board->tracks.count(track_uu)) {
                    auto &track = board->tracks.at(track_uu);
                    for (auto &it_ft : {&track.from, &track.to}) {
                        if (it_ft->junc == ju) {
                            it_ft->connect(via->junction);
                        }
                    }
                }
            }
        }

    } break;
    default:
        throw std::runtime_error("UpdateItem not implemented for " + aItem->KindStr());
    }
}

bool PNS_HORIZON_IFACE::IsFlashedOnLayer(const PNS::ITEM *aItem, int aLayer) const
{
    return true;
}

bool PNS_HORIZON_IFACE::ImportSizes(SIZES_SETTINGS &aSizes, ITEM *aStartItem, int aNet)
{
    return true;
}

int PNS_HORIZON_IFACE::StackupHeight(int aFirstLayer, int aSecondLayer) const
{
    return 0;
}

void PNS_HORIZON_IFACE::DisplayRatline(const SHAPE_LINE_CHAIN &aRatline, int aColor)
{
    auto npts = aRatline.PointCount();
    std::deque<horizon::Coordi> pts;
    for (int i = 0; i < npts; i++) {
        auto pt = aRatline.CPoint(i);
        pts.emplace_back(pt.x, pt.y);
    }
    m_preview_items.insert(canvas->add_line(pts, 0, horizon::ColorP::AIRWIRE_ROUTER, 10000));
}

} // namespace PNS
