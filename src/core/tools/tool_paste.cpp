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
#include "core/buffer.hpp"
#include <iostream>
#include <gtkmm.h>
#include "core/tool_id.hpp"
#include "util/picture_util.hpp"

namespace horizon {

ToolPaste::ToolPaste(IDocument *c, ToolID tid) : ToolBase(c, tid), ToolHelperMove(c, tid), ToolHelperMerge(c, tid)
{
}

bool ToolPaste::can_begin()
{
    if (tool_id == ToolID::PASTE)
        return true;
    else
        return selection.size();
}

bool ToolPaste::is_specific()
{
    return tool_id == ToolID::DUPLICATE;
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
    if (doc.r->get_layer_provider()->get_layers().count(la) == 0) {
        la = 0;
    }
}

void ToolPaste::apply_shift(Coordi &c, const Coordi &cursor_pos)
{
    c += shift;
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
    Coordi cursor_pos = j.at("cursor_pos").get<std::vector<int64_t>>();
    printf("Core %p\n", doc.r);
    selection.clear();
    shift = cursor_pos_canvas - cursor_pos;

    std::map<UUID, const UUID> text_xlat;
    if (j.count("texts") && doc.r->has_object_type(ObjectType::TEXT)) {
        const json &o = j["texts"];
        for (auto it = o.cbegin(); it != o.cend(); ++it) {
            auto u = UUID::random();
            text_xlat.emplace(it.key(), u);
            auto x = doc.r->insert_text(u);
            *x = Text(u, it.value());
            fix_layer(x->layer);
            apply_shift(x->placement.shift, cursor_pos_canvas);
            selection.emplace(u, ObjectType::TEXT);
        }
    }
    if (j.count("dimensions") && doc.r->has_object_type(ObjectType::DIMENSION)) {
        const json &o = j["dimensions"];
        for (auto it = o.cbegin(); it != o.cend(); ++it) {
            auto u = UUID::random();
            auto x = doc.r->insert_dimension(u);
            *x = Dimension(u, it.value());
            apply_shift(x->p0, cursor_pos_canvas);
            apply_shift(x->p1, cursor_pos_canvas);
            selection.emplace(u, ObjectType::DIMENSION, 0);
            selection.emplace(u, ObjectType::DIMENSION, 1);
        }
    }
    std::map<UUID, const UUID> junction_xlat;
    if (j.count("junctions")) {
        const json &o = j["junctions"];
        for (auto it = o.cbegin(); it != o.cend(); ++it) {
            auto u = UUID::random();
            junction_xlat.emplace(it.key(), u);
            auto x = doc.r->insert_junction(u);
            *x = Junction(u, it.value());
            apply_shift(x->position, cursor_pos_canvas);
            selection.emplace(u, ObjectType::JUNCTION);
        }
    }
    if (j.count("lines")) {
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
    if (j.count("arcs")) {
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
        int max_name = doc.k->get_package()->get_max_pad_name();
        for (auto it = o.cbegin(); it != o.cend(); ++it) {
            auto u = UUID::random();
            auto &x = doc.k->get_package()->pads.emplace(u, Pad(u, it.value(), *doc.r->get_pool())).first->second;
            apply_shift(x.placement.shift, cursor_pos_canvas);
            pads.push_back(&x);
            selection.emplace(u, ObjectType::PAD);
        }
        doc.k->get_package()->apply_parameter_set({});
        if (max_name > 0) {
            std::sort(pads.begin(), pads.end(),
                      [](const Pad *a, const Pad *b) { return strcmp_natural(a->name, b->name) < 0; });
            for (auto pad : pads) {
                max_name++;
                pad->name = std::to_string(max_name);
            }
        }
    }
    if (j.count("holes")) {
        const json &o = j["holes"];
        for (auto it = o.cbegin(); it != o.cend(); ++it) {
            auto u = UUID::random();
            auto x = doc.r->insert_hole(u);
            *x = Hole(u, it.value());
            apply_shift(x->placement.shift, cursor_pos_canvas);
            selection.emplace(u, ObjectType::HOLE);
        }
    }
    if (j.count("polygons")) {
        const json &o = j["polygons"];
        for (auto it = o.cbegin(); it != o.cend(); ++it) {
            auto u = UUID::random();
            auto x = doc.r->insert_polygon(u);
            *x = Polygon(u, it.value());
            int vertex = 0;
            for (auto &itv : x->vertices) {
                itv.arc_center += shift;
                itv.position += shift;
                if (itv.type == Polygon::Vertex::Type::ARC)
                    selection.emplace(u, ObjectType::POLYGON_ARC_CENTER, vertex);
                selection.emplace(u, ObjectType::POLYGON_VERTEX, vertex++);
            }
        }
    }
    std::map<UUID, const UUID> net_xlat;
    if (j.count("nets") && doc.r->get_block()) {
        const json &o = j["nets"];
        auto block = doc.r->get_block();
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
            }
            if (net_new)
                net_xlat.emplace(it.key(), net_new->uuid);
        }
    }

