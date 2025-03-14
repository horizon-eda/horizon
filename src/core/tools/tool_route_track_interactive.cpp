#include "tool_route_track_interactive.hpp"
#include "board/board_layers.hpp"
#include "board/board_rules.hpp"
#include "canvas/canvas_gl.hpp"
#include "document/idocument_board.hpp"
#include "board/board.hpp"
#include "imp/imp_interface.hpp"
#include "router/pns_horizon_iface.hpp"
#include "router/pns_solid.h"
#include "router/pns_arc.h"
#include "router/router/pns_router.h"
#include "router/router/pns_meander_placer_base.h"
#include "util/util.hpp"
#include "util/geom_util.hpp"
#include "core/tool_id.hpp"
#include "util/selection_util.hpp"
#include "dialogs/router_settings_window.hpp"
#include <iostream>
#include <iomanip>
#include "nlohmann/json.hpp"
#include "router/layer_ids.h"

namespace horizon {

class ToolWrapper {
public:
    ToolWrapper(ToolRouteTrackInteractive *t) : tool(t)
    {
    }

    void updateStartItem(const ToolArgs &args);
    void updateEndItem(const ToolArgs &args);
    PNS::ITEM *pickSingleItem(const VECTOR2I &aWhere, int aNet = -1, int aLayer = -1);
    const VECTOR2I snapToItem(bool aEnabled, PNS::ITEM *aItem, VECTOR2I aP);
    bool prepareInteractive();
    int getStartLayer();
    PNS::ITEM *m_startItem = nullptr;
    PNS::ITEM *m_endItem = nullptr;
    VECTOR2I m_startSnapPoint;
    VECTOR2I m_endSnapPoint;
    int work_layer = 0;
    PNS::ROUTING_SETTINGS settings;

