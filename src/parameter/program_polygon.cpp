#include "program_polygon.hpp"
#include "clipper/clipper.hpp"

namespace horizon {

std::pair<bool, std::string> ParameterProgramPolygon::set_polygon(const ParameterProgram::TokenCommand *cmd,
                                                                  std::deque<int64_t> &stack)
{
    if (cmd->arguments.size() < 4)
        return {true, "not enough arguments for set-polygon"};
    if (cmd->arguments.at(0)->type != ParameterProgram::Token::Type::STR)
        return {true, "1st argument of set-polygon must be string"};
    if (cmd->arguments.at(1)->type != ParameterProgram::Token::Type::STR)
        return {true, "2nd argument of set-polygon must be string"};
    if (cmd->arguments.at(2)->type != ParameterProgram::Token::Type::INT)
        return {true, "3rd argument of set-polygon must be int"};
    if (cmd->arguments.at(3)->type != ParameterProgram::Token::Type::INT)
        return {true, "4th argument of set-polygon must be int"};

    auto pclass = dynamic_cast<ParameterProgram::TokenString *>(cmd->arguments.at(0).get())->string;
    auto shape = dynamic_cast<ParameterProgram::TokenString *>(cmd->arguments.at(1).get())->string;
    auto x0 = dynamic_cast<ParameterProgram::TokenInt *>(cmd->arguments.at(2).get())->value;
    auto y0 = dynamic_cast<ParameterProgram::TokenInt *>(cmd->arguments.at(3).get())->value;

    if (shape == "rectangle") {
        int64_t width, height;
        if (ParameterProgram::stack_pop(stack, height) || ParameterProgram::stack_pop(stack, width))
            return {true, "empty stack"};
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

    else {
        return {true, "unknown shape " + shape};
    }

    return {false, ""};
}

std::pair<bool, std::string> ParameterProgramPolygon::set_polygon_vertices(const ParameterProgram::TokenCommand *cmd,
                                                                           std::deque<int64_t> &stack)
{
    if (cmd->arguments.size() < 2 || cmd->arguments.at(0)->type != ParameterProgram::Token::Type::STR
        || cmd->arguments.at(1)->type != ParameterProgram::Token::Type::INT)
        return {true, "not enough arguments"};

    auto pclass = dynamic_cast<ParameterProgram::TokenString *>(cmd->arguments.at(0).get())->string;
    std::size_t n_vertices = dynamic_cast<ParameterProgram::TokenInt *>(cmd->arguments.at(1).get())->value;
    if (stack.size() < 2 * n_vertices) {
        return {true, "not enough coordinates on stack"};
    }
    for (auto &it : get_polygons()) {
        if (it.second.parameter_class == pclass) {
            it.second.vertices.clear();
        }
    }
    for (std::size_t i = 0; i < n_vertices; i++) {
        Coordi c;
        if (stack_pop(stack, c.y) || stack_pop(stack, c.x)) {
            return {true, "empty stack"};
        }
        for (auto &it : get_polygons()) {
            if (it.second.parameter_class == pclass) {
                it.second.vertices.emplace_front(c);
            }
        }
    }
    return {false, ""};
}


std::pair<bool, std::string> ParameterProgramPolygon::expand_polygon(const ParameterProgram::TokenCommand *cmd,
                                                                     std::deque<int64_t> &stack)
{
    if (cmd->arguments.size() < 1 || cmd->arguments.at(0)->type != ParameterProgram::Token::Type::STR)
        return {true, "not enough arguments"};

    if (!(cmd->arguments.size() & 1)) {
        return {true, "number of coordinates must be even"};
    }
    ClipperLib::Path path;
    for (size_t i = 0; i < cmd->arguments.size() - 1; i += 2) {
        if (cmd->arguments.at(i + 1)->type != ParameterProgram::Token::Type::INT
            || cmd->arguments.at(i + 2)->type != ParameterProgram::Token::Type::INT) {
            return {true, "coordinates must be int"};
        }
        auto x = dynamic_cast<ParameterProgram::TokenInt *>(cmd->arguments.at(i + 1).get())->value;
        auto y = dynamic_cast<ParameterProgram::TokenInt *>(cmd->arguments.at(i + 2).get())->value;
        path.emplace_back(ClipperLib::IntPoint(x, y));
    }
    if (path.size() < 3) {
        return {true, "must have at least 3 vertices"};
    }

    int64_t expand;
    if (stack_pop(stack, expand))
        return {true, "empty stack"};

    ClipperLib::ClipperOffset ofs;
    ofs.AddPath(path, ClipperLib::jtMiter, ClipperLib::etClosedPolygon);
    ClipperLib::Paths paths_expanded;
    ofs.Execute(paths_expanded, expand);
    if (paths_expanded.size() != 1) {
        return {true, "expand error"};
    }

    auto pclass = dynamic_cast<ParameterProgram::TokenString *>(cmd->arguments.at(0).get())->string;

    for (auto &it : get_polygons()) {
        if (it.second.parameter_class == pclass) {
            it.second.vertices.clear();
            for (const auto &c : paths_expanded[0]) {
                it.second.vertices.emplace_back(Coordi(c.X, c.Y));
            }
        }
    }

    return {false, ""};
}

} // namespace horizon
