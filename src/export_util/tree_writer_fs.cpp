#include "tree_writer_fs.hpp"

namespace horizon {

TreeWriterFS::TreeWriterFS(const fs::path &path) : base_path(path)
{
}

std::ostream &TreeWriterFS::create_file_internal(const fs::path &path)
{
    if (created_files.count(path))
        throw std::runtime_error(path.generic_u8string() + " already exists");

    if (ofstream.is_open())
        throw std::runtime_error("file is already open");

    const auto abs = base_path / path;
    fs::create_directories(abs.parent_path());
    ofstream.open(abs, std::ios_base::out | std::ios_base::binary);
    ofstream.imbue(std::locale::classic());
    if (!ofstream.is_open())
        throw std::runtime_error(abs.u8string() + " not opened");

    created_files.insert(path);

    return ofstream;
}

void TreeWriterFS::close_file()
{
    if (!ofstream.is_open())
        throw std::runtime_error("no open file");
    ofstream.close();
}

} // namespace horizon
