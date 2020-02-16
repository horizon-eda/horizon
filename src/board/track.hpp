#pragma once
#include "block/net.hpp"
#include "board_package.hpp"
#include "common/common.hpp"
#include "common/junction.hpp"
#include "nlohmann/json_fwd.hpp"
#include "util/uuid.hpp"
#include "util/uuid_provider.hpp"
#include "util/uuid_ptr.hpp"
#include <fstream>
#include <map>
#include <vector>

namespace horizon {
using json = nlohmann::json;

class Track : public UUIDProvider {
public:
    Track(const UUID &uu, const json &j, class Board *brd = nullptr);
    Track(const UUID &uu);

    void update_refs(class Board &brd);
    virtual UUID get_uuid() const;
    bool coord_on_line(const Coordi &coord) const;

    UUID uuid;
    uuid_ptr<Net> net = nullptr;
    int layer = 0;
    uint64_t width = 0;
    bool width_from_rules = true;

    bool locked = false;

    class Connection {
    public:
        Connection()
        {
        }
        Connection(const json &j, Board *brd = nullptr);
        Connection(Junction *j);
        Connection(BoardPackage *pkg, Pad *pad);
        uuid_ptr<Junction> junc = nullptr;
        uuid_ptr<BoardPackage> package = nullptr;
        uuid_ptr<Pad> pad = nullptr;
        bool operator<(const Track::Connection &other) const;
        bool operator==(const Track::Connection &other) const;

        void connect(Junction *j);
        void connect(BoardPackage *pkg, Pad *pad);
        UUIDPath<2> get_pad_path() const;
        bool is_junc() const;
        bool is_pad() const;
        UUID get_net_segment() const;
        void update_refs(class Board &brd);
        Coordi get_position() const;
        int get_layer() const;
        Net *get_net();

        json serialize() const;
    };

    Connection from;
    Connection to;

    json serialize() const;
};
} // namespace horizon
