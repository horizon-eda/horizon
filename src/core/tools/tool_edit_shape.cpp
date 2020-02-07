#include "tool_edit_shape.hpp"
#include "document/idocument_padstack.hpp"
#include "pool/padstack.hpp"
#include "imp/imp_interface.hpp"
#include <iostream>

namespace horizon {

ToolEditShape::ToolEditShape(IDocument *c, ToolID tid) : ToolBase(c, tid)
{
}

bool ToolEditShape::can_begin()
{
    if (!doc.a)
        return false;
    return get_shapes().size() > 0;
}

std::set<Shape *> ToolEditShape::get_shapes()
{
    std::set<Shape *> shapes;
    for (const auto &it : selection) {
        if (it.type == ObjectType::SHAPE) {
            shapes.emplace(&doc.a->get_padstack()->shapes.at(it.uuid));
        }
    }
    return shapes;
}

ToolResponse ToolEditShape::begin(const ToolArgs &args)
{
    std::cout << "tool edit shape\n";

    auto shapes = get_shapes();

    bool r = imp->dialogs.edit_shapes(shapes);
    if (r) {
        return ToolResponse::commit();
    }
    else {
        return ToolResponse::revert();
    }
}
ToolResponse ToolEditShape::update(const ToolArgs &args)
{
    return ToolResponse();
}
} // namespace horizon
