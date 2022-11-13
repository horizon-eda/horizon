#include "editor_base.hpp"
#include "util/file_version.hpp"

namespace horizon {
unsigned int PoolEditorBase::get_required_version() const
{
    return get_version().get_app();
}
} // namespace horizon
