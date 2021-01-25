#pragma once
#include "rules/rules.hpp"

namespace horizon {
RulesCheckResult check_item(class IPool &pool, ObjectType type, const class UUID &uu);
}
