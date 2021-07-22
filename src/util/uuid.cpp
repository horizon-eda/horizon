#include "uuid.hpp"
#include <string.h>
#include <stdexcept>

namespace horizon {

UUID::UUID()
{
    static_assert(sizeof(UUID::uu) == 16, "UUID size must be 16");
    static_assert(sizeof(UUID::uu[0]) == 1, "UUID element size must be 1");

    memset(uu, 0, sizeof(uu));
}

UUID UUID::random()
{
    UUID uu;
    uuid_generate_random(uu.uu);
    return uu;
}

UUID::UUID(const char *str)
{
    if (uuid_parse(str, uu)) {
        throw std::domain_error("invalid UUID");
    }
}

UUID::UUID(const std::string &str)
{
    if (uuid_parse(str.data(), uu)) {
        throw std::domain_error("invalid UUID");
    }
}

bool operator==(const UUID &self, const UUID &other)
{
    return memcmp(self.uu, other.uu, sizeof(self.uu)) == 0;
}

bool operator!=(const UUID &self, const UUID &other)
{
    return !(self == other);
}

bool operator<(const UUID &self, const UUID &other)
{
    return memcmp(self.uu, other.uu, sizeof(self.uu)) < 0;
}

bool operator>(const UUID &self, const UUID &other)
{
    return other < self;
}

UUID::operator bool() const
{
    static const uuid_t null_uuid = {0};
    return memcmp(uu, null_uuid, sizeof(uu)) != 0;
}
}; // namespace horizon
