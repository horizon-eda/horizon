#include "tool_edit_table.hpp"

#include "core/tool_data_window.hpp"
#include "document/idocument.hpp"
#include "imp/imp_interface.hpp"
#include "util/selection_util.hpp"

namespace horizon {
ToolResponse ToolEditTable::begin(const ToolArgs &args)
{
    auto table = doc.r->get_table(sel_find_exactly_one(selection, ObjectType::TABLE)->uuid);

    imp->dialogs.show_edit_table_window(*table, true);

    return ToolResponse();
}

ToolResponse ToolEditTable::update(const ToolArgs &args)
{
    if (args.type == ToolEventType::DATA) {
        if (auto data = dynamic_cast<const ToolDataWindow *>(args.data.get())) {
            switch (data->event) {
            case ToolDataWindow::Event::OK:
                return ToolResponse::commit();
            case ToolDataWindow::Event::CLOSE:
                return ToolResponse::revert();
            default:;
            }
        }
    }
    return ToolResponse();
}

bool ToolEditTable::can_begin()
{
    return sel_find_exactly_one(selection, ObjectType::TABLE);
}
} // namespace horizon