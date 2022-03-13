#include "tree_writer.hpp"

namespace horizon {
TreeWriter::FileProxy::FileProxy(TreeWriter &wr, const fs::path &p) : writer(wr), stream(writer.create_file_internal(p))
{
}

TreeWriter::FileProxy::~FileProxy()
{
    writer.close_file();
}

TreeWriterPrefixed::TreeWriterPrefixed(TreeWriter &pa, const fs::path &pr) : parent(pa), prefix(pr)
{
}

std::ostream &TreeWriterPrefixed::create_file_internal(const fs::path &path)
{
    return parent.create_file_internal(prefix / path);
}

void TreeWriterPrefixed::close_file()
{
    parent.close_file();
}

} // namespace horizon
