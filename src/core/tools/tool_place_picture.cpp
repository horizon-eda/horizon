#include "tool_place_picture.hpp"
#include "document/idocument.hpp"
#include "common/picture.hpp"
#include "imp/imp_interface.hpp"
#include <iostream>
#include "util/picture_util.hpp"
#include "dialogs/enter_datum_window.hpp"

namespace horizon {

bool ToolPlacePicture::can_begin()
{
    return doc.r->has_object_type(ObjectType::PICTURE);
}

ToolResponse ToolPlacePicture::begin(const ToolArgs &args)
{
    auto fn = imp->dialogs.ask_picture_filename();
    if (fn) {
        temp = doc.r->insert_picture(UUID::random());
        temp->placement.shift = args.coords;
        temp->data = picture_data_from_file(*fn);
        temp->data_uuid = temp->data->uuid;
        float width = 10_mm;
        temp->px_size = width / temp->data->width;

        imp->tool_bar_set_actions({
                {InToolActionID::LMB},
                {InToolActionID::RMB},
                {InToolActionID::ROTATE},
                {InToolActionID::ENTER_SIZE, "set size"},
        });
        return ToolResponse();
    }
    else {
        return ToolResponse::end();
    }
}

ToolResponse ToolPlacePicture::update(const ToolArgs &args)
{

    if (args.type == ToolEventType::MOVE) {
        if (imp->dialogs.get_nonmodal() == nullptr)
            temp->placement.shift = args.coords;
        return ToolResponse();
    }
    else if (args.type == ToolEventType::ACTION) {
        switch (args.action) {
        case InToolActionID::LMB:
            return ToolResponse::commit();

        case InToolActionID::RMB:
        case InToolActionID::CANCEL:
            return ToolResponse::revert();

        case InToolActionID::ROTATE:
            temp->placement.inc_angle_deg(-90);
            break;

        case InToolActionID::ENTER_SIZE: {
            auto win = imp->dialogs.show_enter_datum_window("Enter pixel size", temp->px_size);
            win->set_use_ok(false);
            win->set_range(.00001_mm, 10_mm);
            win->set_step_size(.0001_mm);
        } break;


        default:;
        }
    }
    else if (args.type == ToolEventType::DATA) {
        if (auto data = dynamic_cast<const ToolDataWindow *>(args.data.get())) {
            if (data->event == ToolDataWindow::Event::UPDATE) {
                if (auto d = dynamic_cast<const ToolDataEnterDatumWindow *>(args.data.get())) {
                    temp->px_size = d->value;
                    return ToolResponse();
                }
            }
        }
    }
    return ToolResponse();
}
} // namespace horizon
