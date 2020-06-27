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


    class Settings : public ToolSettings {
    public:
        json serialize() const override;
        void load_from_json(const json &j) override;
        int effort = 1;
        bool remove_loops = true;
    };

    const ToolSettings *get_settings_const() const override
    {
        return &settings;
    }

    void apply_settings() override;

protected:
    ToolSettings *get_settings() override
    {
        return &settings;
    }

private:
    Settings settings;

    PNS::ROUTER *router = nullptr;
    PNS::PNS_HORIZON_IFACE *iface = nullptr;
    PNS::MEANDER_PLACER_BASE *meander_placer = nullptr;
    class CanvasGL *canvas = nullptr;
    ToolWrapper *wrapper = nullptr;

    enum class State { START, ROUTING };
    State state = State::START;

    Board *board = nullptr;
    const class BoardRules *rules = nullptr;
    bool shove = false;

    void update_tip();
    class Track *get_track(const std::set<SelectableRef> &sel);
    class Via *get_via(const std::set<SelectableRef> &sel);
    bool is_tune() const;
};
} // namespace horizon
