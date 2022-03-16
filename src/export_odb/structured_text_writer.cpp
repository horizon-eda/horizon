#include "structured_text_writer.hpp"

namespace horizon {

StructuredTextWriter::StructuredTextWriter(std::ostream &s) : ost(s)
{
}

void StructuredTextWriter::write_line(const std::string &var, const std::string &value)
{
    write_indent();
    ost << var << "=" << value << "\r\n";
}

void StructuredTextWriter::write_line(const std::string &var, int value)
{
    write_indent();
    ost << var << "=" << value << "\r\n";
}

void StructuredTextWriter::write_indent()
{
    if (in_array)
        ost << "    ";
}

void StructuredTextWriter::begin_array(const std::string &a)
{
    if (in_array)
        throw std::runtime_error("already in array");
    in_array = true;
    ost << a << " {\r\n";
}

void StructuredTextWriter::end_array()
{
    if (!in_array)
        throw std::runtime_error("not in array");
    in_array = false;
    ost << "}\r\n\r\n";
}

StructuredTextWriter::ArrayProxy::ArrayProxy(StructuredTextWriter &wr, const std::string &a) : writer(wr)
{
    writer.begin_array(a);
}

StructuredTextWriter::ArrayProxy::~ArrayProxy()
{
    writer.end_array();
}
} // namespace horizon
