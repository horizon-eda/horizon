#pragma once
#include "tree_writer.hpp"
#include <fstream>
#include <set>

namespace horizon {
class TreeWriterFS : public TreeWriter {
public:
    TreeWriterFS(const fs::path &base);


private:
    std::ostream &create_file_internal(const fs::path &path) override;
    void close_file() override;

    const fs::path base_path;
    std::ofstream ofstream;
    std::set<fs::path> created_files;
};
} // namespace horizon
