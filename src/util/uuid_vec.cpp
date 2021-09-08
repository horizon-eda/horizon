#include "uuid_vec.hpp"
#include <sstream>

namespace horizon {
std::string uuid_vec_to_string(const UUIDVec &v)
{
    std::string s;
    for (const auto &uu : v) {
        if (s.size())
            s += "/";
        s += (std::string)uu;
    }
    return s;
}

UUIDVec uuid_vec_from_string(const std::string &s)
{
    UUIDVec out;
    std::string token;
    std::istringstream tokenStream(s);
    while (std::getline(tokenStream, token, '/')) {
        out.emplace_back(token);
    }
    return out;
}

UUIDVec uuid_vec_append(const UUIDVec &v, const UUID &uu)
{
    UUIDVec o = v;
    o.push_back(uu);
    return o;
}

UUID uuid_vec_flatten(const UUIDVec &v)
{
    if (v.size() == 0)
        throw std::runtime_error("can't flatten empty path");
    else if (v.size() == 1)
        return v.front();
    static const UUID ns_flatten{"822e3f74-6d4b-4b07-807a-dc56415c1a9d"};
    std::vector<unsigned char> path_bytes(v.size() * UUID::size);
    auto p = path_bytes.data();
    for (const auto &uu : v) {
        for (size_t i = 0; i < UUID::size; i++) {
            *p++ = uu.get_bytes()[i];
        }
    }
    return UUID::UUID5(ns_flatten, path_bytes.data(), path_bytes.size());
}

std::pair<UUIDVec, UUID> uuid_vec_split(const UUIDVec &v)
{
    if (v.size() == 0)
        throw std::runtime_error("can't split empty path");
    UUIDVec v2 = v;
    UUID uu = v.back();
    v2.pop_back();
    return {v2, uu};
}


} // namespace horizon
