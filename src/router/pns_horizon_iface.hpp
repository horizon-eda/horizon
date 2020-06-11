#pragma once
#include "canvas/selectables.hpp"
#include "canvas/object_ref.hpp"
#include "router/pns_router.h"
#include "util/uuid.hpp"

namespace horizon {
class Board;
class BoardPackage;
class BoardHole;
class Padstack;
class Placement;
class Pad;
class Track;
class Via;
class CanvasGL;
class Junction;
class Net;
class BoardRules;
class Polygon;
class ViaPadstackProvider;
class Keepout;
class KeepoutContour;
template <typename T> class Coord;
} // namespace horizon

namespace PNS {
class PNS_HORIZON_PARENT_ITEM {
public:
    PNS_HORIZON_PARENT_ITEM()
    {
    }
    PNS_HORIZON_PARENT_ITEM(const horizon::Track *tr) : track(tr)
    {
    }
    PNS_HORIZON_PARENT_ITEM(const horizon::Via *v) : via(v)
    {
    }
    PNS_HORIZON_PARENT_ITEM(const horizon::BoardHole *h) : hole(h)
    {
    }
    PNS_HORIZON_PARENT_ITEM(const horizon::BoardPackage *pkg, const horizon::Pad *p) : package(pkg), pad(p)
    {
    }
    PNS_HORIZON_PARENT_ITEM(const horizon::Keepout *k) : keepout(k)
    {
    }
    PNS_HORIZON_PARENT_ITEM(const horizon::Keepout *k, const horizon::BoardPackage *pkg) : package(pkg), keepout(k)
    {
    }
    bool operator==(const PNS_HORIZON_PARENT_ITEM &other) const
    {
        return track == other.track && via == other.via && package == other.package && pad == other.pad
               && hole == other.hole && keepout == other.keepout;
    }

    const horizon::Track *track = nullptr;
    const horizon::Via *via = nullptr;
    const horizon::BoardPackage *package = nullptr;
    const horizon::Pad *pad = nullptr;
    const horizon::BoardHole *hole = nullptr;
    const horizon::Keepout *keepout = nullptr;
};

class PNS_HORIZON_IFACE : public PNS::ROUTER_IFACE {
public:
    PNS_HORIZON_IFACE();
    ~PNS_HORIZON_IFACE();

    void SetRouter(PNS::ROUTER *aRouter) override;
    void SetBoard(horizon::Board *brd);
    void SetCanvas(class horizon::CanvasGL *ca);
    void SetRules(horizon::BoardRules *rules);
    void SetViaPadstackProvider(horizon::ViaPadstackProvider *v);

    void SyncWorld(PNS::NODE *aWorld) override;
    void EraseView() override;
    void HideItem(PNS::ITEM *aItem) override;
    void DisplayItem(const PNS::ITEM *aItem, int aColor = 0, int aClearance = 0) override;
    void AddItem(PNS::ITEM *aItem) override;
    void RemoveItem(PNS::ITEM *aItem) override;
    void Commit() override;

    void UpdateNet(int aNetCode) override;

    PNS::RULE_RESOLVER *GetRuleResolver() override;
    PNS::DEBUG_DECORATOR *GetDebugDecorator() override;

    void create_debug_decorator(horizon::CanvasGL *ca);

    static int layer_to_router(int l);
    static int layer_from_router(int l);
    horizon::Net *get_net_for_code(int code);
    int get_net_code(const horizon::UUID &uu);

    const PNS_HORIZON_PARENT_ITEM *get_parent(const horizon::Track *track);
    const PNS_HORIZON_PARENT_ITEM *get_parent(const horizon::Via *via);
    const PNS_HORIZON_PARENT_ITEM *get_parent(const horizon::BoardHole *hole);
    const PNS_HORIZON_PARENT_ITEM *get_parent(const horizon::BoardPackage *pkg, const horizon::Pad *pad);
    const PNS_HORIZON_PARENT_ITEM *get_parent(const horizon::Keepout *keepout,
                                              const horizon::BoardPackage *pkg = nullptr);

    int64_t get_override_routing_offset() const
    {
        return override_routing_offset;
    }

    void set_override_routing_offset(int64_t o)
    {
        override_routing_offset = o;
    }

private:
    const PNS_HORIZON_PARENT_ITEM *get_or_create_parent(const PNS_HORIZON_PARENT_ITEM &it);

    class PNS_HORIZON_RULE_RESOLVER *m_ruleResolver = nullptr;
    class PNS_HORIZON_DEBUG_DECORATOR *m_debugDecorator = nullptr;
    std::set<horizon::ObjectRef> m_preview_items;

    horizon::Board *board = nullptr;
    class horizon::CanvasGL *canvas = nullptr;
    class horizon::BoardRules *rules = nullptr;
    class horizon::ViaPadstackProvider *vpp = nullptr;
    PNS::ROUTER *m_router;

    std::unique_ptr<PNS::SOLID> syncPad(const horizon::BoardPackage *pkg, const horizon::Pad *pad);
    std::unique_ptr<PNS::SOLID> syncPadstack(const horizon::Padstack *padstack, const horizon::Placement &tr);
    std::unique_ptr<PNS::SOLID> syncHole(const horizon::BoardHole *hole);
    std::unique_ptr<PNS::SEGMENT> syncTrack(const horizon::Track *track);
    std::unique_ptr<PNS::VIA> syncVia(const horizon::Via *via);
    void syncOutline(const horizon::Polygon *poly, PNS::NODE *aWorld);
    void syncKeepout(const horizon::KeepoutContour *keepout_contour, PNS::NODE *aWorld);
    std::map<horizon::UUID, int> net_code_map;
    std::map<int, horizon::UUID> net_code_map_r;
    int net_code_max = 0;

    int64_t override_routing_offset = -1;

    std::list<PNS_HORIZON_PARENT_ITEM> parents;

    std::pair<horizon::BoardPackage *, horizon::Pad *> find_pad(int layer, const horizon::Coord<int64_t> &c);
    horizon::Junction *find_junction(int layer, const horizon::Coord<int64_t> &c);
};
} // namespace PNS
