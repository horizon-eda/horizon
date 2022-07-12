#pragma once
#include "util/uuid.hpp"
#include "nlohmann/json_fwd.hpp"
#include <memory>

namespace horizon {
using json = nlohmann::json;

class IncludedBoard {
public:
    IncludedBoard(const UUID &uu, const json &j, const std::string &parent_board_directory);
    IncludedBoard(const UUID &uu, const std::string &p, const std::string &parent_board_directory);
    IncludedBoard(const IncludedBoard &other);
    json serialize() const;
    UUID get_uuid() const;
    std::string get_name() const;
    void reload(const std::string &parent_board_directory);
    bool is_valid() const;

    UUID uuid;
    std::string project_filename;

    std::unique_ptr<class ProjectPool> pool;
    std::unique_ptr<class Block> block;
    std::unique_ptr<class Board> board;

    ~IncludedBoard();

private:
    void reset();
};
} // namespace horizon
