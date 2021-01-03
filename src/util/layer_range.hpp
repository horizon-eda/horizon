#pragma once
#include <algorithm>

namespace horizon {

// adapted from KiCad's LAYER_RANGE
class LayerRange {
public:
    LayerRange() : m_start(10000), m_end(10000){};

    LayerRange(int aStart, int aEnd)
    {
        if (aStart > aEnd)
            std::swap(aStart, aEnd);

        m_start = aStart;
        m_end = aEnd;
    }

    LayerRange(int aLayer)
    {
        m_start = m_end = aLayer;
    }

    LayerRange(const LayerRange &aB) : m_start(aB.m_start), m_end(aB.m_end)
    {
    }

    LayerRange &operator=(const LayerRange &aB)
    {
        m_start = aB.m_start;
        m_end = aB.m_end;
        return *this;
    }

    bool overlaps(const LayerRange &aOther) const
    {
        return m_end >= aOther.m_start && m_start <= aOther.m_end;
    }

    bool overlaps(const int aLayer) const
    {
        return aLayer >= m_start && aLayer <= m_end;
    }

    bool is_multilayer() const
    {
        return m_start != m_end;
    }

    int start() const
    {
        return m_start;
    }

    int end() const
    {
        return m_end;
    }

    void merge(const LayerRange &aOther)
    {
        if (m_start == 10000 || m_end == 10000) {
            m_start = aOther.m_start;
            m_end = aOther.m_end;
            return;
        }

        if (aOther.m_start < m_start)
            m_start = aOther.m_start;

        if (aOther.m_end > m_end)
            m_end = aOther.m_end;
    }

    bool operator==(const LayerRange &aOther) const
    {
        return (m_start == aOther.m_start) && (m_end == aOther.m_end);
    }

    bool operator!=(const LayerRange &aOther) const
    {
        return (m_start != aOther.m_start) || (m_end != aOther.m_end);
    }

    bool operator<(const LayerRange &aOther) const
    {
        return std::make_pair(m_start, m_end) < std::make_pair(aOther.m_start, aOther.m_end);
    }

private:
    int m_start;
    int m_end;
};
} // namespace horizon
