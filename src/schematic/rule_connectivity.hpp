#include "rules/rule.hpp"
#include "common/common.hpp"

namespace horizon {
class RuleConnectivity : public Rule {
public:
    static const auto id = RuleID::CONNECTIVITY;
    RuleID get_id() const override
    {
        return id;
    }

    RuleConnectivity();
    RuleConnectivity(const json &j);
    json serialize() const override;

    std::string get_brief(const class Block *block = nullptr, class IPool *pool = nullptr) const override;

    bool include_unnamed = true;
};
} // namespace horizon
