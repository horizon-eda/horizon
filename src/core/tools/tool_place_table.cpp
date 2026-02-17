#include "tool_place_table.hpp"

#include "core/tool_id.hpp"
#include "common/table.hpp"
#include "dialogs/edit_table_window.hpp"
#include "document/idocument.hpp"
#include "imp/imp_interface.hpp"

#include <iostream>

namespace horizon {

ToolResponse ToolPlaceTable::begin(const ToolArgs &args)
{
    std::cout << "tool place table\n";

    temp = doc.r->insert_table(UUID::random());
    imp->set_snap_filter({{ObjectType::TABLE, temp->uuid}});
    temp->layer = args.work_layer;
    temp->placement.shift = args.coords;
    selection = {{temp->uuid, ObjectType::TABLE}};

    imp->tool_bar_set_actions({
            {InToolActionID::LMB},
            {InToolActionID::RMB},
            {InToolActionID::ROTATE},
            {InToolActionID::EDIT, "change table"},
    });

    auto dia = imp->dialogs.show_edit_table_window(*temp, false);
    dia->focus_n_rows();

    return {};
}

ToolResponse ToolPlaceTable::update(const ToolArgs &args)
{
    if (args.type == ToolEventType::MOVE) {
        if (imp->dialogs.get_nonmodal() == nullptr)
            temp->placement.shift = args.coords;
    }
    else if (args.type == ToolEventType::ACTION) {
        switch (args.action) {
        case InToolActionID::LMB: {
            if (imp->dialogs.get_nonmodal() == nullptr) {
                tables_placed.push_front(temp);
                auto old_table = temp;
                temp = doc.r->insert_table(UUID::random());
                imp->set_snap_filter({{ObjectType::TABLE, temp->uuid}});

                temp->layer = old_table->layer;
                temp->placement = old_table->placement;
                temp->placement.shift = args.coords;
                temp->assign_from(*old_table);

                selection = {{temp->uuid, ObjectType::TABLE}};
            }
        } break;

        case InToolActionID::RMB:
        case InToolActionID::CANCEL:
            return finish();

        case InToolActionID::EDIT: {
            auto dia = imp->dialogs.show_edit_table_window(*temp, false);
            dia->focus_n_rows();
        } break;

        case InToolActionID::ROTATE:
            move_mirror_or_rotate(temp->placement.shift, args.action == InToolActionID::ROTATE);
            break;

        default:;
        }
    }
    else if (args.type == ToolEventType::LAYER_CHANGE) {
        temp->layer = args.work_layer;
    }
    else if (args.type == ToolEventType::DATA) {
        if (auto data = dynamic_cast<const ToolDataWindow *>(args.data.get())) {
            if (data->event == ToolDataWindow::Event::OK || data->event == ToolDataWindow::Event::CLOSE) {
                if (data->event == ToolDataWindow::Event::OK)
                    imp->dialogs.close_nonmodal();
                if (temp->is_empty())
                    return finish();
            }
        }
    }
    return {};
}

bool ToolPlaceTable::can_begin()
{
    if (tool_id == ToolID::PLACE_TABLE)
        return doc.r->has_object_type(ObjectType::TABLE);
    return false;
}

ToolResponse ToolPlaceTable::finish()
{
    doc.r->delete_table(temp->uuid);
    selection.clear();
    for (auto it : tables_placed) {
        selection.emplace(it->uuid, ObjectType::TABLE);
    }
    return ToolResponse::commit();
}

} // namespace horizon