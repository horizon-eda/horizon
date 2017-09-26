#include "tool_paste.hpp"
#include <iostream>
#include "core_package.hpp"
#include "core_schematic.hpp"
#include "core_padstack.hpp"
#include "imp_interface.hpp"
#include "entity.hpp"

namespace horizon {

	ToolPaste::ToolPaste(Core *c, ToolID tid): ToolBase(c, tid) {
		name = "Paste";
	}

	class JunctionProvider: public ObjectProvider {
		public:
			JunctionProvider(Core *c, const std::map<UUID, const UUID> &xj):core(c), junction_xlat(xj) {}
			virtual ~JunctionProvider() {}

			Junction *get_junction(const UUID &uu) override {
				return core->get_junction(junction_xlat.at(uu));
			}


		private:
			Core *core;
			const std::map<UUID, const UUID> &junction_xlat;
	};

	void ToolPaste::fix_layer(int &la) {
		if(core.r->get_layer_provider()->get_layers().count(la) == 0) {
			la = 0;
		}
	}

	void ToolPaste::apply_shift(Coordi &c, const Coordi &cursor_pos) {
		c += shift;
	}

	ToolResponse ToolPaste::begin(const ToolArgs &args) {
		auto ref_clipboard = Gtk::Clipboard::get();
		auto seld = ref_clipboard->wait_for_contents("imp-buffer");
		if(seld.gobj())
			std::cout <<  "len " << seld.get_length() <<std::endl;
		else
			return ToolResponse::end();
		auto clipboard_data = seld.get_data_as_string();
		auto j = json::parse(clipboard_data);
		std::cout << j <<std::endl;
		Coordi cursor_pos = j.at("cursor_pos").get<std::vector<int64_t>>();
		core.r->selection.clear();
		shift = args.coords - cursor_pos;

		std::map<UUID, const UUID> text_xlat;
		if(j.count("texts")){
			const json &o = j["texts"];
			for (auto it = o.cbegin(); it != o.cend(); ++it) {
				auto u = UUID::random();
				text_xlat.emplace(it.key(), u);
				auto x = core.r->insert_text(u);
				*x = Text(u, it.value());
				fix_layer(x->layer);
				apply_shift(x->placement.shift, args.coords);
				core.r->selection.emplace(u, ObjectType::TEXT);
			}
		}
		std::map<UUID, const UUID> junction_xlat;
		if(j.count("junctions")){
			const json &o = j["junctions"];
			for (auto it = o.cbegin(); it != o.cend(); ++it) {
				auto u = UUID::random();
				junction_xlat.emplace(it.key(), u);
				auto x = core.r->insert_junction(u);
				*x = Junction(u, it.value());
				apply_shift(x->position, args.coords);
				core.r->selection.emplace(u, ObjectType::JUNCTION);
			}
		}
		if(j.count("lines")){
			const json &o = j["lines"];
			for (auto it = o.cbegin(); it != o.cend(); ++it) {
				auto u = UUID::random();
				auto x = core.r->insert_line(u);
				JunctionProvider p(core.r, junction_xlat);
				*x = Line(u, it.value(), p);
				fix_layer(x->layer);
				core.r->selection.emplace(u, ObjectType::LINE);
			}
		}
		if(j.count("arcs")){
			const json &o = j["arcs"];
			for (auto it = o.cbegin(); it != o.cend(); ++it) {
				auto u = UUID::random();
				auto x = core.r->insert_arc(u);
				JunctionProvider p(core.r, junction_xlat);
				*x = Arc(u, it.value(), p);
				fix_layer(x->layer);
				core.r->selection.emplace(u, ObjectType::ARC);
			}
		}
		if(j.count("pads") && core.k){
			const json &o = j["pads"];
			for (auto it = o.cbegin(); it != o.cend(); ++it) {
				auto u = UUID::random();
				auto &x = core.k->get_package()->pads.emplace(u, Pad(u, it.value(), *core.r->m_pool)).first->second;
				apply_shift(x.placement.shift, args.coords);
				core.r->selection.emplace(u, ObjectType::PAD);
			}
		}
		if(j.count("holes")){
			const json &o = j["holes"];
			for (auto it = o.cbegin(); it != o.cend(); ++it) {
				auto u = UUID::random();
				auto x = core.r->insert_hole(u);
				*x = Hole(u, it.value());
				apply_shift(x->placement.shift, args.coords);
				core.r->selection.emplace(u, ObjectType::HOLE);
			}
		}
		if(j.count("polygons")){
			const json &o = j["polygons"];
			for (auto it = o.cbegin(); it != o.cend(); ++it) {
				auto u = UUID::random();
				auto x = core.r->insert_polygon(u);
				*x = Polygon(u, it.value());
				for(auto &itv: x->vertices) {
					itv.arc_center += shift;
					itv.position += shift;
				}
				core.r->selection.emplace(u, ObjectType::POLYGON);
			}
		}
		std::map<UUID, const UUID> net_xlat;
		if(j.count("nets") && core.c){
			const json &o = j["nets"];
			auto block = core.c->get_schematic()->block;
			for (auto it = o.cbegin(); it != o.cend(); ++it) {
				Net net_from_json(it.key(), it.value());
				std::string net_name = net_from_json.name;
				std::cout << "paste net " << net_name << std::endl;
				Net *net_new = nullptr;
				if(net_name.size()) {
					for(auto &it_net: block->nets) {
						if(it_net.second.name == net_name) {
							net_new = &it_net.second;
							break;
						}
					}
				}
				if(!net_new) {
					net_new = block->insert_net();
					net_new->name = net_name;
				}
				net_xlat.emplace(it.key(), net_new->uuid);
			}
		}

		std::map<UUID, const UUID> component_xlat;
		if(j.count("components") && core.c){
			const json &o = j["components"];
			auto block = core.c->get_schematic()->block;
			for (auto it = o.cbegin(); it != o.cend(); ++it) {
				auto u = UUID::random();
				component_xlat.emplace(it.key(), u);
				Component comp(u, it.value(), *core.r->m_pool);
				for(auto &it_conn: comp.connections) {
					it_conn.second.net = &block->nets.at(net_xlat.at(it_conn.second.net.uuid));
				}
				block->components.emplace(u, comp);
			}
		}
		std::map<UUID, const UUID> symbol_xlat;
		if(j.count("symbols") && core.c){
			const json &o = j["symbols"];
			auto sheet = core.c->get_sheet();
			auto block = core.c->get_schematic()->block;
			for (auto it = o.cbegin(); it != o.cend(); ++it) {
				std::cout << "paste sym" << std::endl;
				auto u = UUID::random();
				symbol_xlat.emplace(it.key(), u);
				SchematicSymbol sym(u, it.value(), *core.r->m_pool);
				sym.component = &block->components.at(component_xlat.at(sym.component.uuid));
				sym.gate = &sym.component->entity->gates.at(sym.gate.uuid);
				auto x = &sheet->symbols.emplace(u, sym).first->second;
				for(auto &it_txt: x->texts) {
					it_txt = core.r->get_text(text_xlat.at(it_txt.uuid));
				}
				x->placement.shift += shift;
				core.r->selection.emplace(u, ObjectType::SCHEMATIC_SYMBOL);
			}
		}
		if(j.count("net_lines") && core.c){
			const json &o = j["net_lines"];
			auto sheet = core.c->get_sheet();
			for (auto it = o.cbegin(); it != o.cend(); ++it) {
				std::cout << "paste net line" << std::endl;
				auto u = UUID::random();
				LineNet line(u, it.value());
				auto update_net_line_conn = [this, &junction_xlat, &symbol_xlat, sheet](LineNet::Connection &c){
					if(c.junc.uuid) {
						c.junc = &sheet->junctions.at(junction_xlat.at(c.junc.uuid));
					}
					if(c.symbol.uuid) {
						c.symbol = &sheet->symbols.at(symbol_xlat.at(c.symbol.uuid));
						c.pin = &c.symbol->symbol.pins.at(c.pin.uuid);
					}
				};
				update_net_line_conn(line.from);
				update_net_line_conn(line.to);
				sheet->net_lines.emplace(u, line);
				core.r->selection.emplace(u, ObjectType::LINE_NET);
			}
		}
		if(j.count("shapes")){
			const json &o = j["shapes"];
			for (auto it = o.cbegin(); it != o.cend(); ++it) {
				auto u = UUID::random();
				auto x = &core.a->get_padstack()->shapes.emplace(u, Shape(u, it.value())).first->second;
				x->placement.shift += shift;
				core.r->selection.emplace(u, ObjectType::SHAPE);
			}
		}
		move_init(args.coords);
		imp->tool_bar_set_tip("<b>LMB:</b>place <b>RMB:</b>cancel <b>r:</b>rotate <b>e:</b>mirror");
		if(core.c) {
			core.c->get_schematic()->expand(true);
		}
		return ToolResponse();
	}

	ToolResponse ToolPaste::update(const ToolArgs &args) {
		if(args.type == ToolEventType::MOVE) {
			move_do_cursor(args.coords);
		}
		else if(args.type == ToolEventType::CLICK) {
			if(args.button == 1) {
				merge_selected_junctions();
				core.r->commit();
				return ToolResponse::end();
			}
			else {
				core.r->revert();
				return ToolResponse::end();
			}
		}
		else if(args.type == ToolEventType::KEY) {
			if(args.key == GDK_KEY_Escape) {
				core.r->revert();
				return ToolResponse::end();
			}
			else if(args.key == GDK_KEY_r || args.key == GDK_KEY_e) {
				bool rotate = args.key == GDK_KEY_r;
				move_mirror_or_rotate(args.coords, rotate);
			}
		}
		return ToolResponse();
	}

}
