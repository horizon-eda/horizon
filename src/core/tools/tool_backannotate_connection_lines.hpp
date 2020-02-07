#pragma once
#include "core/tool.hpp"
#include "util/uuid_path.hpp"
#include "nlohmann/json_fwd.hpp"

namespace horizon {
using json = nlohmann::json;
class ToolBackannotateConnectionLines : public ToolBase {
public:
    ToolBackannotateConnectionLines(IDocument *c, ToolID tid);
    ToolResponse begin(const ToolArgs &args) override;
    ToolResponse update(const ToolArgs &args) override;
    bool can_begin() override;

    class ToolDataBackannotate : public ToolData {
    public:
        class Item {
        public:
            void from_json(class Block &block, const json &j);
            bool is_valid() const;
            class Net *net = nullptr;
            class Component *component = nullptr;
            UUIDPath<2> connpath;
        };
        std::vector<std::pair<Item, Item>> connections;
    };

private:
    Net *create_net_stub(class Component *comp, const UUIDPath<2> &connpath, Net *net = nullptr);
    int net_n = 0;
};
} // namespace horizon
