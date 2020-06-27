#pragma once
#ifdef WIN32_UUID
#include "uuid_win32.hpp"
#else
#include <uuid/uuid.h>
#endif

#include <iostream>

namespace horizon {
/**
 *  This class encapsulates a %UUID and allows it to be uses as a value type.
 *  It uses uuid.h from libutil or the %UUID function from the win32 rpc api (
 * see util/uuid_win32.hpp )
 */
class UUID {
public:
    UUID();
    static UUID random();
    UUID(const char *str);
    UUID(const std::string &str);
    operator std::string() const
    {
        char str[40];
        uuid_unparse(uu, str);
        return std::string(str);
    }
    /**
     *  @return true if uuid is non-null, false otherwise
     */
    operator bool() const;

    friend bool operator==(const UUID &self, const UUID &other);
    friend bool operator!=(const UUID &self, const UUID &other);
    friend bool operator<(const UUID &self, const UUID &other);
    friend bool operator>(const UUID &self, const UUID &other);
    size_t hash() const
    {
        size_t r = 0;
        for (size_t i = 0; i < 16; i++) {
            r ^= uu[i] << ((i % sizeof(size_t)) * 8);
        }
        return r;
    }

private:
    uuid_t uu;
};
} // namespace horizon

namespace std {
template <> struct hash<horizon::UUID> {
    std::size_t operator()(const horizon::UUID &k) const
    {
        return k.hash();
    }
};
} // namespace std
