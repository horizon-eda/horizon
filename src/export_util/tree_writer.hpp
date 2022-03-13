#pragma once
#include <iostream>
#include <filesystem>

namespace horizon {
namespace fs = std::filesystem;
class TreeWriter {
    friend class TreeWriterPrefixed;

public:
    class FileProxy {
        friend TreeWriter;

    private:
        FileProxy(TreeWriter &writer, const fs::path &p);
        TreeWriter &writer;

    public:
        std::ostream &stream;

        ~FileProxy();

        FileProxy(FileProxy &&) = delete;
        FileProxy &operator=(FileProxy &&) = delete;

        FileProxy(FileProxy const &) = delete;
        FileProxy &operator=(FileProxy const &) = delete;
    };

    [[nodiscard]] FileProxy create_file(const fs::path &path)
    {
        return FileProxy(*this, path);
    }

private:
    virtual std::ostream &create_file_internal(const fs::path &path) = 0;
    virtual void close_file() = 0;
};

class TreeWriterPrefixed : public TreeWriter {
public:
    TreeWriterPrefixed(TreeWriter &parent, const fs::path &prefix);

private:
    std::ostream &create_file_internal(const fs::path &path) override;
    void close_file() override;

    TreeWriter &parent;
    const fs::path prefix;
};

} // namespace horizon
