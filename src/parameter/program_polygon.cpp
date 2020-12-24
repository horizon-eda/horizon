#include "program_polygon.hpp"
#include "clipper/clipper.hpp"

namespace horizon {

std::optional<std::string> ParameterProgramPolygon::set_polygon(const TokenCommand &cmd)
{
    if (cmd.arguments.size() < 4)
        return "not enough arguments for set-polygon";
    if (cmd.arguments.at(0)->type != Token::Type::STR)
        return "1st argument of set-polygon must be string";
    if (cmd.arguments.at(1)->type != Token::Type::STR)
        return "2nd argument of set-polygon must be string";
    if (cmd.arguments.at(2)->type != Token::Type::INT)
        return "3rd argument of set-polygon must be int";
    if (cmd.arguments.at(3)->type != Token::Type::INT)
        return "4th argument of set-polygon must be int";

    const auto &pclass = dynamic_cast<TokenString &>(*cmd.arguments.at(0).get()).string;
    const auto &shape = dynamic_cast<TokenString &>(*cmd.arguments.at(1).get()).string;
    const auto &x0 = dynamic_cast<TokenInt &>(*cmd.arguments.at(2).get()).value;
    const auto &y0 = dynamic_cast<TokenInt &>(*cmd.arguments.at(3).get()).value;

    if (shape == "rectangle") {
        int64_t width, height;
        if (stack_pop(height) || stack_pop(width))
            return "empty stack";
        for (auto &it : get_polygons()) {
            if (it.second.parameter_class == pclass) {
                it.second.vertices = {
                        Coordi(x0 - width / 2, y0 - height / 2),
                        Coordi(x0 - width / 2, y0 + height / 2),
                        Coordi(x0 + width / 2, y0 + height / 2),
                        Coordi(x0 + width / 2, y0 - height / 2),
                };
            }
        }
    }
    else if (shape == "circle") {
        int64_t diameter;
        if (stack_pop(diameter))
            return "empty stack";
        for (auto &it : get_polygons()) {
            if (it.second.parameter_class == pclass) {
                it.second.vertices.clear();
                it.second.vertices.emplace_back(Coordi(-diameter / 2 + x0, y0));
                it.second.vertices.back().arc_center = {x0, y0};
                it.second.vertices.back().type = Polygon::Vertex::Type::ARC;

                it.second.vertices.emplace_back(Coordi(diameter / 2 + x0, y0));
                it.second.vertices.back().arc_center = {x0, y0};
                it.second.vertices.back().type = Polygon::Vertex::Type::ARC;

                it.second.vertices.emplace_back(Coordi(-diameter / 2 + x0, y0));
            }
        }
    }

    else {
        return "unknown shape " + shape;
    }

    return {};
}

std::optional<std::string> ParameterProgramPolygon::set_polygon_vertices(const TokenCommand &cmd)
{
    if (cmd.arguments.size() < 2 || cmd.arguments.at(0)->type != Token::Type::STR
        || cmd.arguments.at(1)->type != Token::Type::INT)
        return "not enough arguments";

    const auto &pclass = dynamic_cast<TokenString &>(*cmd.arguments.at(0).get()).string;
    std::size_t n_vertices = dynamic_cast<TokenInt &>(*cmd.arguments.at(1).get()).value;
    if (stack.size() < 2 * n_vertices) {
        return "not enough coordinates on stack";
    }
    for (auto &it : get_polygons()) {
        if (it.second.parameter_class == pclass) {
            it.second.vertices.clear();
        }
    }
    for (std::size_t i = 0; i < n_vertices; i++) {
        Coordi c;
        if (stack_pop(c.y) || stack_pop(c.x)) {
            return "empty stack";
        }
        for (auto &it : get_polygons()) {
            if (it.second.parameter_class == pclass) {
                it.second.vertices.emplace_front(c);
            }
        }
    }
    return {};
}


std::optional<std::string> ParameterProgramPolygon::expand_polygon(const TokenCommand &cmd)
{
    if (cmd.arguments.size() < 1 || cmd.arguments.at(0)->type != Token::Type::STR)
        return "not enough arguments";

    if (!(cmd.arguments.size() & 1)) {
        return "number of coordinates must be even";
    }
    ClipperLib::Path path;
    for (size_t i = 0; i < cmd.arguments.size() - 1; i += 2) {
        if (cmd.arguments.at(i + 1)->type != Token::Type::INT || cmd.arguments.at(i + 2)->type != Token::Type::INT) {
            return "coordinates must be int";
        }
        auto x = dynamic_cast<TokenInt &>(*cmd.arguments.at(i + 1).get()).value;
        auto y = dynamic_cast<TokenInt &>(*cmd.arguments.at(i + 2).get()).value;
        path.emplace_back(ClipperLib::IntPoint(x, y));
    }
    if (path.size() < 3) {
        return "must have at least 3 vertices";
    }

    int64_t expand;
    if (stack_pop(expand))
        return "empty stack";

    ClipperLib::ClipperOffset ofs;
    ofs.AddPath(path, ClipperLib::jtMiter, ClipperLib::etClosedPolygon);
    ClipperLib::Paths paths_expanded;
    ofs.Execute(paths_expanded, expand);
    if (paths_expanded.size() != 1) {
        return "expand error";
    }

    const auto &pclass = dynamic_cast<TokenString &>(*cmd.arguments.at(0).get()).string;

    for (auto &it : get_polygons()) {
        if (it.second.parameter_class == pclass) {
            it.second.vertices.clear();
            for (const auto &c : paths_expanded[0]) {
                it.second.vertices.emplace_back(Coordi(c.X, c.Y));
            }
        }
    }

    return {};
}

ParameterProgram::CommandHandler ParameterProgramPolygon::get_command(const std::string &cmd)
{
    if (auto r = ParameterProgram::get_command(cmd)) {
        return r;
    }
    else if (cmd == "set-polygon") {
        return static_cast<CommandHandler>(&ParameterProgramPolygon::set_polygon);
    }
    else if (cmd == "set-polygon-vertices") {
        return static_cast<CommandHandler>(&ParameterProgramPolygon::set_polygon_vertices);
    }
    else if (cmd == "expand-polygon") {
        return static_cast<CommandHandler>(&ParameterProgramPolygon::expand_polygon);
    }
    return nullptr;
}

} // namespace horizon