    std::map<UUID, const UUID> component_xlat;
    if (j.count("components") && doc.c) {
        const json &o = j["components"];
        auto block = doc.c->get_schematic()->block;
        for (auto it = o.cbegin(); it != o.cend(); ++it) {
            auto u = UUID::random();
            component_xlat.emplace(it.key(), u);
            Component comp(u, it.value(), *doc.r->get_pool());
            for (auto &it_conn : comp.connections) {
                it_conn.second.net = &block->nets.at(net_xlat.at(it_conn.second.net.uuid));
            }
            block->components.emplace(u, comp);
        }
    }
    std::map<UUID, const UUID> symbol_xlat;
    if (j.count("symbols") && doc.c) {
        const json &o = j["symbols"];
        auto sheet = doc.c->get_sheet();
        auto block = doc.c->get_schematic()->block;
        for (auto it = o.cbegin(); it != o.cend(); ++it) {
            auto u = UUID::random();
            symbol_xlat.emplace(it.key(), u);
            SchematicSymbol sym(u, it.value(), *doc.r->get_pool());
            sym.component = &block->components.at(component_xlat.at(sym.component.uuid));
            sym.gate = &sym.component->entity->gates.at(sym.gate.uuid);
            auto x = &sheet->symbols.emplace(u, sym).first->second;
            for (auto &it_txt : x->texts) {
                it_txt = doc.r->get_text(text_xlat.at(it_txt.uuid));
            }
            sheet->expand_symbol(u, *doc.c->get_schematic());
            x->placement.shift += shift;
            selection.emplace(u, ObjectType::SCHEMATIC_SYMBOL);
        }
    }

    std::map<UUID, const UUID> bus_ripper_xlat;
    if (j.count("bus_rippers") && doc.c) {
        const json &o = j["bus_rippers"];
        auto sheet = doc.c->get_sheet();
        auto block = doc.c->get_block();
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
    if (j.count("net_lines") && doc.c) {
        const json &o = j["net_lines"];
        auto sheet = doc.c->get_sheet();
        for (auto it = o.cbegin(); it != o.cend(); ++it) {
            auto u = UUID::random();
            LineNet line(u, it.value());
            auto update_net_line_conn = [&junction_xlat, &symbol_xlat, &bus_ripper_xlat,
                                         sheet](LineNet::Connection &c) {
                if (c.junc.uuid) {
                    c.junc = &sheet->junctions.at(junction_xlat.at(c.junc.uuid));
                }
                else if (c.symbol.uuid) {
                    c.symbol = &sheet->symbols.at(symbol_xlat.at(c.symbol.uuid));
                    c.pin = &c.symbol->symbol.pins.at(c.pin.uuid);
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
        auto block = doc.c->get_block();
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
        auto block = doc.c->get_schematic()->block;
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
            auto x = &doc.a->get_padstack()->shapes.emplace(u, Shape(u, it.value())).first->second;
            x->placement.shift += shift;
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
                                       std::forward_as_tuple(u, it.value()))
                              .first->second;
            if (auto ps = doc.b->get_via_padstack_provider()->get_padstack(x->vpp_padstack.uuid)) {
                x->vpp_padstack = ps;
                x->padstack = *ps;
                x->padstack.apply_parameter_set(x->parameter_set);
            }
            if (brd->block->nets.count(x->net_set.uuid)) {
                x->net_set = &brd->block->nets.at(x->net_set.uuid);
            }
            else {
                x->net_set = nullptr;
            }
            x->junction = &brd->junctions.at(junction_xlat.at(x->junction.uuid));
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
                                       std::forward_as_tuple(u, it.value(), nullptr, doc.r->get_pool()))
                              .first->second;
            if (net_xlat.count(x->net.uuid)) {
                x->net = &brd->block->nets.at(net_xlat.at(x->net.uuid));
            }
            else {
                x->net = nullptr;
            }
            x->padstack.apply_parameter_set(x->parameter_set);
            apply_shift(x->placement.shift, cursor_pos_canvas);
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
                apply_shift(x->placement.shift, cursor_pos_canvas);
                selection.emplace(u, ObjectType::BOARD_PANEL);
            }
        }
    }
    if (selection.size() == 0) {
        imp->tool_bar_flash("Empty buffer");
        return ToolResponse::revert();
    }
    move_init(cursor_pos_canvas);
    update_tip();
    if (doc.c) {
        auto sch = doc.c->get_schematic();
        sch->expand(true);
        for (auto &it : sch->sheets) {
            it.second.warnings.clear();
        }
    }
    return ToolResponse();
}

