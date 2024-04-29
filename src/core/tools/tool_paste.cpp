#include "tool_paste.hpp"
#include "document/idocument_package.hpp"
#include "pool/package.hpp"
#include "document/idocument_padstack.hpp"
#include "pool/padstack.hpp"
#include "document/idocument_schematic.hpp"
#include "schematic/schematic.hpp"
#include "document/idocument_board.hpp"
#include "board/board.hpp"
#include "imp/imp_interface.hpp"
#include "pool/entity.hpp"
#include "util/util.hpp"
#include "util/geom_util.hpp"
#include "util/selection_util.hpp"
#include "core/clipboard/clipboard.hpp"
#include <iostream>
#include <gtkmm.h>
#include "core/tool_id.hpp"
#include "util/picture_util.hpp"
#include "blocks/blocks_schematic.hpp"
#include "pool/pool.hpp"
#include <filesystem>
#include "tool_data_pool_updated.hpp"
#include "util/pasted_package.hpp"
#include "util/range_util.hpp"
#include "board/board_layers.hpp"

namespace horizon {
namespace fs = std::filesystem;
ToolPaste::ToolPaste(IDocument *c, ToolID tid) : ToolBase(c, tid), ToolHelperMove(c, tid), ToolHelperMerge(c, tid)
{
}

bool ToolPaste::can_begin()
{
    if (tool_id == ToolID::PASTE)
        return true;
    else if (tool_id == ToolID::PASTE_RELATIVE)
        return doc.b && (sel_count_type(selection, ObjectType::BOARD_PACKAGE) == 1);
    else
        return selection.size();
}

bool ToolPaste::is_specific()
{
    return tool_id != ToolID::PASTE;
}

class JunctionProvider : public ObjectProvider {
public:
    JunctionProvider(IDocument *c, const std::map<UUID, const UUID> &xj) : doc(c), junction_xlat(xj)
    {
    }
    virtual ~JunctionProvider()
    {
    }

