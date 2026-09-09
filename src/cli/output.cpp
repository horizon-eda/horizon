#include "output.hpp"
#include "options.hpp"
#include "project/project.hpp"
#include "blocks/blocks.hpp"
#include <stdexcept>

namespace horizon::cli {
namespace fs = std::filesystem;

Output::Output(const Project &project, const std::string &project_filename, bool ow) : overwrite(ow)
{
    for (const auto &name :
         {project_filename, project.blocks_filename, project.board_filename, project.planes_filename}) {
        sources.insert(fs::weakly_canonical(fs::u8path(name)));
    }
    for (const auto &name : BlocksBase::peek_filenames(project.blocks_filename)) {
        sources.insert(fs::weakly_canonical(fs::u8path(name)));
    }
    for (const auto &name : {project.pool_directory, project.pictures_directory}) {
        source_directories.push_back(fs::weakly_canonical(fs::u8path(name)));
    }
}

void Output::check_destination(const fs::path &destination) const
{
    const auto canonical = fs::weakly_canonical(destination);
    if (sources.count(canonical))
        throw std::runtime_error("output would replace a project file: " + destination.u8string());
    for (const auto &directory : source_directories) {
        const auto relative = canonical.lexically_relative(directory);
        if (!relative.empty() && *relative.begin() != "..")
            throw std::runtime_error("output is inside a project input directory: " + destination.u8string());
    }
    if (fs::is_symlink(fs::symlink_status(destination)))
        throw std::runtime_error("output is a symbolic link: " + destination.u8string());
    if (fs::exists(destination)) {
        if (!fs::is_regular_file(destination))
            throw std::runtime_error("output is not a regular file: " + destination.u8string());
        if (!overwrite)
            throw std::runtime_error("output already exists: " + destination.u8string() + " (use --overwrite)");
        for (const auto &source : sources) {
            if (fs::exists(source) && fs::equivalent(source, destination))
                throw std::runtime_error("output is linked to a project file: " + destination.u8string());
        }
    }
}

std::string Output::add_file(const fs::path &destination, const fs::path &relative)
{
    check_destination(destination);
    const auto staged = temporary.get_path() / (relative.empty() ? destination.filename() : relative);
    for (const auto &[existing, target] : files) {
        if (existing == staged || fs::weakly_canonical(target) == fs::weakly_canonical(destination))
            throw UsageError("duplicate output filename: " + destination.u8string());
    }
    files.emplace_back(staged, destination);
    return staged.u8string();
}

void Output::add_tree(const fs::path &destination)
{
    for (const auto &entry : fs::recursive_directory_iterator(temporary.get_path())) {
        if (entry.is_symlink())
            throw std::runtime_error("exporter created a symbolic link: " + entry.path().u8string());
        if (!entry.is_regular_file())
            continue;
        const auto relative = entry.path().lexically_relative(temporary.get_path());
        add_file(destination / relative, relative);
        tree_files.insert(entry.path());
    }
    if (tree_files.empty())
        throw std::runtime_error("exporter did not write a directory tree");

    // An old layer file left in the directory would still be part of the ODB++ job
    // Refuse extra files even with --overwrite, so we do not mix old and new output
    std::set<fs::path> roots;
    std::set<fs::path> targets;
    for (const auto &[staged, target] : files) {
        const auto relative = staged.lexically_relative(temporary.get_path());
        roots.insert(destination / *relative.begin());
        targets.insert(fs::weakly_canonical(target));
    }
    for (const auto &root : roots) {
        if (!fs::exists(root))
            continue;
        if (fs::is_symlink(root))
            throw std::runtime_error("output job is a symbolic link: " + root.u8string());
        for (const auto &entry : fs::recursive_directory_iterator(root)) {
            if (entry.is_symlink() || (entry.is_regular_file() && !targets.count(fs::weakly_canonical(entry.path()))))
                throw std::runtime_error("existing job has extra files, use a clean output directory: "
                                         + entry.path().u8string());
        }
    }
}

void Output::publish()
{
    // Check the whole set again before copying anything, since destinations may have changed during export
    for (const auto &[staged, destination] : files) {
        check_destination(destination);
        if (!fs::is_regular_file(staged) || (fs::file_size(staged) == 0 && !tree_files.count(staged)))
            throw std::runtime_error("exporter did not write " + staged.filename().u8string());
    }
    for (const auto &[staged, destination] : files) {
        if (!destination.parent_path().empty())
            fs::create_directories(destination.parent_path());
        fs::copy_file(staged, destination, overwrite ? fs::copy_options::overwrite_existing : fs::copy_options::none);
    }
}
} // namespace horizon::cli
