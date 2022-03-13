#include "padstack_hash.hpp"
#include "pool/padstack.hpp"

namespace horizon {
PadstackHash::PadstackHash() : checksum(Glib::Checksum::CHECKSUM_SHA256)
{
}
void PadstackHash::update(const Padstack &ps)
{
    for (const auto &it : ps.holes) {
        update(it.second);
    }
    for (const auto &it : ps.shapes) {
        update(it.second);
    }
    for (const auto &it : ps.polygons) {
        update(it.second);
    }
}

std::string PadstackHash::get_digest()
{
    return checksum.get_string();
}

std::string PadstackHash::hash(const Padstack &ps)
{
    PadstackHash h;
    h.update(ps);
    return h.get_digest();
}

void PadstackHash::update(const Hole &h)
{
    update(h.diameter);
    update(static_cast<int>(h.shape));
    if (h.shape == Hole::Shape::SLOT) {
        update(h.length);
    }
    update(h.plated);
    update(h.placement);
}

void PadstackHash::update(const Shape &s)
{
    update(static_cast<int>(s.form));
    update(s.placement);
    update(s.layer);
    for (const auto it : s.params) {
        update(it);
    }
}

void PadstackHash::update(const Polygon &p)
{
    update(p.layer);
    for (const auto &it : p.vertices) {
        update(it.position);
        update(static_cast<int>(it.type));
        if (it.type == Polygon::Vertex::Type::ARC)
            update(it.arc_center);
    }
}

void PadstackHash::update(int64_t i)
{
    checksum.update(reinterpret_cast<const guchar *>(&i), sizeof(i));
}

void PadstackHash::update(const Coordi &c)
{
    update(c.x);
    update(c.y);
}

void PadstackHash::update(const Placement &p)
{
    update(p.shift);
    update(p.get_angle());
    update(p.mirror);
}
} // namespace horizon
