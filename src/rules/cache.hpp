#pragma once
#include "canvas/canvas_patch.hpp"
#include "common/common.hpp"
#include "pool/entity.hpp"
#include "util/uuid.hpp"
#include "util/uuid_vec.hpp"
#include <vector>
#include <map>
#include <memory>
#include <mutex>

namespace horizon {
enum class RulesCheckCacheID { NONE, BOARD_IMAGE, NET_PINS };

class RulesCheckCacheBase {
public:
    virtual ~RulesCheckCacheBase()
    {
    }
};

class RulesCheckCacheBoardImage : public RulesCheckCacheBase {
public:
    RulesCheckCacheBoardImage(class IDocument &c);
    const CanvasPatch &get_canvas() const;

private:
    CanvasPatch canvas;
};

class RulesCheckCacheNetPins : public RulesCheckCacheBase {
public:
    RulesCheckCacheNetPins(class IDocument &c);
    struct NetPin {
        UUID comp;
        const class Gate &gate;
        const class Pin &pin;
        UUID sheet;
        UUIDVec instance_path;
        Coordi location;
    };
    struct NetInfo {
        std::string name;
        bool is_nc = false;
        std::vector<NetPin> pins;
    };
    using NetPins = std::map<UUID, NetInfo>;
    const NetPins &get_net_pins() const;

private:
    NetPins net_pins;
};

class RulesCheckCache {
public:
    RulesCheckCache(class IDocument &c);
    RulesCheckCacheBase &get_cache(RulesCheckCacheID id);
    void clear();

private:
    std::map<RulesCheckCacheID, std::unique_ptr<RulesCheckCacheBase>> cache;
    class IDocument &core;
    std::mutex mutex;
};
} // namespace horizon
