#pragma once
#include "core/tool.hpp"
#include "tool_place_junction.hpp"
#include "board/board_junction.hpp"
#include <forward_list>

namespace horizon {

class ToolPlaceVia : public ToolPlaceJunctionT<class BoardJunction> {
public:
    using ToolPlaceJunctionT<BoardJunction>::ToolPlaceJunctionT;
    bool can_begin() override;
    std::set<InToolActionID> get_actions() const override
    {
        using I = InToolActionID;
        return {
                I::LMB, I::CANCEL, I::RMB, I::EDIT, I::SELECT_VIA_DEFINITION,
        };
    }

protected:
    void insert_junction() override;
    void create_attached() override;
    void delete_attached() override;
    bool begin_attached() override;
    void finish() override;
    bool update_attached(const ToolArgs &args) override;
    class Via *via = nullptr;
    class Net *net = nullptr;
    std::set<UUID> nets;

    std::forward_list<class Via *> vias_placed;
    const class ViaDefinition *via_definition = nullptr;

private:
    const class BoardRules *rules = nullptr;
    void update_tip();
    void update_via();
};
} // namespace horizon
