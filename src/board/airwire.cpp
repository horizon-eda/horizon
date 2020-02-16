#include "airwire.hpp"

namespace horizon {
void Airwire::update_refs(Board &brd)
{
    to.update_refs(brd);
    from.update_refs(brd);
}
} // namespace horizon
