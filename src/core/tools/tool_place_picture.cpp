#include "tool_place_picture.hpp"
#include "document/idocument.hpp"
#include "common/picture.hpp"
#include "imp/imp_interface.hpp"
#include <iostream>
#include <gdk/gdkkeysyms.h>
#include "util/picture_util.hpp"
#include "dialogs/enter_datum_window.hpp"

namespace horizon {

ToolPlacePicture::ToolPlacePicture(IDocument *c, ToolID tid) : ToolBase(c, tid)
{
}

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

        imp->tool_bar_set_tip("<b>LMB:</b>place picture <b>RMB:</b>cancel <b>r:</b>rotate <b>s:</b>set size");
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
        return ToolResponse::fast();
    }
    else if (args.type == ToolEventType::CLICK) {
        if (args.button == 1) {
            return ToolResponse::commit();
        }
        else if (args.button == 3) {
            return ToolResponse::revert();
        }
    }
    else if (args.type == ToolEventType::DATA) {
        if (auto data = dynamic_cast<const ToolDataWindow *>(args.data.get())) {
            if (data->event == ToolDataWindow::Event::UPDATE) {
                if (auto d = dynamic_cast<const ToolDataEnterDatumWindow *>(args.data.get())) {
                    temp->px_size = d->value;
                    return ToolResponse::fast();
                }
            }
        }
    }
    else if (args.type == ToolEventType::KEY) {
        if (args.key == GDK_KEY_r) {
            temp->placement.inc_angle_deg(-90);
        }
        else if (args.key == GDK_KEY_s) {
            auto win = imp->dialogs.show_enter_datum_window("Enter pixel size", temp->px_size);
            win->set_use_ok(false);
            win->set_range(.00001_mm, 10_mm);
            win->set_step_size(.0001_mm);
        }
        else if (args.key == GDK_KEY_Escape) {
            return ToolResponse::revert();
        }
    }
    return ToolResponse();
}
} // namespace horizon
