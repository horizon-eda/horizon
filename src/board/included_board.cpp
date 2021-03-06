#include "included_board.hpp"
#include "nlohmann/json.hpp"
#include "project/project.hpp"
#include "board/board.hpp"
#include "pool/entity.hpp"
#include "pool/part.hpp"
#include "pool/symbol.hpp"
#include "block/block.hpp"
#include "pool/project_pool.hpp"

namespace horizon {
IncludedBoard::IncludedBoard(const UUID &uu, const Project &prj, const std::string &p) : uuid(uu), project_filename(p)
{
    reload();
}

IncludedBoard::~IncludedBoard()
{
}

IncludedBoard::IncludedBoard(const UUID &uu, const json &j)
    : IncludedBoard(uu, j.at("project_filename").get<std::string>())
{
}

IncludedBoard::IncludedBoard(const UUID &uu, const std::string &prj_filename)
    : IncludedBoard(uu, Project::new_from_file(prj_filename), prj_filename)
{
}

IncludedBoard::IncludedBoard(const IncludedBoard &other)
    : uuid(other.uuid), project_filename(other.project_filename),
      pool(std::make_unique<ProjectPool>(other.pool->get_base_path(), false)),
      block(std::make_unique<Block>(*other.block)), board(std::make_unique<Board>(*other.board))
{
    board->block = block.get();
    board->update_refs();
}

void IncludedBoard::reload()
{
    auto prj = Project::new_from_file(project_filename);
    reset();

    try {
        pool = std::make_unique<ProjectPool>(prj.pool_directory, false);
        block = std::make_unique<Block>(horizon::Block::new_from_file(prj.get_top_block().block_filename, *pool));
        board = std::make_unique<Board>(horizon::Board::new_from_file(prj.board_filename, *block, *pool));
        board->expand();
    }
    catch (...) {
        reset();
        throw;
    }
}

void IncludedBoard::reset()
{
    pool.reset();
    block.reset();
    board.reset();
}

bool IncludedBoard::is_valid() const
{
    return board != nullptr;
}

json IncludedBoard::serialize() const
{
    json j;
    j["project_filename"] = project_filename;
    return j;
}

std::string IncludedBoard::get_name() const
{
    if (!is_valid())
        return "invalid";
    else if (block->project_meta.count("project_title"))
        return block->project_meta.at("project_title");
    else
        return "unknown";
}

UUID IncludedBoard::get_uuid() const
{
    return uuid;
}
} // namespace horizon
