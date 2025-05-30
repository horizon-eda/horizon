#include "airwire.hpp"

namespace horizon {
void Airwire::update_refs(Board &brd)
{
    to.update_refs(brd);
    from.update_refs(brd);
}

LayerRange Airwire::get_layers() const
{
    auto l = to.get_layer();
    l.merge(from.get_layer());
    return l;
}

} // namespace horizon
