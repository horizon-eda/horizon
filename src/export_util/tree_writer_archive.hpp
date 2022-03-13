#pragma once
#include "tree_writer.hpp"
#include <sstream>
#include <set>
#include <optional>
#include <archive.h>

namespace horizon {
class TreeWriterArchive : public TreeWriter {
public:
    enum class Type { TGZ, ZIP };

    TreeWriterArchive(const fs::path &filename, const fs::path &prefix, Type type);

    ~TreeWriterArchive();

private:
    std::ostream &create_file_internal(const fs::path &path) override;
    void close_file() override;
    void mkdir_recursive(const fs::path &path);

    const fs::path prefix;
    const Type type;
    std::optional<std::ostringstream> ostream;
    std::set<fs::path> created_files;
    std::set<fs::path> created_directories;

    archive *ar = nullptr;
    archive_entry *ar_entry = nullptr;
};
} // namespace horizon
