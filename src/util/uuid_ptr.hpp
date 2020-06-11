#pragma once
#include "uuid.hpp"
#include "uuid_provider.hpp"
#include <assert.h>
#include <map>
#include <type_traits>

namespace horizon {
template <typename T> class uuid_ptr {
private:
    typedef typename std::remove_const<T>::type T_without_const;

public:
    uuid_ptr() : ptr(nullptr)
    {
    }
    uuid_ptr(const UUID &uu) : ptr(nullptr), uuid(uu)
    {
    }
    uuid_ptr(T *p, const UUID &uu) : ptr(p), uuid(uu)
    {
    }
    uuid_ptr(T *p) : ptr(p), uuid(p ? p->get_uuid() : UUID())
    {
        /* static_assert(
                std::is_base_of<T, decltype(*p)>::value,
                "T must be a descendant of MyBase"
            );*/
    }
    T &operator*()
    {
#ifdef UUID_PTR_CHECK
        if (ptr) {
            assert(ptr->get_uuid() == uuid);
        }
#endif
        return *ptr;
    }

    T *operator->() const
    {
#ifdef UUID_PTR_CHECK
        if (ptr) {
            assert(ptr->get_uuid() == uuid);
        }
#endif
        return ptr;
    }

    operator T *() const
    {
#ifdef UUID_PTR_CHECK
        if (ptr) {
            assert(ptr->get_uuid() == uuid);
        }
#endif
        return ptr;
    }

    T *ptr;
    UUID uuid;
    void update(std::map<UUID, T> &map)
    {
        if (uuid) {
            if (map.count(uuid)) {
                ptr = &map.at(uuid);
            }
            else {
                ptr = nullptr;
            }
        }
    }
    void update(const std::map<UUID, T_without_const> &map)
    {
        if (uuid) {
            if (map.count(uuid)) {
                ptr = &map.at(uuid);
            }
            else {
                ptr = nullptr;
            }
        }
    }
};
} // namespace horizon