    Junction *get_junction(const UUID &uu) override
    {
        return doc->get_junction(junction_xlat.at(uu));
    }

private:
    IDocument *doc;
    const std::map<UUID, const UUID> &junction_xlat;
};

void ToolPaste::fix_layer(int &la)
{
    if (doc.r->get_layer_provider().get_layers().count(la) == 0) {
        la = 0;
    }
    if (doc.b && ref_pkg && target_pkg && ref_pkg->flip != target_pkg->flip) {
        doc.b->get_board()->flip_package_layer(la);
    }
}

void ToolPaste::transform(Coordi &c) const
{
    Placement p{c};
    transform(p, ObjectType::BOARD_PACKAGE);
    c = p.shift;
}

void ToolPaste::transform(Placement &p, ObjectType type) const
{
    if (target_pkg && ref_pkg) {
        switch (type) {
        case ObjectType::BOARD_PACKAGE:
        case ObjectType::BOARD_DECAL:
            p = transform_package_placement_to_new_reference(p, ref_pkg->placement, target_pkg->placement);
            break;

        default:
            p = transform_text_placement_to_new_reference(p, ref_pkg->placement, target_pkg->placement);
        }
    }
    else {
        p.shift += shift;
    }
}

class ToolDataPaste : public ToolData {
public:
    ToolDataPaste(const json &j) : paste_data(j)
    {
    }
    ToolDataPaste(Glib::RefPtr<Gdk::Pixbuf> pb) : pixbuf(pb)
    {
    }
    json paste_data;
    Glib::RefPtr<Gdk::Pixbuf> pixbuf;
};

ToolResponse ToolPaste::begin_paste(const json &j, const Coordi &cursor_pos_canvas)
{
    ItemSet items_needed;
    if (doc.c && j.count("pool_items_needed") && j.count("pool_base_path")
        && j.at("pool_base_path").get<std::string>() != doc.r->get_pool().get_base_path()) {
        auto &db = doc.r->get_pool().get_db();
        db.execute("CREATE TEMP TABLE 'paste_items_needed' ('type' TEXT NOT NULL, 'uuid' TEXT NOT NULL);");
        {
            SQLite::Query q_insert(db, "INSERT INTO paste_items_NEEDED VALUES (?, ?)");
            for (const auto &it : j.at("pool_items_needed")) {
                q_insert.reset();
                q_insert.bind(1, it.at(0).get<std::string>());
                q_insert.bind(2, it.at(1).get<std::string>());
                q_insert.step();
            }
        }
        SQLite::Query q(db,
                        "SELECT paste_items_needed.type, paste_items_needed.uuid FROM paste_items_needed LEFT JOIN "
                        "all_items_view USING (uuid) WHERE "
                        "all_items_view.uuid IS NULL");
        while (q.step()) {
            items_needed.emplace(object_type_lut.lookup(q.get<std::string>(0)), q.get<std::string>(1));
        }
        db.execute("DROP TABLE paste_items_needed");
    }
    if (items_needed.size()) {
        Pool paste_pool(j.at("pool_base_path").get<std::string>());
        for (const auto &[type, uu] : items_needed) {
            const auto src_filename = fs::u8path(paste_pool.get_filename(type, uu));
            const auto dest_dir = fs::u8path(doc.r->get_pool().get_base_path()) / Pool::type_names.at(type) / "cache";
            fs::create_directories(dest_dir);
            if (type == ObjectType::PACKAGE) {
                const auto dest_pkg_dir = dest_dir / (((std::string)uu));
                fs::create_directories(dest_pkg_dir);
                fs::copy(src_filename, dest_pkg_dir / "package.json");
            }
            else if (type == ObjectType::PADSTACK) {
                SQLite::Query q(paste_pool.db, "SELECT package FROM padstacks WHERE uuid = ?");
                q.bind(1, uu);
                if (q.step()) {
                    const UUID package_uuid = q.get<std::string>(0);
                    if (package_uuid) {
                        const auto dest_padstack_dir = fs::u8path(doc.r->get_pool().get_base_path())
                                                       / Pool::type_names.at(ObjectType::PACKAGE) / "cache"
                                                       / (std::string)package_uuid / "padstacks";
                        fs::create_directories(dest_padstack_dir);
                        fs::copy(src_filename, dest_padstack_dir / (((std::string)uu) + ".json"));
                    }
                    else {
                        fs::copy(src_filename, dest_dir / (((std::string)uu) + ".json"));
                    }
                }
                else {
                    throw std::runtime_error("padstack not found???");
                }
            }
            else {
                fs::copy(src_filename, dest_dir / (((std::string)uu) + ".json"));
            }
        }
        imp->pool_update({});
        pool_update_pending = true;
        update_tip();
        imp->tool_bar_set_actions({
                {InToolActionID::RMB},
        });
        return ToolResponse();
    }
    else {
        return really_begin_paste(j, cursor_pos_canvas);
    }
}

ToolResponse ToolPaste::really_begin_paste(const json &j, const Coordi &cursor_pos_canvas)
{
    Coordi cursor_pos = j.at("cursor_pos").get<std::vector<int64_t>>();
    selection.clear();
    shift = cursor_pos_canvas - cursor_pos;
    std::map<UUID, PastedPackage> pkgs_from_paste;
    std::map<UUID, Net *> board_net_xlat;
    std::set<UUID> pkg_texts;
    if (target_pkg) {
        for (const auto &[k, v] : j.at("packages").items()) {
            pkgs_from_paste.emplace(std::piecewise_construct, std::forward_as_tuple(k), std::forward_as_tuple(k, v));
        }
        // find target pkg in pkgs_from_paste
        auto pkgs_from_paste_values = pkgs_from_paste | ranges::views::values;
        ref_pkg = find_if_ptr(pkgs_from_paste_values, [this](auto &x) { return x.tag == target_pkg->component->tag; });
        if (!ref_pkg) {
            imp->tool_bar_flash("reference package not found");
            return ToolResponse::end();
        }
        auto pkgs_from_board = doc.b->get_board()->packages | ranges::views::values;
        for (const auto &ppkg : pkgs_from_paste_values) {
            pkg_texts.insert(ppkg.texts.begin(), ppkg.texts.end());
            auto bpkg = find_if_ptr(pkgs_from_board, [this, &ppkg](const auto &x) {
                return x.component->tag == ppkg.tag && x.component->group == ref_pkg->group;
            });
            if (bpkg && bpkg->package.uuid == ppkg.package) {
                auto conns =
                        ppkg.connections
                        | ranges::views::filter([bpkg](const auto &x) { return bpkg->package.pads.count(x.first); })
                        | ranges::views::transform([bpkg](const auto &x) {
                              return std::make_pair(x.second, bpkg->package.pads.at(x.first).net);
                          })
                        | ranges::views::filter([](const auto &x) { return x.second; });
                board_net_xlat.insert(conns.begin(), conns.end());
            }
        }
    }

    std::map<UUID, const UUID> text_xlat;
    if (j.count("texts") && doc.r->has_object_type(ObjectType::TEXT)) {
        const json &o = j["texts"];
        for (auto it = o.cbegin(); it != o.cend(); ++it) {
            if (pkg_texts.count(it.key()))
                continue;
            auto u = UUID::random();
            text_xlat.emplace(it.key(), u);
            auto x = doc.r->insert_text(u);
            *x = Text(u, it.value());
            transform(x->placement, ObjectType::TEXT);
            fix_layer(x->layer);
            selection.emplace(u, ObjectType::TEXT);
        }
    }
    if (j.count("dimensions") && doc.r->has_object_type(ObjectType::DIMENSION)) {
        const json &o = j["dimensions"];
        for (auto it = o.cbegin(); it != o.cend(); ++it) {
            auto u = UUID::random();
            auto x = doc.r->insert_dimension(u);
            *x = Dimension(u, it.value());
            transform(x->p0);
            transform(x->p1);
            selection.emplace(u, ObjectType::DIMENSION, 0);
            selection.emplace(u, ObjectType::DIMENSION, 1);
        }
    }
    std::map<UUID, const UUID> junction_xlat;
    std::map<UUID, Net *> junction_nets;
    if (j.count("junctions")) {
        const json &o = j["junctions"];
        for (auto it = o.cbegin(); it != o.cend(); ++it) {
            auto u = UUID::random();
            junction_xlat.emplace(it.key(), u);
            auto x = doc.r->insert_junction(u);
            const auto &j_ju = it.value();
            *x = Junction(u, j_ju);
            if (doc.c && j_ju.count("net")) {
                const auto net_uu = UUID(j_ju.at("net").get<std::string>());
                auto &block = *doc.c->get_current_block();
                if (block.nets.count(net_uu))
                    junction_nets.emplace(u, &block.nets.at(net_uu));
            }
            transform(x->position);
            selection.emplace(u, ObjectType::JUNCTION);
        }
    }
    if (j.count("lines") && doc.r->has_object_type(ObjectType::LINE)) {
        const json &o = j["lines"];
        for (auto it = o.cbegin(); it != o.cend(); ++it) {
            auto u = UUID::random();
            auto x = doc.r->insert_line(u);
            JunctionProvider p(doc.r, junction_xlat);
            *x = Line(u, it.value(), p);
            fix_layer(x->layer);
            selection.emplace(u, ObjectType::LINE);
        }
    }
    if (j.count("arcs") && doc.r->has_object_type(ObjectType::ARC)) {
        const json &o = j["arcs"];
        for (auto it = o.cbegin(); it != o.cend(); ++it) {
            auto u = UUID::random();
            auto x = doc.r->insert_arc(u);
            JunctionProvider p(doc.r, junction_xlat);
            *x = Arc(u, it.value(), p);
            fix_layer(x->layer);
            selection.emplace(u, ObjectType::ARC);
        }
    }
    if (j.count("pads") && doc.k) {
        const json &o = j["pads"];
        std::vector<Pad *> pads;
        int max_name = doc.k->get_package().get_max_pad_name();
        for (auto it = o.cbegin(); it != o.cend(); ++it) {
            auto u = UUID::random();
            auto &x = doc.k->get_package().pads.emplace(u, Pad(u, it.value(), doc.r->get_pool())).first->second;
            transform(x.placement, ObjectType::PAD);
            pads.push_back(&x);
            selection.emplace(u, ObjectType::PAD);
        }
        doc.k->get_package().apply_parameter_set({});
        if (max_name > 0) {
            std::sort(pads.begin(), pads.end(),
                      [](const Pad *a, const Pad *b) { return strcmp_natural(a->name, b->name) < 0; });
            for (auto pad : pads) {
                max_name++;
                pad->name = std::to_string(max_name);
            }
        }
    }
    if (j.count("holes") && doc.a) {
        const json &o = j["holes"];
        for (auto it = o.cbegin(); it != o.cend(); ++it) {
            auto u = UUID::random();
            auto x = doc.r->insert_hole(u);
            *x = Hole(u, it.value());
            transform(x->placement, ObjectType::HOLE);
            selection.emplace(u, ObjectType::HOLE);
        }
    }
    std::map<UUID, Polygon *> polygon_xlat;
    if (j.count("polygons") && doc.r->has_object_type(ObjectType::POLYGON)) {
        const json &o = j["polygons"];
        for (auto it = o.cbegin(); it != o.cend(); ++it) {
            auto u = UUID::random();
            auto x = doc.r->insert_polygon(u);
            *x = Polygon(u, it.value());
            int vertex = 0;
            for (auto &itv : x->vertices) {
                transform(itv.arc_center);
                transform(itv.position);
                if (itv.type == Polygon::Vertex::Type::ARC)
                    selection.emplace(u, ObjectType::POLYGON_ARC_CENTER, vertex);
                selection.emplace(u, ObjectType::POLYGON_VERTEX, vertex++);
            }
            polygon_xlat.emplace(it.key(), x);
            fix_layer(x->layer);
        }
    }
    if (j.count("planes") && doc.b) {
        for (const auto &[uu, it] : j.at("planes").items()) {
            Plane plane{UUID::random(), it, nullptr};
            if (polygon_xlat.count(plane.polygon.uuid) && board_net_xlat.count(plane.net.uuid)) {
                plane.polygon = polygon_xlat.at(plane.polygon.uuid);
                plane.net = board_net_xlat.at(plane.net.uuid);
                auto &x = doc.b->get_board()->planes.emplace(plane.uuid, plane).first->second;
                x.polygon->usage = &x;
            }
        }
    }
    std::map<UUID, const UUID> net_xlat;
    std::set<Net *> new_nets;
    if (j.count("nets") && doc.c && doc.c->get_current_block()) {
        const json &o = j["nets"];
        auto block = doc.c->get_current_block();
        for (auto it = o.cbegin(); it != o.cend(); ++it) {
            Net net_from_json(it.key(), it.value());
            std::string net_name = net_from_json.name;
            std::cout << "paste net " << net_name << std::endl;
            Net *net_new = nullptr;
            if (net_name.size()) {
                for (auto &it_net : block->nets) {
                    if (it_net.second.name == net_name) {
                        net_new = &it_net.second;
                        break;
                    }
                }
            }
            if (!net_new && doc.c) {
                net_new = block->insert_net();
                net_new->is_power = net_from_json.is_power;
                net_new->name = net_name;
                net_new->power_symbol_style = net_from_json.power_symbol_style;
                if (net_from_json.diffpair_primary) {
                    net_new->diffpair_primary = true;
                    net_new->diffpair.uuid = net_from_json.diffpair.uuid;
                }
                new_nets.insert(net_new);
            }
            if (net_new)
                net_xlat.emplace(it.key(), net_new->uuid);
        }
        for (auto net : new_nets) {
            if (net->diffpair_primary) {
                if (net_xlat.count(net->diffpair.uuid))
                    net->diffpair = &block->nets.at(net_xlat.at(net->diffpair.uuid));
                else
                    net->diffpair_primary = false;
            }
        }
        block->update_diffpairs();
    }

    std::map<UUID, const UUID> component_xlat;
    if (j.count("components") && doc.c) {
        const json &o = j["components"];
        auto block = doc.c->get_current_schematic()->block;
        for (auto it = o.cbegin(); it != o.cend(); ++it) {
            auto u = UUID::random();
            component_xlat.emplace(it.key(), u);
            Component comp(u, it.value(), doc.r->get_pool_caching());
            for (auto &it_conn : comp.connections) {
                if (it_conn.second.net.uuid)
                    it_conn.second.net = &block->nets.at(net_xlat.at(it_conn.second.net.uuid));
            }
            block->components.emplace(u, comp);
            if (comp.group && !block->group_names.count(comp.group)) {
                block->group_names.emplace(comp.group,
                                           j.at("group_names").at((std::string)comp.group).get<std::string>());
            }
            if (comp.tag && !block->tag_names.count(comp.tag)) {
                block->tag_names.emplace(comp.tag, j.at("tag_names").at((std::string)comp.tag).get<std::string>());
            }
        }
    }
    std::map<UUID, const UUID> symbol_xlat;
    if (j.count("symbols") && doc.c) {
        const json &o = j["symbols"];
        auto sheet = doc.c->get_sheet();
        auto block = doc.c->get_current_schematic()->block;
        for (auto it = o.cbegin(); it != o.cend(); ++it) {
            auto u = UUID::random();
            symbol_xlat.emplace(it.key(), u);
            SchematicSymbol sym(u, it.value(), doc.r->get_pool_caching());
            sym.component = &block->components.at(component_xlat.at(sym.component.uuid));
            sym.gate = &sym.component->entity->gates.at(sym.gate.uuid);
            auto x = &sheet->symbols.emplace(u, sym).first->second;
            for (auto &it_txt : x->texts) {
                it_txt = doc.r->get_text(text_xlat.at(it_txt.uuid));
            }
            sheet->expand_symbol(u, *doc.c->get_current_schematic());
            transform(x->placement, ObjectType::SCHEMATIC_SYMBOL);
            selection.emplace(u, ObjectType::SCHEMATIC_SYMBOL);
        }
    }

    std::map<UUID, const UUID> inst_xlat;
    if (j.count("block_instances") && doc.c) {
        auto block = doc.c->get_current_schematic()->block;
        auto &blocks = doc.c->get_blocks();
        for (const auto &[key, value] : j.at("block_instances").items()) {
            const auto block_uu = BlockInstance::peek_block_uuid(value);
            if (blocks.blocks.count(block_uu)) {
                if (doc.c->get_top_block()->can_add_block_instance(doc.c->get_current_block()->uuid, block_uu)) {
                    auto u = UUID::random();
                    inst_xlat.emplace(key, u);
                    BlockInstance inst(u, value, doc.c->get_blocks());
                    for (auto &it_conn : inst.connections) {
                        it_conn.second.net = &block->nets.at(net_xlat.at(it_conn.second.net.uuid));
                    }
                    block->block_instances.emplace(u, inst);
                }
                else {
                    imp->tool_bar_flash("can't paste this block here");
                }
            }
        }
    }

    std::map<UUID, const UUID> block_symbol_xlat;
    if (j.count("block_symbols") && doc.c) {
        auto sheet = doc.c->get_sheet();
        auto block = doc.c->get_current_schematic()->block;
        for (const auto &[key, value] : j.at("block_symbols").items()) {
            const auto inst_uu = SchematicBlockSymbol::peek_block_instance_uuid(value);
            if (inst_xlat.count(inst_uu)) {
                auto &inst = block->block_instances.at(inst_xlat.at(inst_uu));
                auto u = UUID::random();
                block_symbol_xlat.emplace(key, u);
                SchematicBlockSymbol sym(u, doc.c->get_blocks().get_block_symbol(inst.block->uuid), inst);
                sym.schematic = &doc.c->get_blocks().get_schematic(inst.block->uuid);
                sym.placement = Placement(value.at("placement"));
                auto &x = sheet->block_symbols.emplace(u, sym).first->second;
                sheet->expand_block_symbol(u, *doc.c->get_current_schematic());
                doc.c->get_top_schematic()->update_sheet_mapping();
                transform(x.placement, ObjectType::SCHEMATIC_BLOCK_SYMBOL);
                selection.emplace(u, ObjectType::SCHEMATIC_BLOCK_SYMBOL);
            }
        }
    }

    std::map<UUID, const UUID> bus_ripper_xlat;
    if (j.count("bus_rippers") && doc.c) {
        const json &o = j["bus_rippers"];
        auto sheet = doc.c->get_sheet();
        auto block = doc.c->get_current_block();
        for (auto it = o.cbegin(); it != o.cend(); ++it) {
            auto u = UUID::random();
            BusRipper rip(u, it.value());
            if (block->buses.count(rip.bus.uuid)) {
                bus_ripper_xlat.emplace(it.key(), u);
                auto &x = doc.c->get_sheet()->bus_rippers.emplace(u, rip).first->second;
                x.junction = &sheet->junctions.at(junction_xlat.at(x.junction.uuid));
                x.bus.update(block->buses);
                x.bus_member.update(x.bus->members);
                selection.emplace(u, ObjectType::BUS_RIPPER);
            }
        }
    }
    std::map<UUID, Net *> net_line_nets;
    if (j.count("net_lines") && doc.c) {
        const json &o = j["net_lines"];
        auto sheet = doc.c->get_sheet();
        for (auto it = o.cbegin(); it != o.cend(); ++it) {
            auto u = UUID::random();
            const auto &j_line = it.value();
            LineNet line(u, j_line);
            auto update_net_line_conn = [&junction_xlat, &symbol_xlat, &block_symbol_xlat, &bus_ripper_xlat,
                                         sheet](LineNet::Connection &c) {
                if (c.junc.uuid) {
                    c.junc = &sheet->junctions.at(junction_xlat.at(c.junc.uuid));
                }
                else if (c.symbol.uuid) {
                    c.symbol = &sheet->symbols.at(symbol_xlat.at(c.symbol.uuid));
                    c.pin = &c.symbol->symbol.pins.at(c.pin.uuid);
                }
                else if (c.block_symbol.uuid) {
                    if (block_symbol_xlat.count(c.block_symbol.uuid)) {
                        c.block_symbol = &sheet->block_symbols.at(block_symbol_xlat.at(c.block_symbol.uuid));
                        c.port = &c.block_symbol->symbol.ports.at(c.port.uuid);
                    }
                    else {
                        return false;
                    }
                }
                else if (c.bus_ripper.uuid) {
                    if (bus_ripper_xlat.count(c.bus_ripper.uuid))
                        c.bus_ripper = &sheet->bus_rippers.at(bus_ripper_xlat.at(c.bus_ripper.uuid));
                    else
                        return false;
                }
                return true;
            };
            if (update_net_line_conn(line.from) && update_net_line_conn(line.to)) {
                sheet->net_lines.emplace(u, line);
                if (j_line.count("net")) {
                    const auto net_uu = UUID(j_line.at("net").get<std::string>());
                    auto &block = *doc.c->get_current_block();
                    if (block.nets.count(net_uu))
                        net_line_nets.emplace(u, &block.nets.at(net_uu));
                }
                selection.emplace(u, ObjectType::LINE_NET);
            }
        }
    }
    if (j.count("net_labels") && doc.c) {
        const json &o = j["net_labels"];
        auto sheet = doc.c->get_sheet();
        for (auto it = o.cbegin(); it != o.cend(); ++it) {
            auto u = UUID::random();
            auto x = &doc.c->get_sheet()->net_labels.emplace(u, NetLabel(u, it.value())).first->second;
            x->junction = &sheet->junctions.at(junction_xlat.at(x->junction.uuid));
            selection.emplace(u, ObjectType::NET_LABEL);
        }
    }
    if (j.count("bus_labels") && doc.c) {
        const json &o = j["bus_labels"];
        auto sheet = doc.c->get_sheet();
        auto block = doc.c->get_current_block();
        for (auto it = o.cbegin(); it != o.cend(); ++it) {
            auto u = UUID::random();
            BusLabel la(u, it.value());
            if (block->buses.count(la.bus.uuid)) {
                auto &x = doc.c->get_sheet()->bus_labels.emplace(u, la).first->second;
                x.junction = &sheet->junctions.at(junction_xlat.at(x.junction.uuid));
                x.bus.update(block->buses);
                selection.emplace(u, ObjectType::BUS_LABEL);
            }
        }
    }
    if (j.count("power_symbols") && doc.c) {
        const json &o = j["power_symbols"];
        auto sheet = doc.c->get_sheet();
        auto block = doc.c->get_current_schematic()->block;
        for (auto it = o.cbegin(); it != o.cend(); ++it) {
            auto u = UUID::random();
            auto x = &doc.c->get_sheet()->power_symbols.emplace(u, PowerSymbol(u, it.value())).first->second;
            x->junction = &sheet->junctions.at(junction_xlat.at(x->junction.uuid));
            x->net = &block->nets.at(net_xlat.at(x->net.uuid));
            selection.emplace(u, ObjectType::POWER_SYMBOL);
        }
    }
    if (j.count("shapes") && doc.a) {
        const json &o = j["shapes"];
        for (auto it = o.cbegin(); it != o.cend(); ++it) {
            auto u = UUID::random();
            auto &x = doc.a->get_padstack().shapes.emplace(u, Shape(u, it.value())).first->second;
            transform(x.placement, ObjectType::SHAPE);
            selection.emplace(u, ObjectType::SHAPE);
        }
    }
    if (j.count("vias") && doc.b) {
        const json &o = j["vias"];
        for (auto it = o.cbegin(); it != o.cend(); ++it) {
            auto u = UUID::random();
            auto brd = doc.b->get_board();
            auto x = &brd->vias
                              .emplace(std::piecewise_construct, std::forward_as_tuple(u),
                                       std::forward_as_tuple(u, it.value(), doc.b->get_pool_caching(), nullptr))
                              .first->second;
            x->expand(*brd);
            if (brd->block->nets.count(x->net_set.uuid)) {
                x->net_set = &brd->block->nets.at(x->net_set.uuid);
            }
            else if (board_net_xlat.count(x->net_set.uuid)) {
                x->net_set = board_net_xlat.at(x->net_set.uuid);
            }
            else {
                x->net_set = nullptr;
            }
            if (x->net_set)
                nets.insert(x->net_set->uuid);
            x->junction = &brd->junctions.at(junction_xlat.at(x->junction.uuid));
            x->junction->net = x->net_set;
            selection.emplace(u, ObjectType::VIA);
        }
    }
    if (j.count("tracks") && doc.b) {
        const json &o = j["tracks"];
        for (auto it = o.cbegin(); it != o.cend(); ++it) {
            auto u = UUID::random();
            auto brd = doc.b->get_board();
            auto x = &brd->tracks
                              .emplace(std::piecewise_construct, std::forward_as_tuple(u),
                                       std::forward_as_tuple(u, it.value()))
                              .first->second;
            x->to.junc = &brd->junctions.at(junction_xlat.at(x->to.junc.uuid));
            x->from.junc = &brd->junctions.at(junction_xlat.at(x->from.junc.uuid));
            if (x->is_arc())
                transform(x->center.value());
            fix_layer(x->layer);
            struct ConnInfo {
                ConnInfo(const std::string &w, Track::Connection &c) : where(w), conn(c)
                {
                }
                const std::string where;
                Track::Connection &conn;
            };
            std::vector<ConnInfo> conns = {{"from_pad", x->from}, {"to_pad", x->to}};
            auto board_pkgs = brd->packages | ranges::views::values;
            for (auto &conn_info : conns) {
                if (it.value().count(conn_info.where)) {
                    Track::Connection conn{it.value().at(conn_info.where)};
                    if (pkgs_from_paste.count(conn.package.uuid)) {
                        const auto &ppkg = pkgs_from_paste.at(conn.package.uuid);
                        auto bpkg = find_if_ptr(board_pkgs, [this, &ppkg](auto &k) {
                            return k.component->tag == ppkg.tag && k.component->group == target_pkg->component->group;
                        });
                        if (bpkg && bpkg->package.pads.count(conn.pad.uuid)) {
                            auto &pad = bpkg->package.pads.at(conn.pad.uuid);
                            conn_info.conn.connect(bpkg, &pad);
                            if (pad.net)
                                doc.b->get_board()->airwires_expand.insert(pad.net->uuid);
                            doc.b->get_board()->expand_flags |= Board::EXPAND_PROPAGATE_NETS | Board::EXPAND_AIRWIRES;
                        }
                    }
                }
            }
            selection.emplace(u, ObjectType::TRACK);
        }
    }
    if (j.count("board_holes") && doc.b) {
        const json &o = j["board_holes"];
        for (auto it = o.cbegin(); it != o.cend(); ++it) {
            auto u = UUID::random();
            auto brd = doc.b->get_board();
            auto x = &brd->holes
                              .emplace(std::piecewise_construct, std::forward_as_tuple(u),
                                       std::forward_as_tuple(u, it.value(), nullptr, doc.r->get_pool_caching()))
                              .first->second;
            if (net_xlat.count(x->net.uuid)) {
                x->net = &brd->block->nets.at(net_xlat.at(x->net.uuid));
            }
            else {
                x->net = nullptr;
            }
            x->padstack.apply_parameter_set(x->parameter_set);
            transform(x->placement, ObjectType::BOARD_HOLE);
            selection.emplace(u, ObjectType::BOARD_HOLE);
        }
    }
    if (j.count("board_panels") && doc.b) {
        const json &o = j["board_panels"];
        for (auto it = o.cbegin(); it != o.cend(); ++it) {
            auto u = UUID::random();
            auto brd = doc.b->get_board();
            auto j2 = it.value();
            UUID inc_board = j2.at("included_board").get<std::string>();
            if (brd->included_boards.count(inc_board)) {
                auto x = &brd->board_panels
                                  .emplace(std::piecewise_construct, std::forward_as_tuple(u),
                                           std::forward_as_tuple(u, it.value(), *brd))
                                  .first->second;
                transform(x->placement, ObjectType::BOARD_PANEL);
                selection.emplace(u, ObjectType::BOARD_PANEL);
            }
        }
    }
    if (j.count("decals") && doc.b) {
        const json &o = j["decals"];
        for (auto it = o.cbegin(); it != o.cend(); ++it) {
            auto u = UUID::random();
            auto brd = doc.b->get_board();
            auto &x = brd->decals
                              .emplace(std::piecewise_construct, std::forward_as_tuple(u),
                                       std::forward_as_tuple(u, it.value(), doc.r->get_pool_caching(), *brd))
                              .first->second;
            transform(x.placement, ObjectType::BOARD_DECAL);
            x.set_flip(x.placement.mirror, *brd);
            selection.emplace(u, ObjectType::BOARD_DECAL);
        }
    }
    if (selection.size() == 0) {
        imp->tool_bar_flash("Empty buffer");
        return ToolResponse::revert();
    }
    move_init(cursor_pos_canvas);
    update_tip();
    if (doc.c) {
        auto sch = doc.c->get_current_schematic();
        sch->expand_connectivity(true);
        for (auto &it : sch->sheets) {
            it.second.warnings.clear();
        }
        auto &sheet = *doc.c->get_sheet();
        for (const auto &[uu, net] : junction_nets) {
            auto &ju = sheet.junctions.at(uu);
            if (!ju.net)
                ju.net = net;
        }
        for (const auto &[uu, net] : net_line_nets) {
            auto &li = sheet.net_lines.at(uu);
            if (!li.net)
                li.net = net;
        }
    }
    update_airwires();
    if (tool_id == ToolID::PASTE_RELATIVE)
        return ToolResponse::commit();
    imp->tool_bar_set_actions({
            {InToolActionID::LMB},
            {InToolActionID::RMB},
            {InToolActionID::ROTATE},
            {InToolActionID::MIRROR},
            {InToolActionID::RESTRICT},
    });
    return ToolResponse();
}

ToolResponse ToolPaste::begin(const ToolArgs &args)
{
    update_tip();
    if (auto data = dynamic_cast<ToolDataPaste *>(args.data.get())) {
        paste_data = data->paste_data;
        return begin_paste(paste_data, args.coords);
    }

    if (tool_id == ToolID::PASTE_RELATIVE) {
        target_pkg = &doc.b->get_board()->packages.at(sel_find_one(selection, ObjectType::BOARD_PACKAGE).uuid);
    }

    if (tool_id == ToolID::DUPLICATE) {
        auto clip = ClipboardBase::create(*doc.r);
        paste_data = clip->process(args.selection);
        paste_data["cursor_pos"] = args.coords.as_array();
        return begin_paste(paste_data, args.coords);
    }
    else {
        Gtk::Clipboard::get()->request_contents("imp-buffer", [this](const Gtk::SelectionData &sel_data) {
            if (sel_data.gobj() && sel_data.get_data_type() == "imp-buffer") {
                auto td = std::make_unique<ToolDataPaste>(json::parse(sel_data.get_data_as_string()));
                imp->tool_update_data(std::move(td));
            }
            else if (doc.r->has_object_type(ObjectType::PICTURE)) {
                Gtk::Clipboard::get()->request_image([this](const Glib::RefPtr<Gdk::Pixbuf> &pb) {
                    auto td = std::make_unique<ToolDataPaste>(pb);
                    imp->tool_update_data(std::move(td));
                });
            }
            else {
                auto td = std::make_unique<ToolDataPaste>(nullptr);
                imp->tool_update_data(std::move(td));
            }
        });
        return ToolResponse();
    }
}

void ToolPaste::update_tip()
{
    if (paste_data == nullptr && pic == nullptr) { // wait for data
        imp->tool_bar_set_tip("waiting for paste data");
    }
    else if (pool_update_pending) {
        imp->tool_bar_set_tip("waiting for pool update");
    }
    else {
        auto delta = get_delta();
        imp->tool_bar_set_tip(coord_to_string(delta, true) + " " + restrict_mode_to_string());
    }
}

ToolResponse ToolPaste::update(const ToolArgs &args)
{
    if (paste_data == nullptr && pic == nullptr) { // wait for data
        if (args.type == ToolEventType::DATA) {
            if (auto data = dynamic_cast<ToolDataPaste *>(args.data.get())) {
                if (data->paste_data != nullptr) {
                    paste_data = data->paste_data;
                    return begin_paste(paste_data, args.coords);
                }
                else if (data->pixbuf) {
                    pic = doc.r->insert_picture(UUID::random());
                    pic->placement.shift = args.coords;
                    pic->data = picture_data_from_pixbuf(data->pixbuf);
                    pic->data_uuid = pic->data->uuid;
                    float width = 10_mm;
                    pic->px_size = width / pic->data->width;
                    selection.clear();
                    selection.emplace(pic->uuid, ObjectType::PICTURE);
                    update_tip();
                    move_init(args.coords);
                }
                else {
                    imp->tool_bar_flash("Empty Buffer");
                    return ToolResponse::end();
                }
            }
        }
    }
    else if (pool_update_pending) {
        if (dynamic_cast<ToolDataPoolUpdated *>(args.data.get())) {
            pool_update_pending = false;
            return really_begin_paste(paste_data, args.coords);
        }
        else if (args.type == ToolEventType::ACTION
                 && any_of(args.action, {InToolActionID::RMB, InToolActionID::CANCEL})) {
            return ToolResponse::end();
        }
    }
    else {
        if (args.type == ToolEventType::MOVE) {
            move_do_cursor(args.coords);
            update_tip();
            update_airwires();
            return ToolResponse();
        }
        else if (args.type == ToolEventType::ACTION) {
            if (args.action == InToolActionID::LMB || (is_transient && args.action == InToolActionID::LMB_RELEASE)) {
                if (doc.b) {
                    auto &brd = *doc.b->get_board();
                    brd.airwires_expand = nets;
                    brd.expand_flags |= Board::EXPAND_AIRWIRES;
                }
                if (pic) {
                    return ToolResponse::commit();
                }
                merge_and_connect({});
                return ToolResponse::next(ToolResponse::Result::COMMIT, tool_id,
                                          std::make_unique<ToolDataPaste>(paste_data));
            }
            else if (any_of(args.action, {InToolActionID::RMB, InToolActionID::CANCEL})) {
                return ToolResponse::revert();
            }
            else if (args.action == InToolActionID::RESTRICT) {
                cycle_restrict_mode();
                move_do_cursor(args.coords);
            }
            else if (any_of(args.action, {InToolActionID::ROTATE, InToolActionID::MIRROR})) {
                move_mirror_or_rotate(args.coords, args.action == InToolActionID::ROTATE);
            }
        }
    }
    update_tip();
    return ToolResponse();
}

void ToolPaste::update_airwires()
{
    if (doc.b && nets.size()) {
        doc.b->get_board()->update_airwires(true, nets);
    }
}

} // namespace horizon
