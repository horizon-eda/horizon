#pragma once
#include "attribute_util.hpp"
#include "eda_data.hpp"

namespace horizon::ODB {

class Components : public AttributeProvider {
public:
    class Toeprint {
    public:
        Toeprint(const EDAData::Pin &pin) : pin_num(pin.index), toeprint_name(pin.name)
        {
        }

        unsigned int pin_num;

        Placement placement;
        unsigned int net_num = 0;
        unsigned int subnet_num = 0;
        std::string toeprint_name = 0;

        void write(std::ostream &ost) const;
    };

    class Component : public RecordWithAttributes {
    public:
        template <typename T> using check_type = attribute::is_comp<T>;

        Component(unsigned int i, unsigned int r) : index(i), pkg_ref(r)
        {
        }
        const unsigned int index;
        unsigned int pkg_ref;
        Placement placement;

        std::string comp_name;
        std::string part_name;

        std::list<Toeprint> toeprints;

        void write(std::ostream &ost) const;
    };

    std::list<Component> components;

    void write(std::ostream &ost) const;
};

} // namespace horizon::ODB
