#pragma once
#include "common/common.hpp"
#include <set>
#include <string>

namespace horizon {
class DXFImporter {
public:
    DXFImporter(class IDocument *c);
    bool import(const std::string &filename);
    void set_layer(int la);
    void set_width(uint64_t w);
    void set_shift(const Coordi &sh);
    void set_scale(double sc);

    std::set<class Junction *> junctions;
    std::set<class Line *> lines;
    std::set<class Arc *> arcs;

    enum class UnsupportedType { CIRCLE, ELLIPSE, SPLINE };

    const std::map<UnsupportedType, unsigned int> &get_items_unsupported() const;

private:
    class IDocument *core = nullptr;
    int layer = 0;
    uint64_t width = 0;
    Coordi shift;
    double scale = 1;

    std::map<UnsupportedType, unsigned int> items_unsupported;
};
} // namespace horizon
