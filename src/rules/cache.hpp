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
    static const RulesCheckCacheID id = RulesCheckCacheID::BOARD_IMAGE;
    RulesCheckCacheBoardImage(class IDocument &c);
    const CanvasPatch &get_canvas() const;

private:
    CanvasPatch canvas;
};

class RulesCheckCacheNetPins : public RulesCheckCacheBase {
public:
    static const RulesCheckCacheID id = RulesCheckCacheID::NET_PINS;

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
    void clear();
    template <typename T> const T &get_cache()
    {
        return dynamic_cast<const T &>(get_cache(T::id));
    }

private:
    const RulesCheckCacheBase &get_cache(RulesCheckCacheID id);
    std::map<RulesCheckCacheID, std::unique_ptr<RulesCheckCacheBase>> cache;
    class IDocument &core;
    std::mutex mutex;
};
} // namespace horizon
