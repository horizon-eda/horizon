#pragma once
#include "temp_dir.hpp"
#include "project/project.hpp"
#include <set>
#include <vector>

namespace horizon::cli {
class Output {
public:
    // Remember which files belong to the project, so an export cannot overwrite its own inputs
    Output(const Project &project, const std::string &project_filename, bool overwrite);
    // Check and remember the output path, then return a temporary filename for the exporter to write to
    // The optional relative path keeps the directory layout when exporting a whole tree
    std::string add_file(const std::filesystem::path &destination,
                         const std::filesystem::path &relative = {});
    // Collect the files from a generated directory tree so they can be copied to the output directory
    // Empty files are allowed here, since ODB++ uses them for metadata
    void add_tree(const std::filesystem::path &destination);
    // Check the generated files and their destinations, then copy them out of the temporary directory
    // Copying several files is not an all-or-nothing operation, so a failure can leave partial output
    void publish();
    // Return the temporary directory for exporters that create several files or a directory tree
    const std::filesystem::path &get_directory() const
    {
        return temporary.get_path();
    }

private:
    TempDir temporary;
    bool overwrite;
    std::vector<std::pair<std::filesystem::path, std::filesystem::path>> files;
    std::set<std::filesystem::path> sources;
    std::set<std::filesystem::path> tree_files;
    std::vector<std::filesystem::path> source_directories;
    // Check that the destination is safe to write to, including whether --overwrite is needed
    // Links must not allow an export to replace a project input under a different name
    void check_destination(const std::filesystem::path &destination) const;
};
} // namespace horizon::cli
