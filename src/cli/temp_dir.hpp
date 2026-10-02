#pragma once
#include <filesystem>

namespace horizon::cli {
class TempDir {
public:
    // Create a temporary directory for the pool index or export files
    TempDir();
    // Clean up the temporary files without hiding an error that already caused the export to fail
    ~TempDir();
    TempDir(const TempDir &) = delete;
    TempDir &operator=(const TempDir &) = delete;
    // Return the directory path so callers can write their temporary files here
    const std::filesystem::path &get_path() const
    {
        return path;
    }

private:
    std::filesystem::path path;
};
} // namespace horizon::cli
