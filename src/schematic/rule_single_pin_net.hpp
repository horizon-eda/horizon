#include "rules/rule.hpp"
#include "common/common.hpp"

namespace horizon {
class RuleSinglePinNet : public Rule {
public:
    static const auto id = RuleID::SINGLE_PIN_NET;
    RuleID get_id() const override
    {
        return id;
    }

    RuleSinglePinNet();
    RuleSinglePinNet(const json &j);
    json serialize() const override;

    std::string get_brief(const class Block *block = nullptr, class IPool *pool = nullptr) const override;

    bool include_unnamed = true;
};
} // namespace horizon
