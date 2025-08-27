#include "included_board.hpp"
#include <nlohmann/json.hpp>
#include "project/project.hpp"
#include "board/board.hpp"
#include "pool/entity.hpp"
#include "pool/part.hpp"
#include "pool/symbol.hpp"
#include "block/block.hpp"
#include "blocks/blocks.hpp"
#include "pool/project_pool.hpp"
#include "logger/logger.hpp"
#include <filesystem>

namespace horizon {

namespace fs = std::filesystem;

IncludedBoard::IncludedBoard(const UUID &uu, const std::string &p, const std::string &parent_board_directory)
    : uuid(uu), project_filename(p)
{
    try {
        reload(parent_board_directory);
    }
    catch (const std::exception &e) {
        Logger::log_warning("error loading included board", Logger::Domain::BOARD, e.what());
    }
    catch (...) {
        Logger::log_warning("error loading included board", Logger::Domain::BOARD);
    }
}

IncludedBoard::~IncludedBoard()
{
}

IncludedBoard::IncludedBoard(const UUID &uu, const json &j, const std::string &parent_board_directory)
    : IncludedBoard(uu, j.at("project_filename").get<std::string>(), parent_board_directory)
{
}

IncludedBoard::IncludedBoard(const IncludedBoard &other)
    : uuid(other.uuid), project_filename(other.project_filename),
      pool(other.is_valid() ? std::make_unique<ProjectPool>(other.pool->get_base_path(), false) : nullptr),
      block(other.is_valid() ? std::make_unique<Block>(*other.block) : nullptr),
      board(other.is_valid() ? std::make_unique<Board>(*other.board) : nullptr)
{
    if (is_valid()) {
        board->block = block.get();
        board->update_refs();
    }
}

std::string IncludedBoard::get_absolute_project_filename(const std::string &parent_board_directory) const
{
    auto project_path = fs::u8path(project_filename);
    if (project_path.is_relative())
        project_path = fs::weakly_canonical(fs::u8path(parent_board_directory) / project_path);
    return project_path.u8string();
}

void IncludedBoard::reload(const std::string &parent_board_directory)
{
    auto prj = Project::new_from_file(get_absolute_project_filename(parent_board_directory));
    reset();

    try {
        pool = std::make_unique<ProjectPool>(prj.pool_directory, false);
        auto blocks = Blocks::new_from_file(prj.blocks_filename, *pool);
        block = std::make_unique<Block>(blocks.get_top_block_item().block.flatten());
        board = std::make_unique<Board>(horizon::Board::new_from_file(prj.board_filename, *block, *pool));
        if (fs::is_regular_file(fs::u8path(prj.planes_filename)))
            board->load_planes_from_file(prj.planes_filename);
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
