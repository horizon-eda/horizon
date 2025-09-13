#pragma once
#include "core/tool.hpp"
#include "tool_helper_merge.hpp"
#include "tool_helper_move.hpp"
#include <nlohmann/json.hpp>

namespace horizon {

class ToolPaste : public ToolHelperMove, public ToolHelperMerge {
public:
    ToolPaste(IDocument *c, ToolID tid);
    ToolResponse begin(const ToolArgs &args) override;
    ToolResponse update(const ToolArgs &args) override;
    bool can_begin() override;
    bool is_specific() override;

    std::set<InToolActionID> get_actions() const override
    {
        using I = InToolActionID;
        return {
                I::LMB, I::CANCEL, I::RMB, I::ROTATE, I::MIRROR, I::RESTRICT,
        };
    }

private:
    void fix_layer(int &la);
    Coordi shift;
    const class BoardPackage *target_pkg = nullptr;
    const class PastedPackage *ref_pkg = nullptr;
    json paste_data;
    class Picture *pic = nullptr;
    ToolResponse begin_paste(const json &j, const Coordi &cursor_pos);
    ToolResponse really_begin_paste(const json &j, const Coordi &cursor_pos);
    bool pool_update_pending = false;
    void transform(Coordi &c) const;
    void transform(class Placement &p, ObjectType type) const;
    void update_tip();
    std::set<UUID> nets;
    void update_airwires();
};
} // namespace horizon
