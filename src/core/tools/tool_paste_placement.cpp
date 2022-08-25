#include "tool_paste_placement.hpp"
#include "document/idocument_board.hpp"
#include "board/board.hpp"
#include "imp/imp_interface.hpp"
#include "util/selection_util.hpp"
#include "util/util.hpp"
#include "nlohmann/json.hpp"
#include <gtkmm.h>
#include "util/range_util.hpp"
#include "tool_helper_paste.hpp"
#include "pool/ipool.hpp"
#include "board/board_layers.hpp"
#include "util/geom_util.hpp"

namespace horizon {

class ToolDataPastePlacement : public ToolData {
public:
    ToolDataPastePlacement(const json &j) : paste_data(j)
    {
    }
    json paste_data;
};

bool ToolPastePlacement::can_begin()
{
    return sel_count_type(selection, ObjectType::BOARD_PACKAGE) > 1;
}


ToolResponse ToolPastePlacement::begin(const ToolArgs &args)
{
    imp->tool_bar_set_actions({
            {InToolActionID::LMB, "pick reference package"},
            {InToolActionID::RMB},

    });

    return ToolResponse();
}

ToolResponse ToolPastePlacement::begin_paste(const json &j)
{
    if (j.count("packages")) {
        auto &brd = *doc.b->get_board();
        auto pkgs_from_selection =
                selection | sel_filter_type(ObjectType::BOARD_PACKAGE) | map_ref_from_sel(brd.packages);
        std::list<PackageInfo> pkgs_from_paste;
        for (const auto &[k, v] : j.at("packages").items()) {
            pkgs_from_paste.emplace_back(k, v);
        }
        // find target pkg in pkgs_from_paste
        auto ref_pkg = find_if_ptr(pkgs_from_paste, [this](auto &x) { return x.tag == target_pkg->component->tag; });
        if (!ref_pkg) {
            imp->tool_bar_flash("reference package not found");
            return ToolResponse::end();
        }
        for (auto &pkg : pkgs_from_selection) {
            auto this_ref_pkg = find_if_ptr(pkgs_from_paste, [pkg, ref_pkg](const auto &x) {
                return x.group == ref_pkg->group && x.tag == pkg.component->tag;
            });
            if (!this_ref_pkg)
                continue;

            pkg.placement = transform_package_placement_to_new_reference(this_ref_pkg->placement, ref_pkg->placement,
                                                                         target_pkg->placement);
            pkg.flip = pkg.placement.mirror;
            if (this_ref_pkg->alternate_package) {
                auto alt_pkg = doc.b->get_pool().get_package(this_ref_pkg->alternate_package);
                if (alt_pkg->alternate_for->uuid == pkg.pool_package->uuid) {
                    pkg.alternate_package = doc.b->get_pool_caching().get_package(this_ref_pkg->alternate_package);
                }
            }
            if (this_ref_pkg->omit_silkscreen && pkg.smashed) {
                doc.b->get_board()->unsmash_package(&pkg);
            }
            pkg.omit_silkscreen = this_ref_pkg->omit_silkscreen;

            if (this_ref_pkg->smashed) {
                copy_package_silkscreen_texts(pkg, *this_ref_pkg, j);
            }
            pkg.update(brd);
            brd.update_refs();
        }

        // ranges::find_if(pkgs_from_paste, [this](auto &x) { return x.second.tag == target_pkg->component->tag; });
        //  auto pkgs_from_paste = pkg_items | ranges::views::transform([](const auto &x) { return x; });
        //   for (auto pkg : pkgs_from_selection) {
        //       pkg->
        //   }
        return ToolResponse::commit();
    }
    return ToolResponse::end();
}


void ToolPastePlacement::copy_package_silkscreen_texts(BoardPackage &dest, const PackageInfo &src, const json &j)
{
    auto &brd = *doc.b->get_board();
    // Copy smash status
    brd.unsmash_package(&dest);
    if (!src.smashed) {
        return;
    }
    dest.smashed = true;

    // Copy all texts from src
    for (const auto &text_uu : src.texts) {
        if (!(j.count("texts") && j.at("texts").count((std::string)text_uu)))
            continue;
        Text text{text_uu, j.at("texts").at((std::string)text_uu)};
        if (!BoardLayers::is_silkscreen(text.layer)) {
            // Not on top or bottom silkscreen
            continue;
        }

        auto uu = UUID::random();
        auto &x = brd.texts.emplace(uu, uu).first->second;
        x.from_smash = true;
        x.overridden = true;

        x.placement = transform_text_placement_to_new_reference(text.placement, src.placement, dest.placement);
        x.text = text.text;
        x.layer = text.layer;
        if (src.flip != dest.flip) {
            brd.flip_package_layer(x.layer);
        }

        x.size = text.size;
        x.width = text.width;
        x.font = text.font;
        dest.texts.push_back(&x);
    }
}


ToolResponse ToolPastePlacement::update(const ToolArgs &args)
{
    if (target_pkg == nullptr) {
        if (args.type == ToolEventType::ACTION && args.action == InToolActionID::LMB) {
            if (args.target.type == ObjectType::BOARD_PACKAGE || args.target.type == ObjectType::PAD) {
                auto pkg_uuid = args.target.path.at(0);
                auto brd = doc.b->get_board();
                target_pkg = &brd->packages.at(pkg_uuid);
                auto imp_interface = imp;
                Glib::signal_idle().connect_once(
                        [imp_interface] {
                            Gtk::Clipboard::get()->request_contents(
                                    "imp-buffer", [imp_interface](const Gtk::SelectionData &sel_data) {
                                        if (sel_data.gobj() && sel_data.get_data_type() == "imp-buffer") {
                                            auto td = std::make_unique<ToolDataPastePlacement>(
                                                    json::parse(sel_data.get_data_as_string()));
                                            imp_interface->tool_update_data(std::move(td));
                                        }
                                        else {
                                            auto td = std::make_unique<ToolDataPastePlacement>(nullptr);
                                            imp_interface->tool_update_data(std::move(td));
                                        }
                                    });
                        },
                        Glib::PRIORITY_HIGH_IDLE);
            }
        }
        else if (any_of(args.action, {InToolActionID::RMB, InToolActionID::CANCEL})) {
            return ToolResponse::end();
        }
    }
    else {
        if (args.type == ToolEventType::DATA) {
            auto data = dynamic_cast<ToolDataPastePlacement *>(args.data.get());
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
