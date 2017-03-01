#include "tool_paste.hpp"
#include <iostream>
#include "core_package.hpp"
#include "core_schematic.hpp"
#include "core_padstack.hpp"

namespace horizon {

	ToolPaste::ToolPaste(Core *c, ToolID tid): ToolBase(c, tid) {
		name = "Paste";
	}

	class JunctionProvider: public Object {
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
		if(core.r->get_layers().count(la) == 0) {
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
				apply_shift(x->position, args.coords);
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
		std::map<UUID, const UUID> component_xlat;
		if(j.count("components") && core.c){
			const json &o = j["components"];
			auto block = core.c->get_schematic()->block;
			for (auto it = o.cbegin(); it != o.cend(); ++it) {
				auto u = UUID::random();
				component_xlat.emplace(it.key(), u);
				block->components.emplace(u, Component(u, it.value(), *core.r->m_pool, *block));
			}
		}
		if(j.count("symbols") && core.c){
			const json &o = j["symbols"];
			auto sheet = core.c->get_sheet();
			auto block = core.c->get_schematic()->block;
			for (auto it = o.cbegin(); it != o.cend(); ++it) {
				std::cout << "paste sym" << std::endl;
				auto u = UUID::random();
				json k = it.value();
				k["component"] = (std::string)component_xlat.at(k.at("component").get<std::string>());
				auto x = &sheet->symbols.emplace(u, SchematicSymbol(u, k, *block ,*core.r->m_pool)).first->second;
				for(auto &it_txt: x->texts) {
					it_txt = core.r->get_text(text_xlat.at(it_txt.uuid));
				}
				x->placement.shift += shift;
				core.r->selection.emplace(u, ObjectType::SCHEMATIC_SYMBOL);
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
		core.r->commit();
		return ToolResponse::next(ToolID::MOVE);
	}

	ToolResponse ToolPaste::update(const ToolArgs &args) {
		return ToolResponse();
	}

}
