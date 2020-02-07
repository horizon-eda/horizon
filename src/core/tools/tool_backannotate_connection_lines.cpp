#include "tool_backannotate_connection_lines.hpp"
#include "document/idocument_schematic.hpp"
#include "schematic/schematic.hpp"
#include "nlohmann/json.hpp"

namespace horizon {

void ToolBackannotateConnectionLines::ToolDataBackannotate::Item::from_json(Block &block, const json &j)
{
    if (j.count("component")) {
        UUID uu = j.at("component").get<std::string>();
        if (block.components.count(uu)) {
            component = &block.components.at(uu);
            connpath = UUIDPath<2>(j.at("path").get<std::string>());
        }
    }
    if (j.count("net")) {
        UUID uu = j.at("net").get<std::string>();
        if (block.nets.count(uu))
            net = &block.nets.at(uu);
    }
}

bool ToolBackannotateConnectionLines::ToolDataBackannotate::Item::is_valid() const
{
    return component || net;
}

ToolBackannotateConnectionLines::ToolBackannotateConnectionLines(IDocument *c, ToolID tid) : ToolBase(c, tid)
{
}

bool ToolBackannotateConnectionLines::can_begin()
{
    return doc.c;
}

Net *ToolBackannotateConnectionLines::create_net_stub(Component *comp, const UUIDPath<2> &connpath, Net *net)
{
    auto sch = doc.c->get_schematic();
    if (comp->connections.count(connpath))
        return nullptr;
    SchematicSymbol *sym = nullptr;
    Sheet *sheet = nullptr;
    const UUID &uu_gate = connpath.at(0);
    const UUID &uu_pin = connpath.at(1);
    for (auto &it : sch->sheets) {
        for (auto &it_sym : it.second.symbols) {
            if (it_sym.second.component == comp && it_sym.second.gate->uuid == uu_gate) {
                sym = &it_sym.second;
                sheet = &it.second;
                break;
            }
        }
    }
    if (!(sym && sheet))
        return nullptr;
    if (!net) {
        net = sch->block->insert_net();
        net->name = "N" + std::to_string(net_n);
        net_n++;
    }

    auto uu_line = UUID::random();
    auto &line = sheet->net_lines.emplace(uu_line, uu_line).first->second;
    line.net = net;
    line.from.connect(sym, &sym->symbol.pins.at(uu_pin));
    auto uu_label = UUID::random();
    auto uu_ju = UUID::random();
    auto &ju = sheet->junctions.emplace(uu_ju, uu_ju).first->second;
    auto &la = sheet->net_labels.emplace(uu_label, uu_label).first->second;
    la.junction = &ju;
    auto orientation = sym->symbol.pins.at(uu_pin).get_orientation_for_placement(sym->placement);
    la.orientation = orientation;
    line.to.connect(&ju);
    comp->connections.emplace(UUIDPath<2>(uu_gate, uu_pin), net);
    ju.net = net;
    ju.position = sym->placement.transform(sym->symbol.pins.at(uu_pin).position);

    switch (orientation) {
    case Orientation::LEFT:
        ju.position.x -= 1.25_mm;
        break;
    case Orientation::RIGHT:
        ju.position.x += 1.25_mm;
        break;
    case Orientation::DOWN:
        ju.position.y -= 1.25_mm;
        break;
    case Orientation::UP:
        ju.position.y += 1.25_mm;
        break;
    }

    return net;
}

ToolResponse ToolBackannotateConnectionLines::begin(const ToolArgs &args)
{
    if (auto data = dynamic_cast<ToolDataBackannotate *>(args.data.get())) {
        for (const auto &it : data->connections) {
            if (it.first.component && it.second.component) { // pin-pin, maybe create net
                auto net = create_net_stub(it.first.component, it.first.connpath);
                if (net)
                    create_net_stub(it.second.component, it.second.connpath, net);
            }
            else if (it.first.net && it.second.component) { // net-pin, reuse net
                create_net_stub(it.second.component, it.second.connpath, it.first.net);
            }
            else if (it.first.component && it.second.net) { // pin-net, reuse net
                create_net_stub(it.first.component, it.first.connpath, it.second.net);
            }
        }
        return ToolResponse::commit();
    }
    else {
        return ToolResponse::end();
    }
}
ToolResponse ToolBackannotateConnectionLines::update(const ToolArgs &args)
{
    return ToolResponse();
}
} // namespace horizon
