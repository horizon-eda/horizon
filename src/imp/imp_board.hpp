#pragma once
#include "core/core_board.hpp"
#include "imp_layer.hpp"

namespace horizon {
class ImpBoard : public ImpLayer {
public:
    ImpBoard(const std::string &board_filename, const std::string &block_filename, const std::string &via_dir,
             const PoolParams &params);

    const std::map<int, Layer> &get_layers();
    void update_highlights() override;

protected:
    void construct() override;
    bool handle_broadcast(const json &j) override;
    void handle_maybe_drag() override;
    void update_action_sensitivity() override;
    void apply_preferences() override;

    ActionCatalogItem::Availability get_editor_type_for_action() const override
    {
        return ActionCatalogItem::AVAILABLE_IN_BOARD;
    };
    ObjectType get_editor_type() const override
    {
        return ObjectType::BOARD;
    }

    std::string get_hud_text(std::set<SelectableRef> &sel) override;
    std::pair<ActionID, ToolID> get_doubleclick_action(ObjectType type, const UUID &uu) override;

private:
    void canvas_update() override;
    void handle_selection_cross_probe();

    CoreBoard core_board;
    const std::string project_dir;

    class FabOutputWindow *fab_output_window = nullptr;
    class View3DWindow *view_3d_window = nullptr;
    class StepExportWindow *step_export_window = nullptr;
    class TuningWindow *tuning_window = nullptr;
    class PDFExportWindow *pdf_export_window;
    bool cross_probing_enabled = false;

    Coordf cursor_pos_drag_begin;
    Target target_drag_begin;

    void handle_drag();
    void handle_measure_tracks(const ActionConnection &a);

    class CanvasAnnotation *text_owner_annotation = nullptr;
    std::map<UUID, UUID> text_owners;
    void update_text_owners();
    void update_text_owner_annotation();

    void handle_select_more(const ActionConnection &conn);

    int get_schematic_pid();
};
} // namespace horizon
