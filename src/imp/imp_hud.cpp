#include "imp.hpp"
#include "util/str_util.hpp"
#include "util/util.hpp"
#include "util/selection_util.hpp"
#include "pool/entity.hpp"
#include "pool/part.hpp"

namespace horizon {
void ImpBase::hud_update()
{
    if (core->tool_is_active())
        return;

    auto sel = canvas->get_selection();
    if (sel.size()) {
        std::string hud_text = get_hud_text(sel);
        trim(hud_text);
        hud_text += "\n\n";
        auto text_generic = ImpBase::get_hud_text(sel);
        trim(text_generic);
        hud_text += text_generic;
        trim(hud_text);
        main_window->hud_update(hud_text);
    }
    else {
        main_window->hud_update("");
    }
}

std::string ImpBase::get_hud_text(std::set<SelectableRef> &sel)
{
    std::string s;
    if (sel_count_type(sel, ObjectType::LINE)) {
        auto n = sel_count_type(sel, ObjectType::LINE);
        s += "\n\n<b>" + std::to_string(n) + " " + object_descriptions.at(ObjectType::LINE).get_name_for_n(n)
             + "</b>\n";
        std::set<int> layers;
        int64_t length = 0;
        for (const auto &it : sel) {
            if (it.type == ObjectType::LINE) {
                const auto li = core->get_line(it.uuid);
                layers.insert(li->layer);
                length += sqrt((li->from->position - li->to->position).mag_sq());
            }
        }
        s += "Layers: ";
        for (auto layer : layers) {
            s += core->get_layer_provider().get_layers().at(layer).name + " ";
        }
        s += "\nTotal length: " + dim_to_string(length, false);
        sel_erase_type(sel, ObjectType::LINE);
    }

    // Display the length if a single edge of a polygon is given
    if (sel_count_type(sel, ObjectType::POLYGON_EDGE) == 1) {

        auto n = sel_count_type(sel, ObjectType::POLYGON_EDGE);
        s += "\n\n<b>" + std::to_string(n) + " " + object_descriptions.at(ObjectType::POLYGON_EDGE).get_name_for_n(n)
             + "</b>\n";
        int64_t length = 0;
        for (const auto &it : sel) {
            if (it.type == ObjectType::POLYGON_EDGE) {
                const auto li = core->get_polygon(it.uuid);
                const auto pair = li->get_vertices_for_edge(it.vertex);
                length += sqrt((li->vertices[pair.first].position - li->vertices[pair.second].position).mag_sq());
                s += "Layer: ";
                s += core->get_layer_provider().get_layers().at(li->layer).name + " ";
                s += "\nLength: " + dim_to_string(length, false);
                sel_erase_type(sel, ObjectType::POLYGON_EDGE);
                break;
            }
        }
    }

    if (sel_count_type(sel, ObjectType::TEXT) == 1) {
        const auto text = core->get_text(sel_find_one(sel, ObjectType::TEXT).uuid);
        const auto txt = Glib::ustring(text->text);
        auto regex = Glib::Regex::create(R"((https?:\/\/|file:\/\/\/?)([\w\.-]+)(\/\S+)?)");
        Glib::MatchInfo ma;
        if (regex->match(txt, ma)) {
            s += "\n\n<b>Text with links</b>\n";
            do {
                auto url = ma.fetch(0);
                auto regex_file = Glib::Regex::create(R"(file:\/\/?(\S+\/|)([^\/\s]+))");
                Glib::MatchInfo ma_f;
                auto name = ma.fetch(2);
                if (regex_file->match(url, ma_f)) {
                    name = ma_f.fetch(2);
                }
                if (ma.fetch(1).compare("file://") == 0) {
                    url = url.replace(0, 7, "file://" + Glib::path_get_dirname(core->get_filename()) + "/");
                }
                s += "<a href=\"" + Glib::Markup::escape_text(url) + "\" title=\""
                     + Glib::Markup::escape_text(Glib::Markup::escape_text(url)) + "\">" + name + "</a>\n";
            } while (ma.next());
            sel_erase_type(sel, ObjectType::TEXT);
        }
    }

    trim(s);
    if (sel.size()) {
        s += "\n\n<b>Others:</b>\n";
        for (const auto &it : object_descriptions) {
            auto n = std::count_if(sel.begin(), sel.end(), [&it](const auto &a) { return a.type == it.first; });
            if (n) {
                s += std::to_string(n) + " " + it.second.get_name_for_n(n) + "\n";
            }
        }
        sel.clear();
    }
    return s;
}

std::string ImpBase::get_hud_text_for_net(const Net *net)
{
    if (!net)
        return "No net";

    std::string s = "Net: " + net->name + "\n";
    s += "Net class " + net->net_class->name + "\n";
    if (net->is_power)
        s += "is power net";

    trim(s);
    return s;
}

std::string ImpBase::get_hud_text_for_component(const Component *comp)
{
    const Part *part = comp->part;
    if (!part) {
        std::string s = "No part\n";
        s += "Entity: " + comp->entity->name;
        return s;
    }
    else {
        std::string s = "MPN: " + Glib::Markup::escape_text(part->get_MPN()) + "\n";
        s += "Manufacturer: " + Glib::Markup::escape_text(part->get_manufacturer()) + "\n";
        s += "Package: " + Glib::Markup::escape_text(part->package->name) + "\n";
        if (part->get_description().size())
            s += Glib::Markup::escape_text(part->get_description()) + "\n";
        if (part->get_datasheet().size())
            s += "<a href=\"" + Glib::Markup::escape_text(part->get_datasheet()) + "\" title=\""
                 + Glib::Markup::escape_text(Glib::Markup::escape_text(part->get_datasheet())) + "\">Datasheet</a>\n";

        const auto block = core->get_block();
        if (comp->group)
            s += "Group: " + Glib::Markup::escape_text(block->get_group_name(comp->group)) + "\n";
        if (comp->tag)
            s += "Tag: " + Glib::Markup::escape_text(block->get_tag_name(comp->tag)) + "\n";

        trim(s);
        return s;
    }
}

} // namespace horizon
