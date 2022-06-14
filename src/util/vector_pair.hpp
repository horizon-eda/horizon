#pragma once
#include <vector>
#include <tuple>

namespace horizon {


template <typename T1, typename T2> class vector_pair {
    friend class iterator;

private:
    std::vector<T1> m_first;
    std::vector<T2> m_second;

public:
    vector_pair() : first(m_first), second(m_second)
    {
    }

    void clear()
    {
        m_first.clear();
        m_second.clear();
    }

    void reserve(size_t sz)
    {
        m_first.reserve(sz);
        m_second.reserve(sz);
    }


    class iterator {
        friend class vector_pair;

        iterator(vector_pair &p, size_t i) : m_parent(p), m_idx(i)
        {
        }

        class vector_pair &m_parent;
        size_t m_idx = 0;

    public:
        void operator++()
        {
            m_idx++;
        }

        bool operator!=(const iterator &other) const
        {
            return m_idx != other.m_idx;
        }

        std::pair<T1 &, T2 &> operator*()
        {
            return {m_parent.m_first.at(m_idx), m_parent.m_second.at(m_idx)};
        }
    };

    class const_iterator {
        friend class vector_pair;

        const_iterator(const vector_pair &p, size_t i) : m_parent(p), m_idx(i)
        {
        }

        const class vector_pair &m_parent;
        size_t m_idx = 0;

    public:
        void operator++()
        {
            m_idx++;
        }

        bool operator!=(const const_iterator &other) const
        {
            return m_idx != other.m_idx;
        }

        std::pair<const T1 &, const T2 &> operator*()
        {
            return {m_parent.m_first.at(m_idx), m_parent.m_second.at(m_idx)};
        }
    };

    auto begin()
    {
        return iterator(*this, 0);
    }

    auto end()
    {
        return iterator(*this, m_first.size());
    }

    auto begin() const
    {
        return const_iterator(*this, 0);
    }

    auto end() const
    {
        return const_iterator(*this, m_first.size());
    }


    template <typename... Args1, typename... Args2>
    void emplace_back(std::tuple<Args1...> args1, std::tuple<Args2...> args2)
    {
        std::apply([this](auto &&...args) { m_first.emplace_back(args...); }, args1);
        std::apply([this](auto &&...args) { m_second.emplace_back(args...); }, args2);
    }

    std::pair<T1 &, T2 &> atm(size_t idx)
    {
        return {m_first.at(idx), m_second.at(idx)};
    }

    std::pair<const T1 &, const T2 &> at(size_t idx) const
    {
        return {m_first.at(idx), m_second.at(idx)};
    }

    size_t size() const
    {
        return m_first.size();
    }

    void erase(size_t i_first, size_t i_last)
    {
        m_first.erase(m_first.begin() + i_first, m_first.begin() + i_last);
        m_second.erase(m_second.begin() + i_first, m_second.begin() + i_last);
    }

    const std::vector<T1> &first;
    const std::vector<T2> &second;
};

} // namespace horizon
