#include "fab_output_settings.hpp"
#include "board.hpp"
#include "util/util.hpp"
#include "board_layers.hpp"

namespace horizon {

	const LutEnumStr<FabOutputSettings::DrillMode> FabOutputSettings::mode_lut = {
		{"merged",     FabOutputSettings::DrillMode::MERGED},
		{"individual", FabOutputSettings::DrillMode::INDIVIDUAL},
	};

	FabOutputSettings::GerberLayer::GerberLayer(int l): layer(l) {
		switch(layer) {
			case BoardLayers::L_OUTLINE :
				filename = ".gko";
			break;
			case BoardLayers::TOP_COPPER :
				filename = ".gtl";
			break;
			case BoardLayers::TOP_MASK :
				filename = ".gts";
			break;
			case BoardLayers::TOP_SILKSCREEN :
				filename = ".gto";
			break;
			case BoardLayers::TOP_PASTE :
				filename = ".gtp";
			break;
			case BoardLayers::BOTTOM_COPPER :
				filename = ".gbl";
			break;
			case BoardLayers::BOTTOM_MASK :
				filename = ".gbs";
			break;
			case BoardLayers::BOTTOM_SILKSCREEN :
				filename = ".gbo";
			break;
			case BoardLayers::BOTTOM_PASTE :
				filename = ".gbp";
			break;
		}
	}

	FabOutputSettings::GerberLayer::GerberLayer(int l, const json &j): layer(l), filename(j.at("filename").get<std::string>()), enabled(j.at("enabled")) {}

	json FabOutputSettings::GerberLayer::serialize() const {
		json j;
		j["layer"] = layer;
		j["filename"] = filename;
		j["enabled"] = enabled;
		return j;
	}

	FabOutputSettings::FabOutputSettings(const json &j): drill_pth_filename(j.at("drill_pth").get<std::string>()), drill_npth_filename(j.at("drill_npth").get<std::string>()),
	prefix(j.at("prefix").get<std::string>()), output_directory(j.at("output_directory").get<std::string>()) {
		 {
			const json &o = j["layers"];
			for (auto it = o.cbegin(); it != o.cend(); ++it) {
				int k = std::stoi(it.key());
				layers.emplace(std::piecewise_construct, std::forward_as_tuple(k), std::forward_as_tuple(k, it.value()));
			}
		}
		if(j.count("drill_mode")) {
			drill_mode = mode_lut.lookup(j.at("drill_mode"));
		}
	}

	void FabOutputSettings::update_for_board(const Board *brd) {
		auto layers_from_board = brd->get_layers();
		//remove layers not on board
		map_erase_if(layers, [layers_from_board](const auto &it){return layers_from_board.count(it.first)==0;});

		//add new layers
		auto add_layer = [this](int l) {
			layers.emplace(l, l);
		};

		add_layer(BoardLayers::L_OUTLINE);
		add_layer(BoardLayers::TOP_PASTE);
		add_layer(BoardLayers::TOP_SILKSCREEN);
		add_layer(BoardLayers::TOP_MASK);
		add_layer(BoardLayers::TOP_COPPER);
		for(const auto &la: layers_from_board) {
			if(BoardLayers::is_copper(la.first) && la.first > BoardLayers::BOTTOM_COPPER && la.first < BoardLayers::TOP_COPPER)
				add_layer(la.first);
		}
		add_layer(BoardLayers::BOTTOM_COPPER);
		add_layer(BoardLayers::BOTTOM_MASK);
		add_layer(BoardLayers::BOTTOM_SILKSCREEN);
		add_layer(BoardLayers::BOTTOM_PASTE);
	}

	json FabOutputSettings::serialize() const {
		json j;
		j["drill_pth"] = drill_pth_filename;
		j["drill_npth"] = drill_npth_filename;
		j["prefix"] = prefix;
		j["output_directory"] = output_directory;
		j["drill_mode"] = mode_lut.lookup_reverse(drill_mode);
		j["layers"] = json::object();
		for(const auto &it: layers) {
			j["layers"][std::to_string(it.first)] = it.second.serialize();
		}

		return j;
	}
}
