#include "cache.hpp"
#include "document/idocument_board.hpp"
#include "document/idocument_schematic.hpp"
#include "schematic/schematic.hpp"

namespace horizon {
RulesCheckCache::RulesCheckCache(IDocument &c) : core(c)
{
}

void RulesCheckCache::clear()
{
    cache.clear();
}

RulesCheckCacheBoardImage::RulesCheckCacheBoardImage(IDocument &c)
{
    auto &core = dynamic_cast<IDocumentBoard &>(c);
    canvas.update(*core.get_board(), Canvas::PanelMode::SKIP);
}

const CanvasPatch &RulesCheckCacheBoardImage::get_canvas() const
{
    return canvas;
}

RulesCheckCacheNetPins::RulesCheckCacheNetPins(IDocument &c)
{
    auto &core = dynamic_cast<IDocumentSchematic &>(c);
    const auto block = core.get_top_block()->flatten();
    for (auto &[uu, net] : block.nets) {
        net_pins[uu].name = net.name;
        net_pins[uu].is_nc = net.is_nc;
    }
    for (auto &[comp_uu, comp] : block.components) {
        for (auto &[connpath, conn] : comp.connections) {
            if (conn.net) {
                auto gate = &comp.entity->gates.at(connpath.at(0));
                auto pin = &gate->unit->pins.at(connpath.at(1));
                UUID sheet_uuid;
                UUIDVec instance_path;
                Coordi location;
                for (const auto &it : core.get_top_schematic()->get_all_sheets()) {
                    bool found = false;
                    for (const auto &[uu_sym, sym] : it.sheet.symbols) {
                        const auto sym_comp_uu =
                                uuid_vec_flatten(uuid_vec_append(it.instance_path, sym.component->uuid));
                        if (sym_comp_uu == comp_uu && sym.gate == gate) {
                            found = true;
                            sheet_uuid = it.sheet.uuid;
                            instance_path = it.instance_path;
                            location = sym.placement.transform(sym.pool_symbol->pins.at(pin->uuid).position);
                            break;
                        }
                    }
                    if (found)
                        break;
                }
                net_pins.at(conn.net->uuid)
                        .pins.push_back({comp.href.back(), *gate, *pin, sheet_uuid, instance_path, location});
            }
        }
    }
}

const RulesCheckCacheNetPins::NetPins &RulesCheckCacheNetPins::get_net_pins() const
{
    return net_pins;
}

const RulesCheckCacheBase &RulesCheckCache::get_cache(RulesCheckCacheID id)
{
    std::lock_guard<std::mutex> guard(mutex);
    if (!cache.count(id)) {
        switch (id) {
        case RulesCheckCacheID::NONE:
            break;
        case RulesCheckCacheID::BOARD_IMAGE:
            cache.emplace(id, std::make_unique<RulesCheckCacheBoardImage>(core));
            break;
        case RulesCheckCacheID::NET_PINS:
            cache.emplace(id, std::make_unique<RulesCheckCacheNetPins>(core));
            break;
        }
    }
    return *cache.at(id);
}
} // namespace horizon
