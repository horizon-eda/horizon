#include "util/uuid.hpp"

namespace horizon {
class InstallationUUID {
public:
    static UUID get();

private:
    InstallationUUID();

    UUID uuid;
};
} // namespace horizon
