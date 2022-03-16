#pragma once
#include "attributes.hpp"
#include <type_traits>
#include <string>
#include <map>
#include <vector>

namespace horizon::ODB {

class AttributeProvider {

public:
    template <typename Tr, typename Ta> void add_attribute(Tr &r, Ta v)
    {
        using Tc = typename Tr::template check_type<Ta>;
        static_assert(Tc::value);

        const auto id = get_or_create_attribute_name(attribute::attribute_name<Ta>::name);
        if constexpr (std::is_enum_v<Ta>)
            r.attributes.emplace_back(id, std::to_string(static_cast<int>(v)));
        else
            r.attributes.emplace_back(id, attr_to_string(v));
    }

protected:
    unsigned int get_or_create_attribute_name(const std::string &name);

    void write_attributes(std::ostream &ost, const std::string &prefix = "") const;


private:
    unsigned int get_or_create_attribute_text(const std::string &name);

    static std::string double_to_string(double v, unsigned int n);

    template <typename T, unsigned int n> std::string attr_to_string(attribute::float_attribute<T, n> a)
    {
        return double_to_string(a.value, a.ndigits);
    }

    template <typename T> std::string attr_to_string(attribute::boolean_attribute<T> a)
    {
        return "";
    }

    template <typename T> std::string attr_to_string(attribute::text_attribute<T> a)
    {
        return std::to_string(get_or_create_attribute_text(a.value));
    }


    std::map<std::string, unsigned int> attribute_names;
    std::map<std::string, unsigned int> attribute_texts;
};

class RecordWithAttributes {

protected:
    void write_attributes(std::ostream &ost) const;

public:
    std::vector<std::pair<unsigned int, std::string>> attributes;
};
} // namespace horizon::ODB
