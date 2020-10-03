#include "tool_route_track_interactive.hpp"
#include "board/board_layers.hpp"
#include "board/board_rules.hpp"
#include "canvas/canvas_gl.hpp"
#include "document/idocument_board.hpp"
#include "board/board.hpp"
#include "imp/imp_interface.hpp"
#include "router/pns_horizon_iface.hpp"
#include "router/pns_solid.h"
#include "router/router/pns_router.h"
#include "router/router/pns_meander_placer_base.h"
#include "util/util.hpp"
#include "core/tool_id.hpp"
#include "util/selection_util.hpp"
#include "dialogs/router_settings_window.hpp"
#include <iostream>
#include <iomanip>
#include "nlohmann/json.hpp"

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

    ToolRouteTrackInteractive *tool = nullptr;
};

ToolRouteTrackInteractive::ToolRouteTrackInteractive(IDocument *c, ToolID tid) : ToolBase(c, tid)
{
}

const std::map<ToolRouteTrackInteractive::Settings::Mode, std::string> ToolRouteTrackInteractive::Settings::mode_names =
        {{ToolRouteTrackInteractive::Settings::Mode::BEND, "45 degree"},
         {ToolRouteTrackInteractive::Settings::Mode::STRAIGHT, "Straight"},
         {ToolRouteTrackInteractive::Settings::Mode::PUSH, "Push & shove"},
         {ToolRouteTrackInteractive::Settings::Mode::WALKAROUND, "Walkaround"}};

static const LutEnumStr<ToolRouteTrackInteractive::Settings::Mode> mode_lut = {
        {"bend", ToolRouteTrackInteractive::Settings::Mode::BEND},
        {"straight", ToolRouteTrackInteractive::Settings::Mode::STRAIGHT},
        {"push", ToolRouteTrackInteractive::Settings::Mode::PUSH},
        {"walkaround", ToolRouteTrackInteractive::Settings::Mode::WALKAROUND},
};

json ToolRouteTrackInteractive::Settings::serialize() const
{
    json j;
    j["effort"] = effort;
    j["remove_loops"] = remove_loops;
    j["drc"] = drc;
    j["mode"] = mode_lut.lookup_reverse(mode);
    return j;
}

void ToolRouteTrackInteractive::Settings::load_from_json(const json &j)
{
    effort = j.value("effort", 1);
    remove_loops = j.value("remove_loops", true);
    drc = j.value("drc", true);
    mode = mode_lut.lookup(j.value("mode", ""), Settings::Mode::WALKAROUND);
}