ToolResponse ToolPaste::begin(const ToolArgs &args)
{
    update_tip();
    if (auto data = dynamic_cast<ToolDataPaste *>(args.data.get())) {
        paste_data = data->paste_data;
        return begin_paste(paste_data, args.coords);
    }

    if (tool_id == ToolID::DUPLICATE) {
        Buffer buffer(doc);
        buffer.load(args.selection);
        paste_data = buffer.serialize();
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
        return;
    }
    auto delta = get_delta();

    std::string s =
            "<b>LMB:</b>place <b>RMB:</b>cancel <b>r:</b>rotate "
            "<b>e:</b>mirror <b>/:</b>restrict <i>"
            + coord_to_string(delta, true) + " " + restrict_mode_to_string() + "</i> ";
    imp->tool_bar_set_tip(s);
}

ToolResponse ToolPaste::update(const ToolArgs &args)
{
    if (paste_data == nullptr && pic == nullptr) { // wait for data
        if (args.type == ToolEventType::DATA) {
            auto data = dynamic_cast<ToolDataPaste *>(args.data.get());
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
    else {
        if (args.type == ToolEventType::MOVE) {
            move_do_cursor(args.coords);
            update_tip();
            return ToolResponse::fast();
        }
        else if (args.type == ToolEventType::CLICK || (is_transient && args.type == ToolEventType::CLICK_RELEASE)) {
            if (args.button == 1) {
                if (pic) {
                    return ToolResponse::commit();
                }
                merge_selected_junctions();
                for (const auto &it : selection) {
                    if (it.type == ObjectType::SCHEMATIC_SYMBOL) {
                        auto sym = doc.c->get_schematic_symbol(it.uuid);
                        doc.c->get_schematic()->autoconnect_symbol(doc.c->get_sheet(), sym);
                        if (sym->component->connections.size() == 0) {
                            doc.c->get_schematic()->place_bipole_on_line(doc.c->get_sheet(), sym);
                        }
                    }
                }
                return ToolResponse::next(ToolResponse::Result::COMMIT, tool_id,
                                          std::make_unique<ToolDataPaste>(paste_data));
            }
            else {
                return ToolResponse::revert();
            }
        }
        else if (args.type == ToolEventType::KEY) {
            if (args.key == GDK_KEY_Escape) {
                return ToolResponse::revert();
            }
            else if (args.key == GDK_KEY_slash) {
                cycle_restrict_mode();
                move_do_cursor(args.coords);
            }
            else if (args.key == GDK_KEY_r || args.key == GDK_KEY_e) {
                bool rotate = args.key == GDK_KEY_r;
                move_mirror_or_rotate(args.coords, rotate);
            }
        }
    }
    update_tip();
    return ToolResponse();
}
} // namespace horizon
