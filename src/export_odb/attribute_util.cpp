#include "attribute_util.hpp"
#include <sstream>
#include <iomanip>
#include "util/once.hpp"
#include "odb_util.hpp"

namespace horizon::ODB {

namespace attribute::detail {
std::string make_legal_string_attribute(const std::string &n)
{
    std::string out;
    out.reserve(n.size());
    for (auto c : utf8_to_ascii(n)) {
        if (isgraph(c) || c == ' ')
            ;
        else if (isspace(c))
            c = ' ';
        else
            c = '_';
        out.append(1, c);
    }

    return out;
}
} // namespace attribute::detail

std::string AttributeProvider::double_to_string(double v, unsigned int n)
{
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(n) << v;
    return oss.str();
}


static unsigned int get_or_create_text(std::map<std::string, unsigned int> &m, const std::string &t)
{
    if (m.count(t)) {
        return m.at(t);
    }
    else {
        auto n = m.size();
        m.emplace(t, n);
        return n;
    }
}

unsigned int AttributeProvider::get_or_create_attribute_name(const std::string &name)
{
    return get_or_create_text(attribute_names, name);
}

unsigned int AttributeProvider::get_or_create_attribute_text(const std::string &name)
{
    return get_or_create_text(attribute_texts, name);
}

void RecordWithAttributes::write_attributes(std::ostream &ost) const
{
    Once once;
    for (const auto &attr : attributes) {
        if (once())
            ost << " ;";
        else
            ost << ",";
        ost << attr.first;
        if (attr.second.size())
            ost << "=" << attr.second;
    }
}

void AttributeProvider::write_attributes(std::ostream &ost, const std::string &prefix) const
{
    for (const auto &[name, n] : attribute_names) {
        ost << prefix << "@" << n << " " << name << endl;
    }
    for (const auto &[name, n] : attribute_texts) {
        ost << prefix << "&" << n << " " << name << endl;
    }
}

} // namespace horizon::ODB