void ToolRouteTrackInteractive::apply_settings()
{
    if (!router)
        return;
    PNS::ROUTING_SETTINGS routing_settings(router->Settings());
    routing_settings.SetRemoveLoops(settings.remove_loops);
    routing_settings.SetOptimizerEffort(static_cast<PNS::PNS_OPTIMIZATION_EFFORT>(settings.effort));
    routing_settings.SetCanViolateDRC(!settings.drc);
    router->LoadSettings(routing_settings);
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
            return track->net;
        else
            return false;

    case ToolID::TUNE_TRACK:
        return track && track->net;

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
    iface->create_debug_decorator(imp->get_canvas());
    iface->SetCanvas(imp->get_canvas());
    iface->SetRules(rules);
    iface->SetViaPadstackProvider(&doc.b->get_via_padstack_provider());
    // m_iface->SetHostFrame( m_frame );

    router = new PNS::ROUTER;
    router->SetInterface(iface);
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

    PNS::ROUTING_SETTINGS routing_settings;
    routing_settings.SetShoveVias(false);

    PNS::SIZES_SETTINGS sizes_settings;

    router->LoadSettings(routing_settings);
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

        wrapper->m_startItem = router->GetWorld()->FindItemByParent(parent, iface->get_net_code(net->uuid));

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

    PNS::ITEM *prioritized[4] = {0};

    PNS::ITEM_SET candidates = tool->router->QueryHoverItems(aWhere);

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

        if (aNet < 0 || item->Net() == aNet) {
            if (item->OfKind(PNS::ITEM::VIA_T | PNS::ITEM::SOLID_T)) {
                if (!prioritized[2])
                    prioritized[2] = item;
                if (item->Layers().Overlaps(tl))
                    prioritized[0] = item;
            }
            else {
                if (!prioritized[3])
                    prioritized[3] = item;
                if (item->Layers().Overlaps(tl))
                    prioritized[1] = item;
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

static VECTOR2I AlignToSegment(const VECTOR2I &aPoint, const VECTOR2I &snapped, const SEG &aSeg)
{
    OPT_VECTOR2I pts[6];

    VECTOR2I nearest = snapped;

    pts[0] = aSeg.A;
    pts[1] = aSeg.B;
    pts[2] = aSeg.IntersectLines(SEG(nearest, nearest + VECTOR2I(1, 0)));
    pts[3] = aSeg.IntersectLines(SEG(nearest, nearest + VECTOR2I(0, 1)));

    int min_d = std::numeric_limits<int>::max();

    for (int i = 0; i < 4; i++) {
        if (pts[i] && aSeg.Contains(*pts[i])) {
            int d = (*pts[i] - aPoint).EuclideanNorm();

            if (d < min_d) {
                min_d = d;
                nearest = *pts[i];
            }
        }
    }

    return nearest;
}

const VECTOR2I ToolWrapper::snapToItem(bool aEnabled, PNS::ITEM *aItem, VECTOR2I aP)
{
    VECTOR2I anchor;

    if (!aItem || !aEnabled) {
        auto snapped = tool->canvas->snap_to_grid(Coordi(aP.x, aP.y), tool->canvas->get_last_grid_div());
        return VECTOR2I(snapped.x, snapped.y);
    }

    switch (aItem->Kind()) {
    case PNS::ITEM::SOLID_T:
        anchor = static_cast<PNS::SOLID *>(aItem)->Pos();
        break;

    case PNS::ITEM::VIA_T:
        anchor = static_cast<PNS::VIA *>(aItem)->Pos();
        break;

    case PNS::ITEM::SEGMENT_T: {
        PNS::SEGMENT *seg = static_cast<PNS::SEGMENT *>(aItem);
        const SEG &s = seg->Seg();
        int w = seg->Width();

        if ((aP - s.A).EuclideanNorm() < w / 2)
            anchor = s.A;
        else if ((aP - s.B).EuclideanNorm() < w / 2)
            anchor = s.B;
        else {
            auto snapped = tool->canvas->snap_to_grid(Coordi(aP.x, aP.y), tool->canvas->get_last_grid_div());
            anchor = AlignToSegment(aP, VECTOR2I(snapped.x, snapped.y), s);
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
            sizes.SetWidthFromRules(track->width_from_rules);
            track_width = track->width;
        }
    }

    if (!track_width) {
        if (m_startItem) {
            auto netcode = m_startItem->Net();
            auto net = tool->iface->get_net_for_code(netcode);
            track_width =
                    tool->rules->get_default_track_width(net, PNS::PNS_HORIZON_IFACE::layer_from_router(routingLayer));
            sizes.SetWidthFromRules(true);
        }
    }

    sizes.SetTrackWidth(track_width);
    if (tool->tool_id == ToolID::ROUTE_DIFFPAIR_INTERACTIVE && m_startItem) {
        auto netcode = m_startItem->Net();
        auto net = tool->iface->get_net_for_code(netcode);
        auto rules_dp =
                tool->rules->get_diffpair(net->net_class, PNS::PNS_HORIZON_IFACE::layer_from_router(routingLayer));
        sizes.SetDiffPairGap(rules_dp->track_gap);
        sizes.SetDiffPairWidth(rules_dp->track_width);
        sizes.SetDiffPairViaGap(rules_dp->via_gap);
        sizes.SetDiffPairViaGapSameAsTraceGap(false);
        sizes.SetWidthFromRules(false);
    }

    if (m_startItem) {
        auto netcode = m_startItem->Net();
        auto net = tool->iface->get_net_for_code(netcode);
        auto vps = tool->rules->get_via_parameter_set(net);
        if (vps.count(horizon::ParameterID::VIA_DIAMETER)) {
            sizes.SetViaDiameter(vps.at(horizon::ParameterID::VIA_DIAMETER));
        }
        if (vps.count(horizon::ParameterID::HOLE_DIAMETER)) {
            sizes.SetViaDrill(vps.at(horizon::ParameterID::HOLE_DIAMETER));
        }
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
    PNS::ROUTING_SETTINGS settings(tool->router->Settings());
    using Mode = ToolRouteTrackInteractive::Settings::Mode;
    switch (tool->settings.mode) {
    case Mode::WALKAROUND:
        settings.SetMode(PNS::RM_Walkaround);
        settings.SetFreeAngleMode(false);
        settings.SetCanViolateDRC(false);
        break;

    case Mode::PUSH:
        settings.SetMode(PNS::RM_Shove);
        settings.SetFreeAngleMode(false);
        settings.SetCanViolateDRC(false);
        break;

    case Mode::BEND:
        settings.SetMode(PNS::RM_MarkObstacles);
        settings.SetFreeAngleMode(false);
        settings.SetCanViolateDRC(!tool->settings.drc);
        break;

    case Mode::STRAIGHT:
        settings.SetMode(PNS::RM_MarkObstacles);
        settings.SetFreeAngleMode(true);
        settings.SetCanViolateDRC(!tool->settings.drc);
        break;
    }
    tool->router->LoadSettings(settings);

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
                    board->expand_flags =
                            static_cast<Board::ExpandFlags>(Board::EXPAND_PROPAGATE_NETS | Board::EXPAND_AIRWIRES);
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
                    board->expand_flags =
                            static_cast<Board::ExpandFlags>(Board::EXPAND_PROPAGATE_NETS | Board::EXPAND_AIRWIRES);
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
                imp->tool_bar_flash("Meander spacing: " + dim_to_string(meander_placer->MeanderSettings().m_spacing)
                                    + " <i>" + meander_placer->TuningInfo() + "</i>");
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
                imp->tool_bar_flash("Meander amplitude: "
                                    + dim_to_string(meander_placer->MeanderSettings().m_maxAmplitude) + " <i>"
                                    + meander_placer->TuningInfo() + "</i>");
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
                    board->expand_flags =
                            static_cast<Board::ExpandFlags>(Board::EXPAND_PROPAGATE_NETS | Board::EXPAND_AIRWIRES);
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
                case InToolActionID::LMB: {
                    wrapper->updateEndItem(args);

                    const bool was_placing_via = router->IsPlacingVia();

                    if (router->FixRoute(wrapper->m_endSnapPoint, wrapper->m_endItem)) {
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
                        router->SwitchLayer(PNS::PNS_HORIZON_IFACE::layer_to_router(layer));
                    }

                    // Synchronize the indicated layer
                    imp->set_work_layer(PNS::PNS_HORIZON_IFACE::layer_from_router(router->GetCurrentLayer()));
                    wrapper->updateEndItem(args);
                    router->Move(wrapper->m_endSnapPoint, wrapper->m_endItem);
                    wrapper->m_startItem = NULL;
                } break;

                case InToolActionID::RMB:
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
                    board->expand_flags =
                            static_cast<Board::ExpandFlags>(Board::EXPAND_PROPAGATE_NETS | Board::EXPAND_AIRWIRES);
                    return ToolResponse::commit();

                case InToolActionID::POSTURE:
                    router->FlipPosture();
                    wrapper->updateEndItem(args);
                    router->Move(wrapper->m_endSnapPoint, wrapper->m_endItem);
                    break;

                case InToolActionID::TOGGLE_VIA:
                    router->ToggleViaPlacement();
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
                    sz.SetWidthFromRules(false);
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
                    sz.SetWidthFromRules(true);
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
    actions.reserve(12);
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
            imp->tool_bar_set_tip(meander_placer->TuningInfo());
        }
        return;
    }
    if (state == State::ROUTING) {
        actions.emplace_back(InToolActionID::LMB, "connect");
        actions.emplace_back(InToolActionID::RMB, "finish");
        actions.emplace_back(InToolActionID::POSTURE);
        actions.emplace_back(InToolActionID::TOGGLE_VIA);
        actions.emplace_back(InToolActionID::ROUTER_SETTINGS);
        const auto &sz = router->Sizes();
        if (sz.WidthFromRules()) {
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
        if (sz.WidthFromRules()) {
            ss << " (default)";
        }
    }
    else {
        actions.emplace_back(InToolActionID::LMB, "select staring point");
        actions.emplace_back(InToolActionID::RMB, "cancel");
        if (!settings_window_visible)
            actions.emplace_back(InToolActionID::ROUTER_MODE);
        actions.emplace_back(InToolActionID::ROUTER_SETTINGS);
        ss << "Mode: " << Glib::Markup::escape_text(Settings::mode_names.at(settings.mode));
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
