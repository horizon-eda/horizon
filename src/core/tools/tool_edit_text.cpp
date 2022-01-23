#include "tool_edit_text.hpp"
#include "document/idocument.hpp"
#include "common/text.hpp"
#include "imp/imp_interface.hpp"
#include "core/tool_data_window.hpp"
#include "util/selection_util.hpp"

namespace horizon {

bool ToolEditText::can_begin()
{
    return sel_find_exactly_one(selection, ObjectType::TEXT);
}

ToolResponse ToolEditText::begin(const ToolArgs &args)
{
    auto text = doc.r->get_text(sel_find_exactly_one(selection, ObjectType::TEXT)->uuid);

    imp->dialogs.show_edit_text_window(*text, true);

    return ToolResponse();
}
ToolResponse ToolEditText::update(const ToolArgs &args)
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
} // namespace horizon
