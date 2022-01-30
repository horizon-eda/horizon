#pragma once
#include <vector>
#include <cstddef>
#include <cstdint>

namespace horizon {
namespace detail {
template <typename Parameter> class NamedIndex {
public:
    explicit NamedIndex(size_t value) : value_(value)
    {
    }

    NamedIndex() : value_(SIZE_MAX)
    {
    }

    bool has_value() const
    {
        return value_ != SIZE_MAX;
    }

    size_t get() const
    {
        return value_;
    }

    size_t operator++(int)
    {
        return value_++;
    }

    auto operator+(size_t other) const
    {
        return NamedIndex<Parameter>{get() + other};
    }

    bool operator<(NamedIndex<Parameter> other) const
    {
        return get() < other.get();
    }

    bool operator==(NamedIndex<Parameter> other) const
    {
        return get() == other.get();
    }

private:
    size_t value_;
};
} // namespace detail

template <typename T, typename Name> class NamedVector {
public:
    NamedVector() = default;
    using vector_type = std::vector<T>;
    using index_type = detail::NamedIndex<Name>;

    static index_type make_index(typename vector_type::size_type i)
    {
        return index_type{i};
    }

    typename vector_type::reference at(index_type i)
    {
        return vec.at(i.get());
    }

    typename vector_type::const_reference at(index_type i) const
    {
        return vec.at(i.get());
    }

    void push_back(const T &value)
    {
        vec.push_back(value);
    }

    template <class... Args> void emplace_back(Args &&... args)
    {
        vec.emplace_back(std::forward<Args>(args)...);
    }

    auto size() const
    {
        return make_index(vec.size());
    }

    auto begin()
    {
        return vec.begin();
    }

    auto end()
    {
        return vec.end();
    }


private:
    vector_type vec;
};
} // namespace horizon
