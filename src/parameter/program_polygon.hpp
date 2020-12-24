#pragma once
#include "program.hpp"
#include <map>
#include "util/uuid.hpp"
#include "common/polygon.hpp"

namespace horizon {
class ParameterProgramPolygon : public ParameterProgram {
public:
    using ParameterProgram::ParameterProgram;

protected:
    std::pair<bool, std::string> set_polygon(const ParameterProgram::TokenCommand *cmd);
    std::pair<bool, std::string> set_polygon_vertices(const ParameterProgram::TokenCommand *cmd);
    std::pair<bool, std::string> expand_polygon(const ParameterProgram::TokenCommand *cmd);
    virtual std::map<UUID, Polygon> &get_polygons() = 0;

    virtual ~ParameterProgramPolygon()
    {
    }
};
} // namespace horizon
