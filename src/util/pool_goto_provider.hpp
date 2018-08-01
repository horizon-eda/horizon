#pragma once
#include "util/uuid.hpp"
#include "common/common.hpp"
#include <sigc++/sigc++.h>

namespace horizon {
class PoolGotoProvider {
public:
    typedef sigc::signal<void, ObjectType, UUID> type_signal_goto;
    type_signal_goto signal_goto()
    {
        return s_signal_goto;
    }

protected:
    type_signal_goto s_signal_goto;
};
} // namespace horizon
