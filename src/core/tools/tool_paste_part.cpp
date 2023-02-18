#include "tool_paste_part.hpp"
#include "document/idocument_schematic.hpp"
#include "schematic/schematic.hpp"
#include "imp/imp_interface.hpp"
#include <gtkmm.h>
#include "nlohmann/json.hpp"
#include "pool/ipool.hpp"
#include "pool/part.hpp"

namespace horizon {

class ToolDataPastePart : public ToolData {
public:
    ToolDataPastePart(const json &j) : paste_data(j)
    {
    }
    json paste_data;
};


std::shared_ptr<const Entity> ToolPastePart::get_entity()
{
    std::shared_ptr<const Entity> entity = nullptr;
    for (const auto &it : selection) {
        if (it.type == ObjectType::SCHEMATIC_SYMBOL) {
            auto &sym = doc.c->get_sheet()->symbols.at(it.uuid);
            if (entity) {
                if (entity != sym.component->entity) {
                    return nullptr;
                }
            }
            else {
                entity = sym.component->entity;
            }
        }
    }
    return entity;
}

bool ToolPastePart::can_begin()
{
    return get_entity() != nullptr;
}

ToolResponse ToolPastePart::begin_paste(const json &j)
{
    if (j.count("components")) {
        auto &comps = j.at("components");
        if (comps.size() != 1) {
            imp->tool_bar_flash("Need exactly one component in clipboard");
            return ToolResponse::end();
        }
        auto &comp_j = comps.items().begin().value();
        if (comp_j.count("part")) {
            const UUID part_uu = comp_j.at("part").get<std::string>();
            std::shared_ptr<const Part> part = nullptr;
            try {
                part = doc.c->get_pool_caching().get_part(part_uu);
            }
            catch (...) {
            }
            if (!part) {
                imp->tool_bar_flash("Part from clipboard not found in pool");
                return ToolResponse::end();
            }
            auto entity = get_entity();
            if (part->entity->uuid != entity->uuid) {
                imp->tool_bar_flash("Can't paste part of different entity");
                return ToolResponse::end();
            }

            for (const auto &it : selection) {
                if (it.type == ObjectType::SCHEMATIC_SYMBOL) {
                    auto &sym = doc.c->get_sheet()->symbols.at(it.uuid);
                    if (sym.component->entity == entity) {
                        sym.component->part = part;
                    }
                }
            }
            return ToolResponse::commit();
        }
    }
    return ToolResponse::end();
}

ToolResponse ToolPastePart::begin(const ToolArgs &args)
{
    Gtk::Clipboard::get()->request_contents("imp-buffer", [this](const Gtk::SelectionData &sel_data) {
        if (sel_data.gobj() && sel_data.get_data_type() == "imp-buffer") {
            auto td = std::make_unique<ToolDataPastePart>(json::parse(sel_data.get_data_as_string()));
            imp->tool_update_data(std::move(td));
        }
        else {
            auto td = std::make_unique<ToolDataPastePart>(nullptr);
            imp->tool_update_data(std::move(td));
        }
    });
    return ToolResponse();
}

ToolResponse ToolPastePart::update(const ToolArgs &args)
{
    if (args.type == ToolEventType::DATA) {
        if (auto data = dynamic_cast<ToolDataPastePart *>(args.data.get())) {
            if (data->paste_data != nullptr) {
                return begin_paste(data->paste_data);
            }
            else {
                imp->tool_bar_flash("Empty Buffer");
                return ToolResponse::end();
            }
        }
    }


    return ToolResponse();
}

} // namespace horizon
