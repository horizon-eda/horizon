#pragma once
#include "core/tool.hpp"
#include <deque>
#include <memory>
#include <set>

namespace PNS {
class ROUTER;
class PNS_HORIZON_IFACE;
class ITEM;
class MEANDER_PLACER_BASE;
} // namespace PNS

namespace horizon {
class ToolWrapper;
class Board;
class ToolRouteTrackInteractive : public ToolBase {
    friend ToolWrapper;

public:
    ToolRouteTrackInteractive(IDocument *c, ToolID tid);
    ToolResponse begin(const ToolArgs &args) override;
    ToolResponse update(const ToolArgs &args) override;
    bool can_begin() override;
    bool is_specific() override;
    bool handles_esc() override
    {
        return true;
    }
    ~ToolRouteTrackInteractive();

private:
    PNS::ROUTER *router = nullptr;
    PNS::PNS_HORIZON_IFACE *iface = nullptr;
    PNS::MEANDER_PLACER_BASE *meander_placer = nullptr;
    class CanvasGL *canvas = nullptr;
    ToolWrapper *wrapper = nullptr;

    enum class State { START, ROUTING };
    State state = State::START;

    Board *board = nullptr;
    class BoardRules *rules = nullptr;
    bool shove = false;

    void update_tip();
    class Track *get_track(const std::set<SelectableRef> &sel);
    bool is_tune() const;
};
} // namespace horizon
