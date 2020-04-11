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
#include <iostream>

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
    switch (tool_id) {
    case ToolID::DRAG_TRACK_INTERACTIVE:
    case ToolID::TUNE_TRACK:
        return get_track(selection);

    case ToolID::TUNE_DIFFPAIR:
    case ToolID::TUNE_DIFFPAIR_SKEW: {
        auto track = get_track(selection);
        return track && track->net && track->net->diffpair;
    }

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
    Track *track = nullptr;
    for (const auto &it : sel) {
        if (it.type == ObjectType::TRACK) {
            if (track == nullptr) {
                track = &doc.b->get_board()->tracks.at(it.uuid);
            }
            else {
                return nullptr;
            }
        }
    }
    return track;
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
    iface->SetViaPadstackProvider(doc.b->get_via_padstack_provider());
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

    PNS::ROUTING_SETTINGS settings;
    settings.SetShoveVias(false);

    PNS::SIZES_SETTINGS sizes_settings;

    router->LoadSettings(settings);
    router->UpdateSizes(sizes_settings);

    imp->canvas_update();
    update_tip();

    if (tool_id == ToolID::DRAG_TRACK_INTERACTIVE) {
        Track *track = get_track(args.selection);
        if (!track) {
            return ToolResponse::end();
        }
        auto parent = iface->get_parent(track);
        wrapper->m_startItem = router->GetWorld()->FindItemByParent(parent, iface->get_net_code(track->net.uuid));

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
        router->Move(p0, NULL);
        meander_placer = dynamic_cast<PNS::MEANDER_PLACER_BASE *>(router->Placer());
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
        auto snapped = tool->canvas->snap_to_grid(Coordi(aP.x, aP.y));
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
            auto snapped = tool->canvas->snap_to_grid(Coordi(aP.x, aP.y));
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
    settings.SetMode(tool->shove ? PNS::RM_Shove : PNS::RM_Walkaround);
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
        else if (args.type == ToolEventType::CLICK) {
            if (args.button == 1) {
                wrapper->updateEndItem(args);
                if (router->FixRoute(wrapper->m_endSnapPoint, wrapper->m_endItem)) {
                    router->StopRouting();
                    imp->canvas_update();
                    board->expand_flags =
                            static_cast<Board::ExpandFlags>(Board::EXPAND_PROPAGATE_NETS | Board::EXPAND_AIRWIRES);
                    return ToolResponse::commit();
                }
            }
            else if (args.button == 3) {
                return ToolResponse::revert();
            }
        }
        else if (args.type == ToolEventType::KEY) {
            if (args.key == GDK_KEY_Escape) {
                board->expand_flags =
                        static_cast<Board::ExpandFlags>(Board::EXPAND_PROPAGATE_NETS | Board::EXPAND_AIRWIRES);
                return ToolResponse::commit();
            }
        }
    }
    else if (is_tune()) {
        if (args.type == ToolEventType::MOVE) {
            canvas->set_cursor_pos(args.coords);
            router->Move(VECTOR2I(args.coords.x, args.coords.y), NULL);
        }
        else if (args.type == ToolEventType::CLICK) {
            if (args.button == 1) {
                if (router->FixRoute(VECTOR2I(args.coords.x, args.coords.y), NULL)) {
                    router->StopRouting();
                    board->expand_flags =
                            static_cast<Board::ExpandFlags>(Board::EXPAND_PROPAGATE_NETS | Board::EXPAND_AIRWIRES);
                    return ToolResponse::commit();
                }
            }
            else if (args.button == 3) {
                board->expand_flags =
                        static_cast<Board::ExpandFlags>(Board::EXPAND_PROPAGATE_NETS | Board::EXPAND_AIRWIRES);
                return ToolResponse::commit();
            }
        }
        else if (args.type == ToolEventType::KEY) {
            PNS::MEANDER_SETTINGS settings = meander_placer->MeanderSettings();
            if (args.key == GDK_KEY_l) {
                auto r = imp->dialogs.ask_datum("Target length", settings.m_targetLength);
                if (r.first) {
                    settings.m_targetLength = r.second;
                    meander_placer->UpdateSettings(settings);
                    router->Move(VECTOR2I(args.coords.x, args.coords.y), NULL);
                }
            }
            else if (args.key == GDK_KEY_less || args.key == GDK_KEY_greater) {
                int dir = 0;
                if (args.key == GDK_KEY_less) {
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
            }
            else if (args.key == GDK_KEY_comma || args.key == GDK_KEY_period) {
                int dir = 0;
                if (args.key == GDK_KEY_comma) {
                    dir = -1;
                }
                else {
                    dir = 1;
                }
                meander_placer->SpacingStep(dir);
                imp->tool_bar_flash("Meander spacing: " + dim_to_string(meander_placer->MeanderSettings().m_spacing)
                                    + " <i>" + meander_placer->TuningInfo() + "</i>");
                router->Move(VECTOR2I(args.coords.x, args.coords.y), NULL);
            }
            else if (args.key == GDK_KEY_Escape) {
                board->expand_flags =
                        static_cast<Board::ExpandFlags>(Board::EXPAND_PROPAGATE_NETS | Board::EXPAND_AIRWIRES);
                return ToolResponse::commit();
            }
        }
    }
    else if (tool_id == ToolID::ROUTE_TRACK_INTERACTIVE || tool_id == ToolID::ROUTE_DIFFPAIR_INTERACTIVE) {
        if (state == State::START) {
            if (args.type == ToolEventType::MOVE) {
                wrapper->updateStartItem(args);
            }
            else if (args.type == ToolEventType::KEY) {
                if (args.key == GDK_KEY_s) {
                    shove ^= true;
                }
                else if (args.key == GDK_KEY_Escape) {
                    board->expand_flags =
                            static_cast<Board::ExpandFlags>(Board::EXPAND_PROPAGATE_NETS | Board::EXPAND_AIRWIRES);
                    return ToolResponse::commit();
                }
            }
            else if (args.type == ToolEventType::CLICK) {
                wrapper->updateStartItem(args);
                if (args.button == 1) {
                    for (const auto &it : board->keepouts) {
                        if (keepout_hit(it.second, {wrapper->m_startSnapPoint.x, wrapper->m_startSnapPoint.y},
                                        PNS::PNS_HORIZON_IFACE::layer_from_router(wrapper->getStartLayer()))) {
                            imp->tool_bar_flash("Can't start routing in keepout");
                            return ToolResponse::end();
                        }
                    }
                    state = State::ROUTING;
                    if (!wrapper->prepareInteractive()) {
                        return ToolResponse::end();
                    }
                }
                else if (args.button == 3) {
                    board->expand_flags =
                            static_cast<Board::ExpandFlags>(Board::EXPAND_PROPAGATE_NETS | Board::EXPAND_AIRWIRES);
                    return ToolResponse::commit();
                }
            }
        }
        else if (state == State::ROUTING) {
            if (args.type == ToolEventType::MOVE) {
                wrapper->updateEndItem(args);
                router->Move(wrapper->m_endSnapPoint, wrapper->m_endItem);
            }
            else if (args.type == ToolEventType::CLICK) {
                if (args.button == 1) {
                    wrapper->updateEndItem(args);

                    if (router->FixRoute(wrapper->m_endSnapPoint, wrapper->m_endItem)) {
                        router->StopRouting();
                        imp->canvas_update();
                        state = State::START;
                        imp->get_highlights().clear();
                        imp->update_highlights();
                        update_tip();
                        return ToolResponse();
                    }
                    imp->canvas_update();

                    // Synchronize the indicated layer
                    imp->set_work_layer(PNS::PNS_HORIZON_IFACE::layer_from_router(router->GetCurrentLayer()));
                    wrapper->updateEndItem(args);
                    router->Move(wrapper->m_endSnapPoint, wrapper->m_endItem);
                    wrapper->m_startItem = NULL;
                }
                else if (args.button == 3) {
                    router->StopRouting();
                    imp->canvas_update();
                    state = State::START;
                    imp->get_highlights().clear();
                    imp->update_highlights();
                    update_tip();
                    return ToolResponse();
                }
            }
            else if (args.type == ToolEventType::LAYER_CHANGE) {
                if (BoardLayers::is_copper(args.work_layer)) {
                    router->SwitchLayer(PNS::PNS_HORIZON_IFACE::layer_to_router(args.work_layer));
                    wrapper->updateEndItem(args);
                    router->Move(wrapper->m_endSnapPoint, wrapper->m_endItem);
                }
            }
            else if (args.type == ToolEventType::KEY) {
                if (args.key == GDK_KEY_slash) {
                    router->FlipPosture();
                    wrapper->updateEndItem(args);
                    router->Move(wrapper->m_endSnapPoint, wrapper->m_endItem);
                }
                else if (args.key == GDK_KEY_v) {
                    router->ToggleViaPlacement();
                    wrapper->updateEndItem(args);
                    router->Move(wrapper->m_endSnapPoint, wrapper->m_endItem);
                }
                else if (args.key == GDK_KEY_Escape) {
                    router->StopRouting();
                    board->expand_flags =
                            static_cast<Board::ExpandFlags>(Board::EXPAND_PROPAGATE_NETS | Board::EXPAND_AIRWIRES);
                    return ToolResponse::commit();
                }
            }
        }
        if (args.type == ToolEventType::KEY) {
            if (args.key == GDK_KEY_w) {
                PNS::SIZES_SETTINGS sz(router->Sizes());
                auto r = imp->dialogs.ask_datum("Track width", sz.TrackWidth());
                if (r.first) {
                    sz.SetTrackWidth(r.second);
                    sz.SetWidthFromRules(false);
                    router->UpdateSizes(sz);
                    router->Move(wrapper->m_endSnapPoint, wrapper->m_endItem);
                }
            }
            else if (args.key == GDK_KEY_W) {
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
            }
            else if (args.key == GDK_KEY_o) {
                auto r = imp->dialogs.ask_datum("Routing offset", .1_mm);
                if (r.first) {
                    iface->set_override_routing_offset(r.second);
                    router->Move(wrapper->m_endSnapPoint, wrapper->m_endItem);
                }
            }
            else if (args.key == GDK_KEY_O) {
                iface->set_override_routing_offset(-1);
                router->Move(wrapper->m_endSnapPoint, wrapper->m_endItem);
            }
        }
    }
    update_tip();
    return ToolResponse();
}

void ToolRouteTrackInteractive::update_tip()
{
    std::stringstream ss;
    if (tool_id == ToolID::DRAG_TRACK_INTERACTIVE) {
        imp->tool_bar_set_tip("<b>LMB:</b>place <b>RMB:</b>cancel");
        return;
    }
    if (is_tune()) {
        if (meander_placer) {
            ss << "<b>LMB:</b>place <b>RMB:</b>cancel <b>l:</b>set length <b>&lt;&gt;:</b>+/- amplitude <b>,.:</b>+/- "
                  "spacing";
            ss << " <i>";
            ss << meander_placer->TuningInfo();
            ss << "</i>";
            imp->tool_bar_set_tip(ss.str());
        }
        return;
    }
    if (state == State::ROUTING) {
        ss << "<b>LMB:</b>place junction/connect <b>RMB:</b>finish and delete "
              "last segment <b>/:</b>track posture <b>v:</b>toggle via "
              "<b>w:</b>track width <b>W:</b>default track width "
              "<b>o:</b>clearance offset <b>O:</b>default cl. offset"
              "<i> ";
        auto nets = router->GetCurrentNets();
        Net *net = nullptr;
        for (auto x : nets) {
            net = iface->get_net_for_code(x);
        }
        if (net) {
            if (net->name.size()) {
                ss << "routing net \"" << net->name << "\"";
            }
            else {
                ss << "routing unnamed net";
            }
        }
        else {
            ss << "routing no net";
        }

        PNS::SIZES_SETTINGS sz(router->Sizes());
        ss << "  track width " << dim_to_string(sz.TrackWidth());
        if (sz.WidthFromRules()) {
            ss << " (default)";
        }
        ss << "</i>";
    }
    else {
        ss << "<b>LMB:</b>select starting junction/pad <b>RMB:</b>cancel "
              "<b>s:</b>shove/walkaround ";
        ss << "<i>";
        ss << "Mode: ";
        if (shove)
            ss << "shove";
        else
            ss << "walkaround";
        if (wrapper->m_startItem) {
            auto nc = wrapper->m_startItem->Net();
            auto net = iface->get_net_for_code(nc);

            if (net)
                ss << " Current Net: " << net->name;
            ;
        }
        ss << "</i>";
    }
    imp->tool_bar_set_tip(ss.str());
}
} // namespace horizon
