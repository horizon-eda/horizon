#include "tree_writer_archive.hpp"
#include "archive_entry.h"

namespace horizon {

[[maybe_unused]] static int open_archive(archive *ar, const char *filename)
{
    return archive_write_open_filename(ar, filename);
}

[[maybe_unused]] static int open_archive(archive *ar, const wchar_t *filename)
{
    return archive_write_open_filename_w(ar, filename);
}

TreeWriterArchive::TreeWriterArchive(const fs::path &filename, Type ty) : type(ty)
{
    ar = archive_write_new();
    if (!ar)
        throw std::runtime_error("archive is null");

    switch (type) {
    case Type::TGZ:
        if (archive_write_add_filter_gzip(ar) != ARCHIVE_OK)
            throw std::runtime_error("archive_write_add_filter_gzip failed");

        if (archive_write_set_format_ustar(ar) != ARCHIVE_OK)
            throw std::runtime_error("archive_write_set_format_ustar failed");
        break;

    case Type::ZIP:
        if (archive_write_set_format_zip(ar) != ARCHIVE_OK)
            throw std::runtime_error("archive_write_set_format_zip failed");
        break;
    }

    if (open_archive(ar, filename.c_str()) != ARCHIVE_OK)
        throw std::runtime_error("archive_open failed");

    ar_entry = archive_entry_new();
    if (!ar_entry)
        throw std::runtime_error("archive_entry is null");
}

void TreeWriterArchive::mkdir_recursive(const fs::path &path)
{
    if (!path.has_relative_path())
        return;
    if (created_directories.count(path))
        return;
    auto parent = path.parent_path();
    mkdir_recursive(parent);

    archive_entry_clear(ar_entry);
    archive_entry_set_pathname(ar_entry, path.u8string().c_str());
    archive_entry_set_filetype(ar_entry, AE_IFDIR);
    archive_entry_set_perm(ar_entry, 0755);
    if (archive_write_header(ar, ar_entry) != ARCHIVE_OK)
        throw std::runtime_error("archive_write_header failed");

    created_directories.insert(path);
}

std::ostream &TreeWriterArchive::create_file_internal(const fs::path &path)
{
    if (created_files.count(path))
        throw std::runtime_error(path.generic_u8string() + " already exists");

    if (ostream.has_value())
        throw std::runtime_error("file is already open");

    mkdir_recursive(path.parent_path());

    ostream.emplace();
    ostream->imbue(std::locale::classic());

    archive_entry_clear(ar_entry);
    archive_entry_set_pathname(ar_entry, path.u8string().c_str());
    archive_entry_set_filetype(ar_entry, AE_IFREG);
    archive_entry_set_perm(ar_entry, 0644);
    const time_t file_time = 1485716817; // Sun Jan 29 20:06:57 2017 (first commit)
    archive_entry_set_ctime(ar_entry, file_time, 0);
    archive_entry_set_mtime(ar_entry, file_time, 0);

    created_files.insert(path);

    return *ostream;
}

void TreeWriterArchive::close_file()
{
    if (!ostream.has_value())
        throw std::runtime_error("no open file");

    const auto data = ostream->str();

    archive_entry_set_size(ar_entry, data.size());
    if (archive_write_header(ar, ar_entry) != ARCHIVE_OK)
        throw std::runtime_error("archive_write_header failed");
    if (archive_write_data(ar, data.data(), data.size()) != static_cast<ssize_t>(data.size()))
        throw std::runtime_error("archive_write_data failed");

    ostream.reset();
}

TreeWriterArchive::~TreeWriterArchive()
{
    archive_entry_free(ar_entry);
    archive_write_close(ar);
    archive_write_free(ar);
}

} // namespace horizon
