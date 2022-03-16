#include "components.hpp"
#include "odb_util.hpp"
#include "board/board_package.hpp"

namespace horizon::ODB {

void Components::write(std::ostream &ost) const
{
    ost << "UNITS=MM" << endl;
    write_attributes(ost);
    for (const auto &comp : components) {
        comp.write(ost);
    }
}

void Components::Component::write(std::ostream &ost) const
{
    ost << "CMP " << pkg_ref << " " << placement.shift << " " << Angle{placement} << " "
        << "N"
        << " " << comp_name << " " << part_name;
    write_attributes(ost);
    ost << endl;
    for (const auto &toep : toeprints) {
        toep.write(ost);
    }
}

void Components::Toeprint::write(std::ostream &ost) const
{
    ost << "TOP " << pin_num << " " << placement.shift << " " << Angle{placement} << " "
        << "N"
        << " " << net_num << " " << subnet_num << " " << toeprint_name << endl;
}

} // namespace horizon::ODB
