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
    std::optional<std::string> set_polygon(const TokenCommand &cmd);
    std::optional<std::string> set_polygon_vertices(const TokenCommand &cmd);
    std::optional<std::string> expand_polygon(const TokenCommand &cmd);
    virtual std::map<UUID, Polygon> &get_polygons() = 0;

    CommandHandler get_command(const std::string &cmd) override;

    virtual ~ParameterProgramPolygon()
    {
    }
};
} // namespace horizon
