#pragma once
#include "imp.hpp"
#include "core/core_schematic.hpp"
#include "search/searcher_schematic.hpp"

namespace horizon {
class ImpSchematic : public ImpBase {

public:
    ImpSchematic(const CoreSchematic::Filenames &filenames, const PoolParams &params);

protected:
    void construct() override;
    bool handle_broadcast(const json &j) override;
    void handle_maybe_drag(bool ctrl) override;
    void update_action_sensitivity() override;
    void update_highlights() override;
    void clear_highlights() override;

    ActionCatalogItem::Availability get_editor_type_for_action() const override
    {
        return ActionCatalogItem::AVAILABLE_IN_SCHEMATIC;
    };

    std::string get_hud_text(std::set<SelectableRef> &sel) override;
    void search_center(const Searcher::SearchResult &res) override;
    ActionToolID get_doubleclick_action(ObjectType type, const UUID &uu) override;
    void expand_selection_for_property_panel(std::set<SelectableRef> &sel_extra,
                                             const std::set<SelectableRef> &sel) override;

    Searcher *get_searcher_ptr() override
    {
        return &searcher;
    }

    ToolID get_tool_for_drag_move(bool ctrl, const std::set<SelectableRef> &sel) const override;

    void update_monitor() override;

    std::map<ObjectType, ImpBase::SelectionFilterInfo> get_selection_filter_info() const override;

private:
    void canvas_update() override;
    CoreSchematic core_schematic;
    const std::string project_dir;
    SearcherSchematic searcher;

    int handle_ask_net_merge(class Net *net, class Net *into);
    int handle_ask_delete_component(class Component *comp);
    void handle_select_sheet(const UUID &sheet, const UUID &block, const UUIDVec &instance_path);
    void handle_core_rebuilt();
    void handle_tool_change(ToolID id) override;
    void handle_move_to_other_sheet(const ActionConnection &conn);
    void handle_highlight_group_tag(const ActionConnection &conn);
    void handle_next_prev_sheet(const ActionConnection &conn);
    const Entity *entity_from_selection(const std::set<SelectableRef> &sel);

    struct ViewInfo {
        float scale;
        Coordf offset;
        std::set<SelectableRef> selection;
    };
    std::map<std::pair<UUID, UUID>, ViewInfo> view_infos;
    class SheetBox *sheet_box;
    UUID current_sheet;
    void handle_selection_cross_probe() override;
    bool cross_probing_enabled = false;

    Coordf cursor_pos_drag_begin;
    Target target_drag_begin;

    class BOMExportWindow *bom_export_window;
    class PDFExportWindow *pdf_export_window;
    class UnplacedBox *unplaced_box = nullptr;
    void update_unplaced();

    void handle_drag();

    void handle_extra_button(const GdkEventButton *button_event) override;

    Glib::RefPtr<Gio::SimpleAction> toggle_snap_to_targets_action;

    int get_board_pid();

    UUID net_from_selectable(const SelectableRef &sr);

    std::vector<class ActionButton *> action_buttons_schematic;
    ActionButton &add_action_button_schematic(ActionToolID id);

    struct HighlightItem {
        ObjectType type;
        UUID uuid;
        UUIDVec instance_path;
    };

    std::vector<HighlightItem> highlights_hierarchical;

    void update_instance_path_bar();
    UUIDVec instance_path_for_bar;

    const Block &get_block_for_group_tag_names() override;

    void handle_show_in_pool_manager(const ActionConnection &conn);
};
} // namespace horizon
