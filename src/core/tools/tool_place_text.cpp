#include "tool_place_text.hpp"
#include "imp/imp_interface.hpp"
#include <iostream>
#include "nlohmann/json.hpp"
#include "common/text.hpp"
#include "document/idocument.hpp"
#include "document/idocument_board.hpp"
#include "core/tool_id.hpp"
#include "util/selection_util.hpp"
#include "board/board.hpp"
#include "board/board_layers.hpp"
#include "core/tool_data_window.hpp"
#include "dialogs/edit_text_window.hpp"

namespace horizon {

ToolPlaceText::Settings::LayerSettings::LayerSettings(const json &j)
    : width(j.value("width", 0)), size(j.value("size", 1.5_mm))
{
}

json ToolPlaceText::Settings::LayerSettings::serialize() const
{
    json j;
    j["width"] = width;
    j["size"] = size;
    return j;
}

void ToolPlaceText::Settings::load_from_json(const json &j)
{
    if (j.count("layers")) {
        const json &o = j["layers"];
        for (auto it = o.cbegin(); it != o.cend(); ++it) {
            int k = std::stoi(it.key());
            layers.emplace(std::piecewise_construct, std::forward_as_tuple(k), std::forward_as_tuple(it.value()));
        }
    }
    if (j.count("font"))
        font = TextData::font_lut.lookup(j.at("font").get<std::string>());
}

static const ToolPlaceText::Settings::LayerSettings settings_default;


const ToolPlaceText::Settings::LayerSettings &ToolPlaceText::Settings::get_layer(int l) const
{
    if (layers.count(l))
        return layers.at(l);
    else
        return settings_default;
}

json ToolPlaceText::Settings::serialize() const
{
    json j;
    j["layers"] = json::object();
    for (const auto &it : layers) {
        j["layers"][std::to_string(it.first)] = it.second.serialize();
    }
    j["font"] = TextData::font_lut.lookup_reverse(font);
    return j;
}

bool ToolPlaceText::can_begin()
{
    if (tool_id == ToolID::PLACE_TEXT)
        return doc.r->has_object_type(ObjectType::TEXT);
    else if (tool_id == ToolID::ADD_TEXT)
        return sel_count_type(selection, ObjectType::BOARD_PACKAGE) == 1;
    else
        return false;
}

bool ToolPlaceText::is_specific()
{
    return tool_id == ToolID::ADD_TEXT;
}

void ToolPlaceText::apply_settings()
{
    if (temp) {
        const auto &s = settings.get_layer(temp->layer);
        temp->width = s.width;
        temp->size = s.size;
        temp->font = settings.font;
    }
}

ToolResponse ToolPlaceText::begin(const ToolArgs &args)
{
    std::cout << "tool place text\n";
    if (tool_id == ToolID::ADD_TEXT) {
        pkg = &doc.b->get_board()->packages.at(sel_find_one(selection, ObjectType::BOARD_PACKAGE).uuid);
    }

    temp = doc.r->insert_text(UUID::random());
    imp->set_snap_filter({{ObjectType::TEXT, temp->uuid}});
    if (pkg) {
        if (pkg->flip)
            temp->layer = BoardLayers::BOTTOM_SILKSCREEN;
        else
            temp->layer = BoardLayers::TOP_SILKSCREEN;
        imp->set_work_layer(temp->layer);
        temp->text = "$VALUE";
        temp->text_override = pkg->replace_text(temp->text, &temp->overridden);
    }
    else {
        temp->layer = args.work_layer;
    }
    temp->placement.shift = args.coords;
    selection = {{temp->uuid, ObjectType::TEXT}};
    apply_settings();
    imp->tool_bar_set_actions({
            {InToolActionID::LMB},
            {InToolActionID::RMB},
            {InToolActionID::ROTATE},
            {InToolActionID::MIRROR},
            {InToolActionID::ENTER_WIDTH, "text width"},
            {InToolActionID::ENTER_SIZE, "text size"},
            {InToolActionID::EDIT, "change text"},
    });
    if (!pkg) {
        auto dia = imp->dialogs.show_edit_text_window(*temp, false);
        dia->focus_text();
    }
    else {
        pkg->texts.push_back(temp);
    }

    return ToolResponse();
}
ToolResponse ToolPlaceText::update(const ToolArgs &args)
{
    std::string *text;

    if (args.type == ToolEventType::MOVE) {
        if (imp->dialogs.get_nonmodal() == nullptr)
            temp->placement.shift = args.coords;
    }
    else if (args.type == ToolEventType::ACTION) {
        switch (args.action) {
        case InToolActionID::LMB: {
            if (imp->dialogs.get_nonmodal() == nullptr) {
                if (tool_id == ToolID::ADD_TEXT) {
                    selection = {{temp->uuid, ObjectType::TEXT}};
                    return ToolResponse::commit();
                }

                text = &temp->text;
                texts_placed.push_front(temp);
                auto old_text = temp;
                temp = doc.r->insert_text(UUID::random());
                imp->set_snap_filter({{ObjectType::TEXT, temp->uuid}});
                temp->text = *text;
                temp->layer = old_text->layer;
                temp->placement = old_text->placement;
                temp->placement.shift = args.coords;
                selection = {{temp->uuid, ObjectType::TEXT}};
                apply_settings();
            }
        } break;

        case InToolActionID::RMB:
        case InToolActionID::CANCEL:
            return finish();

        case InToolActionID::EDIT: {
            auto dia = imp->dialogs.show_edit_text_window(*temp, false);
            dia->focus_text();
        } break;

        case InToolActionID::ENTER_WIDTH: {
            auto dia = imp->dialogs.show_edit_text_window(*temp, false);
            dia->focus_width();
        } break;

        case InToolActionID::ENTER_SIZE: {
            auto dia = imp->dialogs.show_edit_text_window(*temp, false);
            dia->focus_size();
        } break;

        case InToolActionID::ROTATE:
        case InToolActionID::MIRROR:
            move_mirror_or_rotate(temp->placement.shift, args.action == InToolActionID::ROTATE);
            break;

        default:;
        }
    }
    else if (args.type == ToolEventType::LAYER_CHANGE) {
        temp->layer = args.work_layer;
        apply_settings();
        if (auto win = dynamic_cast<EditTextWindow *>(imp->dialogs.get_nonmodal())) {
            win->set_dims(temp->size, temp->width);
        }
    }
    else if (args.type == ToolEventType::DATA) {
        if (auto data = dynamic_cast<const ToolDataWindow *>(args.data.get())) {
            if (data->event == ToolDataWindow::Event::OK || data->event == ToolDataWindow::Event::CLOSE) {
                if (data->event == ToolDataWindow::Event::OK)
                    imp->dialogs.close_nonmodal();
                if (temp->text.size() == 0)
                    return finish();
            }
            else if (data->event == ToolDataWindow::Event::UPDATE) {
                if (pkg)
                    temp->text_override = pkg->replace_text(temp->text, &temp->overridden);
                settings.layers[temp->layer].width = temp->width;
                settings.layers[temp->layer].size = temp->size;
                settings.font = temp->font;
            }
        }
    }
    return ToolResponse();
}

ToolResponse ToolPlaceText::finish()
{
    doc.r->delete_text(temp->uuid);
    selection.clear();
    for (auto it : texts_placed) {
        selection.emplace(it->uuid, ObjectType::TEXT);
    }
    return ToolResponse::commit();
}

} // namespace horizon
