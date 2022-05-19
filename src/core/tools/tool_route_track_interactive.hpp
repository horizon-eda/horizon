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
    using ToolBase::ToolBase;
    ToolResponse begin(const ToolArgs &args) override;
    ToolResponse update(const ToolArgs &args) override;
    bool can_begin() override;
    bool is_specific() override;
    ~ToolRouteTrackInteractive();

    class Settings : public ToolSettings {
    public:
        json serialize() const override;
        void load_from_json(const json &j) override;
        int effort = 1;
        bool remove_loops = true;
        bool fix_all_segments = false;
        enum class Mode { WALKAROUND, PUSH, BEND, STRAIGHT };
        Mode mode = Mode::WALKAROUND;
        static const std::map<Mode, std::string> mode_names;

        enum class CornerMode { MITERED_45, ROUNDED_45, MITERED_90, ROUNDED_90 };
        CornerMode corner_mode = CornerMode::MITERED_45;
        static const std::map<CornerMode, std::string> corner_mode_names;

        bool drc = true;
    };

    ToolSettings *get_settings() override
    {
        return &settings;
    }

    void apply_settings() override;

    std::set<InToolActionID> get_actions() const override
    {
        using I = InToolActionID;
        return {
                I::LMB,
                I::CANCEL,
                I::RMB,
                I::LENGTH_TUNING_LENGTH,
                I::LENGTH_TUNING_AMPLITUDE_INC,
                I::LENGTH_TUNING_AMPLITUDE_DEC,
                I::LENGTH_TUNING_SPACING_INC,
                I::LENGTH_TUNING_SPACING_DEC,
                I::POSTURE,
                I::TOGGLE_VIA,
                I::ROUTER_SETTINGS,
                I::ROUTER_MODE,
                I::ENTER_WIDTH,
                I::TRACK_WIDTH_DEFAULT,
                I::CLEARANCE_OFFSET,
                I::CLEARANCE_OFFSET_DEFAULT,
                I::DELETE_LAST_SEGMENT,
                I::TOGGLE_CORNER_STYLE,
        };
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

    void update_tip();
    class Track *get_track(const std::set<SelectableRef> &sel);
    class Via *get_via(const std::set<SelectableRef> &sel);
    bool is_tune() const;
    bool settings_window_visible = false;
    void update_settings_window();
};
} // namespace horizon