    ToolRouteTrackInteractive *tool = nullptr;
};

const std::map<ToolRouteTrackInteractive::Settings::Mode, std::string> ToolRouteTrackInteractive::Settings::mode_names =
        {{ToolRouteTrackInteractive::Settings::Mode::BEND, "Bent"},
         {ToolRouteTrackInteractive::Settings::Mode::STRAIGHT, "Straight"},
         {ToolRouteTrackInteractive::Settings::Mode::PUSH, "Push & shove"},
         {ToolRouteTrackInteractive::Settings::Mode::WALKAROUND, "Walkaround"}};

static const LutEnumStr<ToolRouteTrackInteractive::Settings::Mode> mode_lut = {
        {"bend", ToolRouteTrackInteractive::Settings::Mode::BEND},
        {"straight", ToolRouteTrackInteractive::Settings::Mode::STRAIGHT},
        {"push", ToolRouteTrackInteractive::Settings::Mode::PUSH},
        {"walkaround", ToolRouteTrackInteractive::Settings::Mode::WALKAROUND},
};


const std::map<ToolRouteTrackInteractive::Settings::CornerMode, std::string>
        ToolRouteTrackInteractive::Settings::corner_mode_names = {
                {ToolRouteTrackInteractive::Settings::CornerMode::MITERED_45, "45°"},
                {ToolRouteTrackInteractive::Settings::CornerMode::ROUNDED_45, "45° rounded"},
                {ToolRouteTrackInteractive::Settings::CornerMode::MITERED_90, "90°"},
                {ToolRouteTrackInteractive::Settings::CornerMode::ROUNDED_90, "90° rounded"},
};

static const LutEnumStr<ToolRouteTrackInteractive::Settings::CornerMode> corner_mode_lut = {
        {"mitered_45", ToolRouteTrackInteractive::Settings::CornerMode::MITERED_45},
        {"mitered_90", ToolRouteTrackInteractive::Settings::CornerMode::MITERED_90},
        {"rounded_45", ToolRouteTrackInteractive::Settings::CornerMode::ROUNDED_45},
        {"rounded_90", ToolRouteTrackInteractive::Settings::CornerMode::ROUNDED_90},
};


json ToolRouteTrackInteractive::Settings::serialize() const
{
    json j;
    j["effort"] = effort;
    j["remove_loops"] = remove_loops;
    j["fix_all_segments"] = fix_all_segments;
    j["drc"] = drc;
    j["mode"] = mode_lut.lookup_reverse(mode);
    j["corner_mode"] = corner_mode_lut.lookup_reverse(corner_mode);
    return j;
}

void ToolRouteTrackInteractive::Settings::load_from_json(const json &j)
{
    effort = j.value("effort", 1);
    remove_loops = j.value("remove_loops", true);
    fix_all_segments = j.value("fix_all_segments", false);
    drc = j.value("drc", true);
    mode = mode_lut.lookup(j.value("mode", ""), Settings::Mode::WALKAROUND);
    corner_mode = corner_mode_lut.lookup(j.value("corner_mode", ""), CornerMode::MITERED_45);
}

void ToolRouteTrackInteractive::apply_settings()
{
    if (!router)
        return;
    wrapper->settings.SetRemoveLoops(settings.remove_loops);
    wrapper->settings.SetOptimizerEffort(static_cast<PNS::PNS_OPTIMIZATION_EFFORT>(settings.effort));
    wrapper->settings.SetAllowDRCViolations(!settings.drc);
    wrapper->settings.SetFixAllSegments(settings.fix_all_segments);
    DIRECTION_45::CORNER_MODE corner_mode = DIRECTION_45::CORNER_MODE::MITERED_45;
#define X_MODE(m_)                                                                                                     \
    case Settings::CornerMode::m_:                                                                                     \
        corner_mode = DIRECTION_45::CORNER_MODE::m_;                                                                   \
        break;
    switch (settings.corner_mode) {
        X_MODE(MITERED_45)
        X_MODE(MITERED_90)
        X_MODE(ROUNDED_45)
        X_MODE(ROUNDED_90)
    }
#undef X_MODE
    wrapper->settings.SetCornerMode(corner_mode);
}

bool ToolRouteTrackInteractive::is_tune() const
{
    switch (tool_id) {
    case ToolID::TUNE_TRACK:
    case ToolID::TUNE_DIFFPAIR:
    case ToolID::TUNE_DIFFPAIR_SKEW:
        return true;
    default:
        return false;
    }
}

bool ToolRouteTrackInteractive::can_begin()
{
    if (!doc.b)
        return false;
    auto via = get_via(selection);
    auto track = get_track(selection);
    switch (tool_id) {
    case ToolID::DRAG_TRACK_INTERACTIVE:
        if (via && track)
            return false;
        else if (via)
            return via->junction->net;
        else if (track)
            return !track->is_arc() && track->net;
        else
            return false;

    case ToolID::TUNE_TRACK:
        return track && track->net && !track->is_arc();

    case ToolID::TUNE_DIFFPAIR:
    case ToolID::TUNE_DIFFPAIR_SKEW:
        return track && track->net && track->net->diffpair;

    default:
        return true;
    }
}

ToolRouteTrackInteractive::~ToolRouteTrackInteractive()
{
    delete router;
    delete iface;
    delete wrapper;
}

bool ToolRouteTrackInteractive::is_specific()
{
    return tool_id == ToolID::DRAG_TRACK_INTERACTIVE || is_tune();
}

Track *ToolRouteTrackInteractive::get_track(const std::set<SelectableRef> &sel)
{
    if (sel_count_type(sel, ObjectType::TRACK) == 1)
        return &doc.b->get_board()->tracks.at(sel_find_one(sel, ObjectType::TRACK).uuid);
    else
        return nullptr;
}

Via *ToolRouteTrackInteractive::get_via(const std::set<SelectableRef> &sel)
{
    if (sel_count_type(sel, ObjectType::VIA) == 1)
        return &doc.b->get_board()->vias.at(sel_find_one(sel, ObjectType::VIA).uuid);
    else
        return nullptr;
}

ToolResponse ToolRouteTrackInteractive::begin(const ToolArgs &args)
{
    std::cout << "tool route track\n";
    selection.clear();
    canvas = imp->get_canvas();
    canvas->set_cursor_external(true);
    imp->set_no_update(true);

    wrapper = new ToolWrapper(this);

    board = doc.b->get_board();
    rules = dynamic_cast<BoardRules *>(doc.r->get_rules());

    iface = new PNS::PNS_HORIZON_IFACE;
    iface->SetBoard(board);
    iface->SetCanvas(imp->get_canvas());
    iface->SetRules(rules);
    iface->SetPool(&doc.b->get_pool_caching());
    // m_iface->SetHostFrame( m_frame );

    router = new PNS::ROUTER;
    router->SetInterface(iface);
    iface->SetRouter(router);
    switch (tool_id) {
    case ToolID::ROUTE_DIFFPAIR_INTERACTIVE:
        router->SetMode(PNS::ROUTER_MODE::PNS_MODE_ROUTE_DIFF_PAIR);
        break;

    case ToolID::TUNE_TRACK:
        router->SetMode(PNS::ROUTER_MODE::PNS_MODE_TUNE_SINGLE);
        break;

    case ToolID::TUNE_DIFFPAIR:
        router->SetMode(PNS::ROUTER_MODE::PNS_MODE_TUNE_DIFF_PAIR);
        break;

    case ToolID::TUNE_DIFFPAIR_SKEW:
        router->SetMode(PNS::ROUTER_MODE::PNS_MODE_TUNE_DIFF_PAIR_SKEW);
        break;

    default:
        router->SetMode(PNS::ROUTER_MODE::PNS_MODE_ROUTE_SINGLE);
    }

    router->ClearWorld();
    router->SyncWorld();

    wrapper->settings.SetShoveVias(false);

    PNS::SIZES_SETTINGS sizes_settings;

    router->LoadSettings(&wrapper->settings);
    router->UpdateSizes(sizes_settings);
    apply_settings();

    imp->canvas_update();
    update_tip();

    if (tool_id == ToolID::DRAG_TRACK_INTERACTIVE) {
        auto track = get_track(args.selection);
        auto via = get_via(args.selection);
        const PNS::PNS_HORIZON_PARENT_ITEM *parent = nullptr;
        Net *net = nullptr;
        if (track) {
            parent = iface->get_parent(track);
            net = track->net;
        }
        else if (via) {
            parent = iface->get_parent(via);
            net = via->junction->net;
        }
        else
            return ToolResponse::end();

        auto &highlights = imp->get_highlights();
        highlights.clear();
        highlights.emplace(ObjectType::NET, net->uuid);
        imp->update_highlights();

        wrapper->m_startItem = router->GetWorld()->FindItemByParent(parent, iface->get_net_code(net->uuid));

        wrapper->settings.SetMode(PNS::RM_Shove);
        router->LoadSettings(&wrapper->settings);


        VECTOR2I p0(args.coords.x, args.coords.y);
        if (!router->StartDragging(p0, wrapper->m_startItem, PNS::DM_ANY))
            return ToolResponse::end();
    }
    if (is_tune()) {
        Track *track = get_track(args.selection);
        if (!track) {
            return ToolResponse::end();
        }
        auto parent = iface->get_parent(track);
        wrapper->m_startItem = router->GetWorld()->FindItemByParent(parent, iface->get_net_code(track->net.uuid));
        VECTOR2I p0(args.coords.x, args.coords.y);
        if (!router->StartRouting(p0, wrapper->m_startItem, 0))
            return ToolResponse::end();
        meander_placer = dynamic_cast<PNS::MEANDER_PLACER_BASE *>(router->Placer());
        if (auto ref = imp->get_length_tuning_ref()) {
            PNS::MEANDER_SETTINGS meander_settings = meander_placer->MeanderSettings();
            meander_settings.m_targetLength = ref;
            meander_placer->UpdateSettings(meander_settings);
        }
        router->Move(p0, NULL);
    }
    update_tip();
    return ToolResponse();
}

PNS::ITEM *ToolWrapper::pickSingleItem(const VECTOR2I &aWhere, int aNet, int aLayer)
{
    int tl = PNS::PNS_HORIZON_IFACE::layer_to_router(tool->imp->get_work_layer());

    if (aLayer > 0)
        tl = aLayer;
    static const int candidateCount = 5;
    PNS::ITEM *prioritized[candidateCount];
    SEG::ecoord dist[candidateCount];

    for (int i = 0; i < candidateCount; i++) {
        prioritized[i] = nullptr;
        dist[i] = VECTOR2I::ECOORD_MAX;
    }


    PNS::ITEM_SET candidates = tool->router->QueryHoverItems(aWhere);
    if (candidates.Empty())
        candidates = tool->router->QueryHoverItems(aWhere, true);


    for (PNS::ITEM *item : candidates.Items()) {
        // if( !IsCopperLayer( item->Layers().Start() ) )
        //	continue;

        // fixme: this causes flicker with live loop removal...
        // if( item->Parent() && !item->Parent()->ViewIsVisible() )
        //    continue;
        if (!item->Layers().IsMultilayer()) {
            auto la = PNS::PNS_HORIZON_IFACE::layer_from_router(item->Layers().Start());
            if (!tool->canvas->layer_is_visible(la))
                continue;
        }

        if (aNet <= 0 || item->Net() == aNet) {
            if (item->OfKind(PNS::ITEM::VIA_T | PNS::ITEM::SOLID_T)) {
                // if (item->OfKind(PNS::ITEM::SOLID_T) && aIgnorePads)
                //     continue;

                SEG::ecoord d = (item->Shape()->Centre() - aWhere).SquaredEuclideanNorm();

                if (d < dist[2]) {
                    prioritized[2] = item;
                    dist[2] = d;
                }

                if (item->Layers().Overlaps(tl) && d < dist[0]) {
                    prioritized[0] = item;
                    dist[0] = d;
                }
            }
            else // ITEM::SEGMENT_T | ITEM::ARC_T
            {
                PNS::LINKED_ITEM *li = static_cast<PNS::LINKED_ITEM *>(item);
                SEG::ecoord d = std::min((li->Anchor(0) - aWhere).SquaredEuclideanNorm(),
                                         (li->Anchor(1) - aWhere).SquaredEuclideanNorm());

                if (d < dist[3]) {
                    prioritized[3] = item;
                    dist[3] = d;
                }

                if (item->Layers().Overlaps(tl) && d < dist[1]) {
                    prioritized[1] = item;
                    dist[1] = d;
                }
            }
        }
    }

    PNS::ITEM *rv = NULL;
    // PCB_EDIT_FRAME* frame = getEditFrame<PCB_EDIT_FRAME>();
    // DISPLAY_OPTIONS* displ_opts =
    // (DISPLAY_OPTIONS*)frame->GetDisplayOptions();

    for (int i = 0; i < 4; i++) {
        PNS::ITEM *item = prioritized[i];
        if (item) {
            rv = item;
            break;
        }
    }

    if (rv && aLayer >= 0 && !rv->Layers().Overlaps(aLayer))
        rv = NULL;

    if (rv) {
        wxLogTrace("PNS", "%s, layer : %d, tl: %d", rv->KindStr().c_str(), rv->Layers().Start(), tl);
    }

    return rv;
}


static VECTOR2I AlignToSegment(const VECTOR2I &aPoint, const SEG &aSeg)
{
    OPT_VECTOR2I pts[6];

    const int c_gridSnapEpsilon = 2;


    VECTOR2I nearest = aPoint;

    SEG pos_slope(nearest + VECTOR2I(-1, 1), nearest + VECTOR2I(1, -1));
    SEG neg_slope(nearest + VECTOR2I(-1, -1), nearest + VECTOR2I(1, 1));
    int max_i = 2;

    pts[0] = aSeg.A;
    pts[1] = aSeg.B;

    if (!aSeg.ApproxParallel(pos_slope))
        pts[max_i++] = aSeg.IntersectLines(pos_slope);

    if (!aSeg.ApproxParallel(neg_slope))
        pts[max_i++] = aSeg.IntersectLines(neg_slope);

    int min_d = std::numeric_limits<int>::max();

    for (int i = 0; i < max_i; i++) {
        if (pts[i] && aSeg.Distance(*pts[i]) <= c_gridSnapEpsilon) {
            int d = (*pts[i] - aPoint).EuclideanNorm();

            if (d < min_d) {
                min_d = d;
                nearest = *pts[i];
            }
        }
    }

    return nearest;
}


static VECTOR2I AlignToArc(const VECTOR2I &aPoint, const SHAPE_ARC &aArc)
{
    VECTOR2I nearest = aPoint;

    int min_d = std::numeric_limits<int>::max();

    for (auto pt : {aArc.GetP0(), aArc.GetP1()}) {
        int d = (pt - aPoint).EuclideanNorm();

        if (d < min_d) {
            min_d = d;
            nearest = pt;
        }
        else
            break;
    }

    return nearest;
}

const VECTOR2I ToolWrapper::snapToItem(bool aEnabled, PNS::ITEM *aItem, VECTOR2I aP)
{
    VECTOR2I anchor;
    auto snapped = tool->canvas->snap_to_grid(Coordi(aP.x, aP.y), tool->canvas->get_last_grid_div());
    auto snapped_v = VECTOR2I(snapped.x, snapped.y);

    if (!aItem || !aEnabled) {
        return snapped_v;
    }

    switch (aItem->Kind()) {
    case PNS::ITEM::SOLID_T:
        anchor = static_cast<PNS::SOLID *>(aItem)->Pos();
        break;

    case PNS::ITEM::VIA_T:
        anchor = static_cast<PNS::VIA *>(aItem)->Pos();
        break;

    case PNS::ITEM::SEGMENT_T:
    case PNS::ITEM::ARC_T: {
        PNS::LINKED_ITEM *li = static_cast<PNS::LINKED_ITEM *>(aItem);
        VECTOR2I A = li->Anchor(0);
        VECTOR2I B = li->Anchor(1);
        SEG::ecoord w_sq = SEG::Square(li->Width() / 2);
        SEG::ecoord distA_sq = (aP - A).SquaredEuclideanNorm();
        SEG::ecoord distB_sq = (aP - B).SquaredEuclideanNorm();

        if (distA_sq < w_sq || distB_sq < w_sq) {
            return (distA_sq < distB_sq) ? A : B;
        }
        else if (aItem->Kind() == PNS::ITEM::SEGMENT_T) {
            // TODO(snh): Clean this up
            PNS::SEGMENT *seg = static_cast<PNS::SEGMENT *>(li);
            return AlignToSegment(snapped_v, seg->Seg());
        }
        else if (aItem->Kind() == PNS::ITEM::ARC_T) {
            PNS::ARC *arc = static_cast<PNS::ARC *>(li);
            return AlignToArc(snapped_v, *static_cast<const SHAPE_ARC *>(arc->Shape()));
        }

        break;
    }

    default:
        break;
    }

    return anchor;
}

void ToolWrapper::updateStartItem(const ToolArgs &args)
{
    int tl = PNS::PNS_HORIZON_IFACE::layer_to_router(args.work_layer);
    work_layer = tl;
    VECTOR2I cp(args.coords.x, args.coords.y);
    VECTOR2I p;

    bool snapEnabled = true;

    /*if( aEvent.IsMotion() || aEvent.IsClick() )
    {
            snapEnabled = !aEvent.Modifier( MD_SHIFT );
            p = aEvent.Position();
    }
    else
    {
            p = cp;
    }*/
    p = cp;

    m_startItem = pickSingleItem(p);

    if (!snapEnabled && m_startItem && !m_startItem->Layers().Overlaps(tl))
        m_startItem = nullptr;

    m_startSnapPoint = snapToItem(snapEnabled, m_startItem, p);
    tool->canvas->set_cursor_pos(Coordi(m_startSnapPoint.x, m_startSnapPoint.y));
}

int ToolWrapper::getStartLayer()
{
    int wl = tool->imp->get_canvas()->property_work_layer();
    int tl = PNS::PNS_HORIZON_IFACE::layer_to_router(wl);

    if (m_startItem) {
        const LAYER_RANGE &ls = m_startItem->Layers();

        if (ls.Overlaps(tl))
            return tl;
        else
            return ls.Start();
    }

    return tl;
}

const PNS::PNS_HORIZON_PARENT_ITEM *inheritTrackWidth(PNS::ITEM *aItem)
{
    using namespace PNS;
    VECTOR2I p;

    assert(aItem->Owner() != NULL);

    switch (aItem->Kind()) {
    case ITEM::VIA_T:
        p = static_cast<PNS::VIA *>(aItem)->Pos();
        break;

    case ITEM::SOLID_T:
        p = static_cast<SOLID *>(aItem)->Pos();
        break;

    case ITEM::SEGMENT_T:
        return static_cast<SEGMENT *>(aItem)->Parent();

    case ITEM::ARC_T:
        return static_cast<ARC *>(aItem)->Parent();

    default:
        return 0;
    }

    JOINT *jt = static_cast<NODE *>(aItem->Owner())->FindJoint(p, aItem);

    assert(jt != NULL);

    int mval = INT_MAX;

    ITEM_SET linkedSegs = jt->Links();
    linkedSegs.ExcludeItem(aItem).FilterKinds(ITEM::SEGMENT_T);

    const PNS::PNS_HORIZON_PARENT_ITEM *parent = 0;

    for (ITEM *item : linkedSegs.Items()) {
        int w = static_cast<SEGMENT *>(item)->Width();
        if (w < mval) {
            parent = item->Parent();
            mval = w;
        }
        mval = std::min(w, mval);
    }

    return (mval == INT_MAX ? 0 : parent);
}

bool ToolWrapper::prepareInteractive()
{
    int routingLayer = getStartLayer();

    if (!IsCopperLayer(routingLayer)) {
        tool->imp->tool_bar_flash("Tracks on Copper layers only");
        return false;
    }

    tool->imp->set_work_layer(PNS::PNS_HORIZON_IFACE::layer_from_router(routingLayer));

    PNS::SIZES_SETTINGS sizes(tool->router->Sizes());

    int64_t track_width = 0;

    if (m_startItem) {
        auto parent = inheritTrackWidth(m_startItem);
        if (parent && parent->track) {
            auto track = parent->track;
            sizes.SetTrackWidthIsExplicit(!track->width_from_rules);
            track_width = track->width;
        }
    }

    if (!track_width) {
        if (m_startItem) {
            auto netcode = m_startItem->Net();
            auto net = tool->iface->get_net_for_code(netcode);
            track_width =
                    tool->rules->get_default_track_width(net, PNS::PNS_HORIZON_IFACE::layer_from_router(routingLayer));
            sizes.SetTrackWidthIsExplicit(false);
        }
    }

    sizes.SetTrackWidth(track_width);
    if (tool->tool_id == ToolID::ROUTE_DIFFPAIR_INTERACTIVE && m_startItem) {
        auto netcode = m_startItem->Net();
        auto *net = tool->iface->get_net_for_code(netcode);

        if (!net) {
            tool->imp->tool_bar_flash("No net");
            return false;
        }

        const auto &rules_dp =
                tool->rules->get_diffpair(net->net_class, PNS::PNS_HORIZON_IFACE::layer_from_router(routingLayer));
        sizes.SetDiffPairGap(rules_dp.track_gap);
        sizes.SetDiffPairWidth(rules_dp.track_width);
        sizes.SetDiffPairViaGap(rules_dp.via_gap);
        sizes.SetDiffPairViaGapSameAsTraceGap(false);
        sizes.SetTrackWidthIsExplicit(true);
    }

    if (m_startItem) {
        auto netcode = m_startItem->Net();
        auto net = tool->iface->get_net_for_code(netcode);
        if (net) {
            auto &highlights = tool->imp->get_highlights();
            highlights.clear();
            highlights.emplace(ObjectType::NET, net->uuid);
            if (tool->tool_id == ToolID::ROUTE_DIFFPAIR_INTERACTIVE) {
                if (net->diffpair)
                    highlights.emplace(ObjectType::NET, net->diffpair->uuid);
            }
            tool->imp->update_highlights();
        }
    }

    /*sizes.Init( m_board, m_startItem );
    sizes.AddLayerPair( m_frame->GetScreen()->m_Route_Layer_TOP,
                                            m_frame->GetScreen()->m_Route_Layer_BOTTOM
    );
    */
    tool->router->UpdateSizes(sizes);
    using Mode = ToolRouteTrackInteractive::Settings::Mode;
    switch (tool->settings.mode) {
    case Mode::WALKAROUND:
        settings.SetMode(PNS::RM_Walkaround);
        settings.SetFreeAngleMode(false);
        settings.SetAllowDRCViolations(false);
        break;

    case Mode::PUSH:
        settings.SetMode(PNS::RM_Shove);
        settings.SetFreeAngleMode(false);
        settings.SetAllowDRCViolations(false);
        break;

    case Mode::BEND:
        settings.SetMode(PNS::RM_MarkObstacles);
        settings.SetFreeAngleMode(false);
        settings.SetAllowDRCViolations(!tool->settings.drc);
        break;

    case Mode::STRAIGHT:
        settings.SetMode(PNS::RM_MarkObstacles);
        settings.SetFreeAngleMode(true);
        settings.SetAllowDRCViolations(!tool->settings.drc);
        break;
    }

    if (!tool->router->StartRouting(m_startSnapPoint, m_startItem, routingLayer)) {
        std::cout << "error " << tool->router->FailureReason() << std::endl;
        return false;
    }

    m_endItem = NULL;
    m_endSnapPoint = m_startSnapPoint;

    return true;
}

void ToolWrapper::updateEndItem(const ToolArgs &args)
{
    VECTOR2I p(args.coords.x, args.coords.y);
    int layer;

    bool snapEnabled = true;
    /*
    if( m_router->GetCurrentNets().empty() || m_router->GetCurrentNets().front()
    < 0 )
    {
            m_endSnapPoint = snapToItem( snapEnabled, nullptr, p );
            m_ctls->ForceCursorPosition( true, m_endSnapPoint );
            m_endItem = nullptr;

            return;
    }*/

    if (tool->router->IsPlacingVia())
        layer = -1;
    else
        layer = tool->router->GetCurrentLayer();

    PNS::ITEM *endItem = nullptr;

    std::vector<int> nets = tool->router->GetCurrentNets();

    for (int net : nets) {
        endItem = pickSingleItem(p, net, layer);

        if (endItem)
            break;
    }

    VECTOR2I cursorPos = snapToItem(snapEnabled, endItem, p);
    tool->canvas->set_cursor_pos({cursorPos.x, cursorPos.y});
    m_endItem = endItem;
    m_endSnapPoint = cursorPos;

    if (m_endItem) {
        wxLogTrace("PNS", "%s, layer : %d", m_endItem->KindStr().c_str(), m_endItem->Layers().Start());
    }
}

static bool keepout_hit(const Keepout &keepout, Coordi p, int layer)
{
    if (!keepout.all_cu_layers && keepout.polygon->layer != layer) // other layer
        return false;
    if (keepout.patch_types_cu.count(PatchType::TRACK) == 0)
        return false;
    ClipperLib::Path keepout_contour;
    {
        auto poly2 = keepout.polygon->remove_arcs();
        keepout_contour.reserve(poly2.vertices.size());
        for (const auto &it : poly2.vertices) {
            keepout_contour.push_back({it.position.x, it.position.y});
        }
    }
    // returns 0 if false, +1 if true, -1 if pt ON polygon boundary
    return ClipperLib::PointInPolygon({p.x, p.y}, keepout_contour) != 0;
}

ToolResponse ToolRouteTrackInteractive::update(const ToolArgs &args)
{
    if (tool_id == ToolID::DRAG_TRACK_INTERACTIVE) {
        if (args.type == ToolEventType::MOVE) {
            wrapper->updateEndItem(args);
            router->Move(wrapper->m_endSnapPoint, wrapper->m_endItem);
        }
        else if (args.type == ToolEventType::ACTION) {
            if ((args.action == InToolActionID::LMB) || (is_transient && args.action == InToolActionID::LMB_RELEASE)) {
                wrapper->updateEndItem(args);
                if (router->FixRoute(wrapper->m_endSnapPoint, wrapper->m_endItem)) {
                    router->StopRouting();
                    imp->canvas_update();
                    return ToolResponse::commit();
                }
            }
            else if (any_of(args.action, {InToolActionID::RMB, InToolActionID::CANCEL})) {
                return ToolResponse::revert();
            }
        }
    }
    else if (is_tune()) {
        if (args.type == ToolEventType::MOVE) {
            canvas->set_cursor_pos(args.coords);
            router->Move(VECTOR2I(args.coords.x, args.coords.y), NULL);
        }
        else if (args.type == ToolEventType::ACTION) {
            PNS::MEANDER_SETTINGS meander_settings = meander_placer->MeanderSettings();

            switch (args.action) {
            case InToolActionID::LMB:
                if (router->FixRoute(VECTOR2I(args.coords.x, args.coords.y), NULL)) {
                    router->StopRouting();
                    return ToolResponse::commit();
                }
                break;

            case InToolActionID::RMB:
            case InToolActionID::CANCEL:
                return ToolResponse::revert();

            case InToolActionID::LENGTH_TUNING_LENGTH: {
                if (auto r = imp->dialogs.ask_datum("Target length", meander_settings.m_targetLength)) {
                    meander_settings.m_targetLength = *r;
                    meander_placer->UpdateSettings(meander_settings);
                    router->Move(VECTOR2I(args.coords.x, args.coords.y), NULL);
                }
            } break;

            case InToolActionID::LENGTH_TUNING_SPACING_INC:
            case InToolActionID::LENGTH_TUNING_SPACING_DEC: {
                int dir = 0;
                if (args.action == InToolActionID::LENGTH_TUNING_SPACING_DEC) {
                    dir = -1;
                }
                else {
                    dir = 1;
                }
                meander_placer->SpacingStep(dir);
                imp->tool_bar_flash_replace("Meander spacing: "
                                            + dim_to_string(meander_placer->MeanderSettings().m_spacing) + " <i>"
                                            + meander_placer->TuningInfo(EDA_UNITS::MILLIMETRES) + "</i>");
                router->Move(VECTOR2I(args.coords.x, args.coords.y), NULL);
            } break;

            case InToolActionID::LENGTH_TUNING_AMPLITUDE_INC:
            case InToolActionID::LENGTH_TUNING_AMPLITUDE_DEC: {
                int dir = 0;
                if (args.action == InToolActionID::LENGTH_TUNING_AMPLITUDE_DEC) {
                    dir = -1;
                }
                else {
                    dir = 1;
                }
                meander_placer->AmplitudeStep(dir);
                imp->tool_bar_flash_replace("Meander amplitude: "
                                            + dim_to_string(meander_placer->MeanderSettings().m_maxAmplitude) + " <i>"
                                            + meander_placer->TuningInfo(EDA_UNITS::MILLIMETRES) + "</i>");
                router->Move(VECTOR2I(args.coords.x, args.coords.y), NULL);
            } break;

            default:;
            }
        }
    }
    else if (tool_id == ToolID::ROUTE_TRACK_INTERACTIVE || tool_id == ToolID::ROUTE_DIFFPAIR_INTERACTIVE) {
        if (state == State::START) {
            if (args.type == ToolEventType::MOVE) {
                wrapper->updateStartItem(args);
            }
            else if (args.type == ToolEventType::ACTION) {
                switch (args.action) {
                case InToolActionID::LMB:
                    wrapper->updateStartItem(args);
                    for (const auto &it : board->keepouts) {
                        if (keepout_hit(it.second, {wrapper->m_startSnapPoint.x, wrapper->m_startSnapPoint.y},
                                        PNS::PNS_HORIZON_IFACE::layer_from_router(wrapper->getStartLayer()))) {
                            imp->tool_bar_flash("Can't start routing in keepout");
                            return ToolResponse::end();
                        }
                    }
                    state = State::ROUTING;
                    update_settings_window();
                    if (!wrapper->prepareInteractive()) {
                        return ToolResponse::end();
                    }
                    break;

                case InToolActionID::ROUTER_MODE:
                    if (!imp->dialogs.get_nonmodal()) {
                        switch (settings.mode) {
                        case Settings::Mode::BEND:
                            settings.mode = Settings::Mode::STRAIGHT;
                            break;

                        case Settings::Mode::STRAIGHT:
                            settings.mode = Settings::Mode::BEND;
                            break;

                        case Settings::Mode::WALKAROUND:
                            settings.mode = Settings::Mode::PUSH;
                            break;

                        case Settings::Mode::PUSH:
                            settings.mode = Settings::Mode::WALKAROUND;
                            break;
                        }
                    }
                    break;

                case InToolActionID::RMB:
                case InToolActionID::CANCEL:
                    return ToolResponse::commit();

                default:;
                }
            }
        }
        else if (state == State::ROUTING) {
            if (args.type == ToolEventType::MOVE) {
                wrapper->updateEndItem(args);
                router->Move(wrapper->m_endSnapPoint, wrapper->m_endItem);
            }
            else if (args.type == ToolEventType::ACTION) {
                switch (args.action) {
                case InToolActionID::LMB:
                case InToolActionID::LMB_DOUBLE: {
                    if (args.action == InToolActionID::LMB)
                        wrapper->updateEndItem(args);

                    const bool was_placing_via = router->IsPlacingVia();
                    const bool force_finish = args.action == InToolActionID::LMB_DOUBLE;

                    if (router->FixRoute(wrapper->m_endSnapPoint, wrapper->m_endItem, force_finish) || force_finish) {
                        router->CommitRouting();
                        router->StopRouting();
                        imp->canvas_update();
                        state = State::START;
                        imp->get_highlights().clear();
                        imp->update_highlights();
                        update_tip();
                        update_settings_window();
                        return ToolResponse();
                    }
                    imp->canvas_update();


                    if (was_placing_via) {
                        auto layer = PNS::PNS_HORIZON_IFACE::layer_from_router(router->GetCurrentLayer());
                        auto nets = router->GetCurrentNets();
                        const Net *net = nullptr;
                        for (auto x : nets) {
                            net = iface->get_net_for_code(x);
                        }
                        layer = rules->get_layer_pair(net, layer);
                        // clamp layer to via span
                        {
                            const auto defcode = router->Sizes().ViaDefinition();
                            if (auto uu = iface->get_via_definition_for_code(defcode)) {
                                const auto &span = rules->get_via_definitions().via_definitions.at(uu).span;
                                layer = std::clamp(layer, span.start(), span.end());
                            }
                        }
                        router->SwitchLayer(PNS::PNS_HORIZON_IFACE::layer_to_router(layer));
                    }

                    // Synchronize the indicated layer
                    imp->set_work_layer(PNS::PNS_HORIZON_IFACE::layer_from_router(router->GetCurrentLayer()));
                    wrapper->updateEndItem(args);
                    router->Move(wrapper->m_endSnapPoint, wrapper->m_endItem);
                    wrapper->m_startItem = NULL;
                } break;

                case InToolActionID::RMB:
                    router->CommitRouting();
                    router->StopRouting();
                    imp->canvas_update();
                    state = State::START;
                    imp->get_highlights().clear();
                    imp->update_highlights();
                    update_tip();
                    update_settings_window();
                    return ToolResponse();

                case InToolActionID::CANCEL:
                    router->StopRouting();
                    return ToolResponse::commit();

                case InToolActionID::POSTURE:
                    router->FlipPosture();
                    wrapper->updateEndItem(args);
                    router->Move(wrapper->m_endSnapPoint, wrapper->m_endItem);
                    break;

                case InToolActionID::TOGGLE_VIA: {
                    update_via_settings();
                    router->ToggleViaPlacement();
                    wrapper->updateEndItem(args);
                    router->Move(wrapper->m_endSnapPoint, wrapper->m_endItem);
                } break;

                case InToolActionID::SELECT_VIA_DEFINITION: {
                    const auto &rule_via_defs = rules->get_via_definitions();
                    ViaDefinition via_def_from_rules{UUID{}};
                    Net *net = nullptr;
                    {
                        auto nets = router->GetCurrentNets();
                        if (nets.size()) {
                            net = iface->get_net_for_code(nets.front());
                        }
                    }
                    via_def_from_rules.padstack = rules->get_via_padstack_uuid(net);
                    via_def_from_rules.parameters = rules->get_via_parameter_set(net);
                    via_def_from_rules.span = BoardLayers::layer_range_through;
                    if (auto def = imp->dialogs.select_via_definition(rule_via_defs, *board, via_def_from_rules,
                                                                      doc.r->get_pool())) {
                        via_definition_uuid = *def;
                        update_via_settings();
                        if (!router->IsPlacingVia())
                            router->ToggleViaPlacement();
                        wrapper->updateEndItem(args);
                        router->Move(wrapper->m_endSnapPoint, wrapper->m_endItem);
                    }
                } break;

                case InToolActionID::DELETE_LAST_SEGMENT:
                    router->UndoLastSegment();
                    wrapper->updateEndItem(args);
                    router->Move(wrapper->m_endSnapPoint, wrapper->m_endItem);
                    break;

                default:;
                }
            }
            else if (args.type == ToolEventType::LAYER_CHANGE) {
                if (BoardLayers::is_copper(args.work_layer)) {
                    router->SwitchLayer(PNS::PNS_HORIZON_IFACE::layer_to_router(args.work_layer));
                    wrapper->updateEndItem(args);
                    router->Move(wrapper->m_endSnapPoint, wrapper->m_endItem);
                }
            }
        }
        if (args.type == ToolEventType::ACTION) {
            switch (args.action) {
            case InToolActionID::ENTER_WIDTH: {
                PNS::SIZES_SETTINGS sz(router->Sizes());
                if (auto r = imp->dialogs.ask_datum("Track width", sz.TrackWidth())) {
                    sz.SetTrackWidth(*r);
                    sz.SetTrackWidthIsExplicit(true);
                    router->UpdateSizes(sz);
                    router->Move(wrapper->m_endSnapPoint, wrapper->m_endItem);
                }
            } break;

            case InToolActionID::TRACK_WIDTH_DEFAULT: {
                auto nets = router->GetCurrentNets();
                Net *net = nullptr;
                for (auto x : nets) {
                    net = iface->get_net_for_code(x);
                }
                if (net) {
                    PNS::SIZES_SETTINGS sz(router->Sizes());
                    sz.SetTrackWidth(rules->get_default_track_width(
                            net, PNS::PNS_HORIZON_IFACE::layer_from_router(router->GetCurrentLayer())));
                    sz.SetTrackWidthIsExplicit(true); // workaround to get updated track width
                    router->UpdateSizes(sz);
                    sz.SetTrackWidthIsExplicit(false);
                    router->UpdateSizes(sz);
                    router->Move(wrapper->m_endSnapPoint, wrapper->m_endItem);
                }
            } break;

            case InToolActionID::CLEARANCE_OFFSET: {
                if (auto r = imp->dialogs.ask_datum("Routing offset", .1_mm)) {
                    iface->set_override_routing_offset(*r);
                    router->Move(wrapper->m_endSnapPoint, wrapper->m_endItem);
                }
            } break;

            case InToolActionID::CLEARANCE_OFFSET_DEFAULT: {
                iface->set_override_routing_offset(-1);
                router->Move(wrapper->m_endSnapPoint, wrapper->m_endItem);
            } break;

            case InToolActionID::ROUTER_SETTINGS:
                imp->dialogs.show_router_settings_window(settings);
                settings_window_visible = true;
                update_settings_window();
                break;

            case InToolActionID::TOGGLE_CORNER_STYLE:
                if (!imp->dialogs.get_nonmodal()) {
                    switch (settings.corner_mode) {
                    case Settings::CornerMode::MITERED_45:
                        settings.corner_mode = Settings::CornerMode::ROUNDED_45;
                        break;

                    case Settings::CornerMode::ROUNDED_45:
                        settings.corner_mode = Settings::CornerMode::MITERED_90;
                        break;

                    case Settings::CornerMode::MITERED_90:
                        settings.corner_mode = Settings::CornerMode::ROUNDED_90;
                        break;

                    case Settings::CornerMode::ROUNDED_90:
                        settings.corner_mode = Settings::CornerMode::MITERED_45;
                        break;
                    }
                    apply_settings();
                    router->Move(wrapper->m_endSnapPoint, wrapper->m_endItem);
                }
                break;

            default:;
            }
        }
        else if (args.type == ToolEventType::DATA) {
            if (auto data = dynamic_cast<const ToolDataWindow *>(args.data.get())) {
                if (data->event == ToolDataWindow::Event::UPDATE) {
                    apply_settings();
                }
                else if (data->event == ToolDataWindow::Event::CLOSE) {
                    settings_window_visible = false;
                    update_tip();
                }
            }
        }
    }
    update_tip();
    return ToolResponse();
}

void ToolRouteTrackInteractive::update_via_settings()
{
    auto nets = router->GetCurrentNets();
    if (nets.size() < 1)
        return;
    auto sizes = router->Sizes();
    auto net = iface->get_net_for_code(nets.front());
    ParameterSet vps;
    if (via_definition_uuid == UUID()) {
        vps = rules->get_via_parameter_set(net);
        sizes.SetViaDefinition(-1);
        sizes.SetViaType(VIATYPE::THROUGH);
    }
    else {
        auto via_def = rules->get_via_definitions().via_definitions.at(via_definition_uuid);
        vps = via_def.parameters;
        auto defcode = iface->get_via_definition_code(via_definition_uuid);
        sizes.SetViaDefinition(defcode);
        sizes.SetViaType(VIATYPE::BLIND_BURIED);
        sizes.ClearLayerPairs();
        sizes.AddLayerPair(PNS::PNS_HORIZON_IFACE::layer_to_router(via_def.span.start()),
                           PNS::PNS_HORIZON_IFACE::layer_to_router(via_def.span.end()));
    }
    if (vps.count(horizon::ParameterID::VIA_DIAMETER)) {
        sizes.SetViaDiameter(vps.at(horizon::ParameterID::VIA_DIAMETER));
    }
    if (vps.count(horizon::ParameterID::HOLE_DIAMETER)) {
        sizes.SetViaDrill(vps.at(horizon::ParameterID::HOLE_DIAMETER));
    }
    router->UpdateSizes(sizes);
}

static std::string format_length(double l)
{
    l /= 1e6;
    std::stringstream ss;
    ss.imbue(get_locale());
    ss << l;
    ss << " mm";
    return ss.str();
}

void ToolRouteTrackInteractive::update_tip()
{
    std::stringstream ss;
    std::vector<ActionLabelInfo> actions;
    actions.reserve(13);
    if (tool_id == ToolID::DRAG_TRACK_INTERACTIVE) {
        imp->tool_bar_set_actions({
                {InToolActionID::LMB},
                {InToolActionID::RMB},
        });
        return;
    }
    if (is_tune()) {
        if (meander_placer) {
            imp->tool_bar_set_actions({
                    {InToolActionID::LMB},
                    {InToolActionID::RMB},
                    {InToolActionID::LENGTH_TUNING_LENGTH},
                    {InToolActionID::LENGTH_TUNING_AMPLITUDE_DEC, InToolActionID::LENGTH_TUNING_AMPLITUDE_INC,
                     "amplitude"},
                    {InToolActionID::LENGTH_TUNING_SPACING_DEC, InToolActionID::LENGTH_TUNING_SPACING_INC, "spacing"},
            });
            imp->tool_bar_set_tip(meander_placer->TuningInfo(EDA_UNITS::MILLIMETRES));
        }
        return;
    }
    if (state == State::ROUTING) {
        actions.emplace_back(InToolActionID::LMB, "connect");
        actions.emplace_back(InToolActionID::RMB, "finish");
        actions.emplace_back(InToolActionID::POSTURE);
        if (tool_id != ToolID::ROUTE_DIFFPAIR_INTERACTIVE)
            actions.emplace_back(InToolActionID::DELETE_LAST_SEGMENT);
        actions.emplace_back(InToolActionID::TOGGLE_VIA);
        if (rules->get_via_definitions().via_definitions.size())
            actions.emplace_back(InToolActionID::SELECT_VIA_DEFINITION);
        actions.emplace_back(InToolActionID::ROUTER_SETTINGS);
        if (!settings_window_visible && tool_id != ToolID::ROUTE_DIFFPAIR_INTERACTIVE)
            actions.emplace_back(InToolActionID::TOGGLE_CORNER_STYLE);
        const auto &sz = router->Sizes();
        if (!sz.TrackWidthIsExplicit()) {
            actions.emplace_back(InToolActionID::ENTER_WIDTH, "track width");
        }
        else {
            actions.emplace_back(InToolActionID::ENTER_WIDTH, InToolActionID::TRACK_WIDTH_DEFAULT,
                                 "track width / default");
        }
        if (iface->get_override_routing_offset() == -1) {
            actions.emplace_back(InToolActionID::CLEARANCE_OFFSET, "set clearance offset");
        }
        else {
            actions.emplace_back(InToolActionID::CLEARANCE_OFFSET, InToolActionID::CLEARANCE_OFFSET_DEFAULT,
                                 "set / reset clearance offset");
        }
        if (tool_id != ToolID::ROUTE_DIFFPAIR_INTERACTIVE)
            ss << Settings::corner_mode_names.at(settings.corner_mode) << " ";
        auto nets = router->GetCurrentNets();
        Net *net = nullptr;
        for (auto x : nets) {
            net = iface->get_net_for_code(x);
        }
        if (net) {
            if (net->name.size()) {
                ss << "Net \"" << net->name << "\"";
            }
            else {
                ss << "routing unnamed net";
            }
        }
        else {
            ss << "routing no net";
        }

        ss << "  width " << format_length(sz.TrackWidth());
        if (!sz.TrackWidthIsExplicit()) {
            ss << " (default)";
        }
        if (router->IsPlacingVia()) {
            const auto defcode = router->Sizes().ViaDefinition();
            if (auto uu = iface->get_via_definition_for_code(defcode)) {
                ss << " defined via: " << rules->get_via_definitions().via_definitions.at(uu).name;
            }
            else {
                ss << " via from rules";
            }
        }
    }
    else {
        actions.emplace_back(InToolActionID::LMB, "select staring point");
        actions.emplace_back(InToolActionID::RMB, "cancel");
        if (!settings_window_visible)
            actions.emplace_back(InToolActionID::ROUTER_MODE);
        actions.emplace_back(InToolActionID::ROUTER_SETTINGS);
        if (!settings_window_visible && tool_id != ToolID::ROUTE_DIFFPAIR_INTERACTIVE)
            actions.emplace_back(InToolActionID::TOGGLE_CORNER_STYLE);
        ss << "Mode: " << Glib::Markup::escape_text(Settings::mode_names.at(settings.mode));
        ss << " Corners: " << Settings::corner_mode_names.at(settings.corner_mode);

        if (wrapper->m_startItem) {
            auto nc = wrapper->m_startItem->Net();
            auto net = iface->get_net_for_code(nc);

            if (net)
                ss << " Current Net: " << net->name;
            ;
        }
    }
    imp->tool_bar_set_actions(actions);
    imp->tool_bar_set_tip(ss.str());
}

void ToolRouteTrackInteractive::update_settings_window()
{
    if (auto win = dynamic_cast<RouterSettingsWindow *>(imp->dialogs.get_nonmodal())) {
        win->set_is_routing(state == State::ROUTING);
    }
}


} // namespace horizon
