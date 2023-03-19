#include "canvas_odb.hpp"
#include "odb_export.hpp"
#include "common/keepout.hpp"
#include "board/plane.hpp"
#include "board/board_layers.hpp"
#include "util/clipper_util.hpp"
#include "util/geom_util.hpp"
#include "pool/padstack.hpp"
#include "db.hpp"
#include "util/once.hpp"
#include "common/arc.hpp"
#include "board/board.hpp"
#include "odb_util.hpp"

namespace horizon {
CanvasODB::CanvasODB(ODB::Job &j, const Board &b) : Canvas::Canvas(), job(j), brd(b)
{
    img_mode = true;
}
void CanvasODB::request_push()
{
}

void CanvasODB::img_net(const Net *n)
{
}

void CanvasODB::img_arc(const Coordi &from, const Coordi &to, const Coordi &center, const uint64_t width, int layer)
{
    if (auto feats = get_layer_features(layer)) {
        using Dir = ODB::Features::Arc::Direction;
        feats->draw_arc(transform.transform(from), transform.transform(to), transform.transform(center), width,
                        transform.mirror ? Dir::CW : Dir::CCW);
    }
}

void CanvasODB::img_polygon(const Polygon &ipoly, bool tr)
{
    if (!tr)
        throw std::runtime_error("can't not transform polygon");
    if (padstack_mode)
        return;

    if (ipoly.layer == BoardLayers::TOP_ASSEMBLY || ipoly.layer == BoardLayers::BOTTOM_ASSEMBLY) {
        if (auto feats = get_layer_features(ipoly.layer)) // on assembly layer, draw polygons as lines
            feats->draw_polygon_outline(ipoly, transform);

        return;
    }

    if (auto plane = dynamic_cast<const Plane *>(ipoly.usage.ptr)) {
        if (auto feats = get_layer_features(ipoly.layer)) {
            ODB::EDAData::Subnet *subnet = nullptr;
            if (plane->fragments.size()) {
                using SP = ODB::EDAData::SubnetPlane;
                subnet = &eda_data->get_net(plane->net->uuid)
                                  .add_subnet<SP>(SP::FillType::SOLID, SP::CutoutType::CIRCLE, 0);
            }
            for (const auto &frag : plane->fragments) {
                auto &surf = feats->add_surface();
                eda_data->add_feature_id(*subnet, ODB::EDAData::FeatureID::Type::COPPER,
                                         ODB::get_layer_name(plane->polygon->layer), surf.index);

                Once is_outline;
                for (const auto &path : frag.paths) {
                    // check orientations
                    assert(ClipperLib::Orientation(path) == is_outline());
                    surf.data.lines.emplace_back();
                    auto &p = surf.data.lines.back();
                    p.reserve(path.size());
                    assert(transform.mirror == false);
                    for (auto it = path.crbegin(); it != path.crend(); it++) {
                        p.emplace_back(transform.transform(Coordi(it->X, it->Y)));
                    }
                }
            }
        }
    }
    else if (dynamic_cast<const Keepout *>(ipoly.usage.ptr)) {
        // nop
    }
    else {
        if (auto feats = get_layer_features(ipoly.layer)) {
            auto poly = ipoly;
            if (poly.is_ccw() != transform.mirror)
                poly.reverse();

            auto &surf = feats->add_surface();
            surf.data.append_polygon(poly, transform);
        }
    }
}

void CanvasODB::img_line(const Coordi &p0, const Coordi &p1, const uint64_t width, int layer, bool tr)
{
    if (auto feats = get_layer_features(layer)) {
        ODB::Features::Line *feat = nullptr;
        if (tr)
            feat = &feats->draw_line(transform.transform(p0), transform.transform(p1), width);
        else
            feat = &feats->draw_line(p0, p1, width);

        if (object_refs_current.size()) {
            const auto &ref = object_refs_current.back();
            if (ref.type == ObjectType::TRACK) {
                if (track_subnets.count(ref.uuid))
                    eda_data->add_feature_id(*track_subnets.at(ref.uuid), ODB::EDAData::FeatureID::Type::COPPER,
                                             ODB::get_layer_name(layer), feat->index);
            }
        }
        if (text_current) {
            auto &text = *text_current;
            feats->add_attribute(*feat, ODB::attribute::string{text.overridden ? text.text_override : text.text});
        }
    }
}

ODB::EDAData::SubnetToeprint *CanvasODB::get_subnet_toeprint()
{
    if (object_refs_current.size()) {
        const auto &ref = object_refs_current.back();
        if (ref.type == ObjectType::PAD) {
            const auto &pkg_uuid = ref.uuid2;
            const auto &pad_uuid = ref.uuid;
            const auto key = std::make_pair(pkg_uuid, pad_uuid);
            if (pad_subnets.count(key))
                return pad_subnets.at(key);
        }
    }
    return nullptr;
}

void CanvasODB::img_padstack(const Padstack &padstack)
{
    std::set<int> layers;
    for (const auto &it : padstack.polygons) {
        layers.insert(it.second.layer);
    }
    for (const auto &it : padstack.shapes) {
        layers.insert(it.second.layer);
    }
    ODB::EDAData::SubnetVia *subnet_via = nullptr;
    ODB::EDAData::SubnetToeprint *subnet_toep = get_subnet_toeprint();
    if (object_refs_current.size()) {
        const auto &ref = object_refs_current.back();
        if (ref.type == ObjectType::VIA) {
            auto &via = brd.vias.at(ref.uuid);
            if (via.junction->net) {
                auto &n = eda_data->get_net(via.junction->net->uuid);
                auto &subnet = n.add_subnet<ODB::EDAData::SubnetVia>();
                via_subnets.emplace(via.uuid, &subnet);
                subnet_via = &subnet;
            }
        }
    }
    for (const auto layer : layers) {
        if (auto feats = get_layer_features(layer)) {
            auto sym = job.get_or_create_symbol(padstack, layer);
            auto &pad = feats->draw_pad(sym, transform);
            switch (patch_type) {
            case PatchType::VIA: {
                feats->add_attribute(pad, ODB::attribute::pad_usage::VIA);
                if (subnet_via) {
                    eda_data->add_feature_id(*subnet_via, ODB::EDAData::FeatureID::Type::COPPER,
                                             ODB::get_layer_name(layer), pad.index);
                }
            } break;

            case PatchType::PAD:
                feats->add_attribute(pad, ODB::attribute::pad_usage::TOEPRINT);
                if (layer == BoardLayers::TOP_COPPER || layer == BoardLayers::BOTTOM_COPPER)
                    feats->add_attribute(pad, ODB::attribute::smd{});
                if (subnet_toep)
                    eda_data->add_feature_id(*subnet_toep, ODB::EDAData::FeatureID::Type::COPPER,
                                             ODB::get_layer_name(layer), pad.index);
                break;

            case PatchType::PAD_TH:
                feats->add_attribute(pad, ODB::attribute::pad_usage::TOEPRINT);
                if (subnet_toep)
                    eda_data->add_feature_id(*subnet_toep, ODB::EDAData::FeatureID::Type::COPPER,
                                             ODB::get_layer_name(layer), pad.index);
                break;

            default:;
            }
        }
    }
}

void CanvasODB::img_set_padstack(bool v)
{
    padstack_mode = v;
}

void CanvasODB::img_patch_type(PatchType pt)
{
    patch_type = pt;
}

void CanvasODB::img_text(const Text *text)
{
    text_current = text;
}

void CanvasODB::img_hole(const Hole &hole)
{
    auto &feats = *drill_features.at(hole.span);
    if (hole.shape == Hole::Shape::ROUND) {
        auto &pad = feats.draw_circle(transform.transform(hole.placement.shift), hole.diameter);
        if (patch_type == PatchType::VIA) {
            if (object_refs_current.size()) {
                const auto &ref = object_refs_current.back();
                if (ref.type == ObjectType::VIA) {
                    auto &subnet = *via_subnets.at(ref.uuid);
                    eda_data->add_feature_id(subnet, ODB::EDAData::FeatureID::Type::HOLE,
                                             ODB::get_drills_layer_name(hole.span), pad.index);
                }
            }
            feats.add_attribute(pad, ODB::attribute::drill::VIA);
        }
        else if (hole.plated) {
            feats.add_attribute(pad, ODB::attribute::drill::PLATED);
            if (auto subnet_toep = get_subnet_toeprint())
                eda_data->add_feature_id(*subnet_toep, ODB::EDAData::FeatureID::Type::COPPER,
                                         ODB::get_drills_layer_name(hole.span), pad.index);
        }
        else {
            feats.add_attribute(pad, ODB::attribute::drill::NON_PLATED);
        }
    }
    else if (hole.shape == Hole::Shape::SLOT) {
        auto tr = transform;
        tr.accumulate(hole.placement);
        if (tr.mirror)
            tr.invert_angle();

        double d = std::max(((int64_t)hole.length - (int64_t)hole.diameter) / 2, (int64_t)0);
        const auto dv = Coordd::euler(d, angle_to_rad(tr.get_angle())).to_coordi();

        auto &line = feats.draw_line(tr.shift - dv, tr.shift + dv, hole.diameter);
        if (hole.plated) {
            feats.add_attribute(line, ODB::attribute::drill::PLATED);
            if (auto subnet_toep = get_subnet_toeprint())
                eda_data->add_feature_id(*subnet_toep, ODB::EDAData::FeatureID::Type::HOLE,
                                         ODB::get_drills_layer_name(hole.span), line.index);
        }
        else {
            feats.add_attribute(line, ODB::attribute::drill::NON_PLATED);
        }
    }
}
} // namespace horizon
