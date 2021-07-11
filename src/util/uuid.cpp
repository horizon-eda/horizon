#include "uuid.hpp"
#include <string.h>
#include <glibmm/checksum.h>
#include <array>

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

UUID UUID::UUID5(const UUID &nsid, const unsigned char *name, size_t name_size)
{
    UUID uu;
    Glib::Checksum chk(Glib::Checksum::CHECKSUM_SHA1);
    chk.update(nsid.uu, sizeof(nsid.uu));
    chk.update(name, name_size);
    std::array<unsigned char, 20> buf;
    gsize length = buf.size();
    chk.get_digest(buf.data(), &length);
    memcpy(uu.uu, buf.data(), sizeof(uu.uu));
    // set version 5 (sha1)
    uu.uu[6] &= ~0xf0;
    uu.uu[6] |= 0x50;
    // set variant to RFC 4122
    uu.uu[8] &= ~0xc0;
    uu.uu[8] |= 0x80;
    return uu;
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
