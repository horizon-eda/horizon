#pragma once
#include "core/tool.hpp"

namespace horizon {

class ToolPastePlacement : public ToolBase {
public:
    using ToolBase::ToolBase;
    ToolResponse begin(const ToolArgs &args) override;
    ToolResponse update(const ToolArgs &args) override;
    bool can_begin() override;
    bool is_specific() override
    {
        return true;
    }

    std::set<InToolActionID> get_actions() const override
    {
        using I = InToolActionID;
        return {I::LMB, I::CANCEL, I::RMB};
    }

private:
    ToolResponse begin_paste(const json &j);
    std::set<UUID> nets;
    class BoardPackage *target_pkg = nullptr;
    void update_airwires();
    void copy_package_silkscreen_texts(BoardPackage &dest, const class PackageInfo &src, const json &j);
};
} // namespace horizon
