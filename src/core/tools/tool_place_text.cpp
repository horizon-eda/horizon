#include "tool_place_text.hpp"
#include "imp/imp_interface.hpp"
#include <iostream>
#include "nlohmann/json.hpp"
#include "common/text.hpp"
#include "document/idocument.hpp"
#include <gdk/gdkkeysyms.h>

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
    return j;
}

ToolPlaceText::ToolPlaceText(IDocument *c, ToolID tid) : ToolBase(c, tid), ToolHelperMove(c, tid)
{
}

bool ToolPlaceText::can_begin()
{
    return doc.r->has_object_type(ObjectType::TEXT);
}

void ToolPlaceText::apply_settings()
{
    if (temp) {
        const auto &s = settings.get_layer(temp->layer);
        temp->width = s.width;
        temp->size = s.size;
    }
}

ToolResponse ToolPlaceText::begin(const ToolArgs &args)
{
    std::cout << "tool place text\n";

    temp = doc.r->insert_text(UUID::random());
    imp->set_snap_filter({{ObjectType::TEXT, temp->uuid}});
    temp->layer = args.work_layer;
    temp->placement.shift = args.coords;
    apply_settings();
    imp->tool_bar_set_tip(
            "<b>LMB:</b>place text <b>RMB:</b>finish "
            "<b>space:</b>change text <b>w:</b>text width <b>s:</b>text size");

    auto r = imp->dialogs.ask_datum_string("Enter text", temp->text);
    if (r.first) {
        temp->text = r.second;
    }
    else {
        doc.r->delete_text(temp->uuid);
        selection.clear();
        return ToolResponse::end();
    }

    return ToolResponse();
}
ToolResponse ToolPlaceText::update(const ToolArgs &args)
{
    std::string *text;

    if (args.type == ToolEventType::MOVE) {
        temp->placement.shift = args.coords;
    }
    else if (args.type == ToolEventType::CLICK) {
        if (args.button == 1) {
            text = &temp->text;
            texts_placed.push_front(temp);
            auto old_text = temp;
            temp = doc.r->insert_text(UUID::random());
            imp->set_snap_filter({{ObjectType::TEXT, temp->uuid}});
            temp->text = *text;
            temp->layer = args.work_layer;
            temp->placement = old_text->placement;
            temp->placement.shift = args.coords;
            apply_settings();
        }
        else if (args.button == 3) {
            doc.r->delete_text(temp->uuid);
            selection.clear();
            for (auto it : texts_placed) {
                selection.emplace(it->uuid, ObjectType::TEXT);
            }
            return ToolResponse::commit();
        }
    }
    else if (args.type == ToolEventType::LAYER_CHANGE) {
        temp->layer = args.work_layer;
        apply_settings();
    }
    else if (args.type == ToolEventType::KEY) {
        if (args.key == GDK_KEY_space) {
            auto r = imp->dialogs.ask_datum_string("Enter text", temp->text);
            if (r.first) {
                temp->text = r.second;
            }
        }
        else if (args.key == GDK_KEY_w) {
            auto r = imp->dialogs.ask_datum("Enter width", settings.get_layer(temp->layer).width);
            if (r.first) {
                settings.layers[temp->layer].width = std::max(r.second, (int64_t)0);
                apply_settings();
            }
        }
        else if (args.key == GDK_KEY_s) {
            auto r = imp->dialogs.ask_datum("Enter size", settings.get_layer(temp->layer).size);
            if (r.first) {
                settings.layers[temp->layer].size = std::max(r.second, (int64_t)0);
                apply_settings();
            }
        }
        else if (args.key == GDK_KEY_r || args.key == GDK_KEY_e) {
            bool rotate = args.key == GDK_KEY_r;
            move_mirror_or_rotate(temp->placement.shift, rotate);
        }
        else if (args.key == GDK_KEY_Escape) {
            return ToolResponse::revert();
        }
    }
    return ToolResponse();
}
} // namespace horizon
