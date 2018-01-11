#include "board_rules.hpp"
#include "util/util.hpp"
#include "board/board.hpp"
#include "rules/cache.hpp"
#include "util/accumulator.hpp"
#include "common/patch_type_names.hpp"
#include "board/board_layers.hpp"

namespace horizon {
	RulesCheckResult BoardRules::check_track_width(const Board *brd) {
		RulesCheckResult r;
		r.level = RulesCheckErrorLevel::PASS;
		auto rules = dynamic_cast_vector<RuleTrackWidth*>(get_rules_sorted(RuleID::TRACK_WIDTH));
		for(const auto &it: brd->tracks) {
			auto width = it.second.width;
			Net *net = it.second.net;
			auto layer = it.second.layer;
			auto &track = it.second;
			for(auto ru: rules) {
				if(ru->enabled && ru->match.match(net)) {
					if(ru->widths.count(layer)) {
						const auto &ws = ru->widths.at(layer);
						if(width < ws.min || width > ws.max) {
							r.errors.emplace_back(RulesCheckErrorLevel::FAIL);
							auto &e = r.errors.back();
							e.has_location = true;
							e.location = (track.from.get_position()+track.to.get_position())/2;
							e.comment = "Track width "+dim_to_string(width);
							if(width < ws.min) {
								e.comment += " is less than " + dim_to_string(ws.min);
							}
							else {
								e.comment += " is greater than " + dim_to_string(ws.max);
							}
						}
					}
					break;
				}
			}
		}
		r.update();
		return r;
	}

	static RulesCheckError *check_hole(RulesCheckResult &r, uint64_t dia, const RuleHoleSize *ru, const std::string &what) {
		if(dia < ru->diameter_min || dia > ru->diameter_max) {
			r.errors.emplace_back(RulesCheckErrorLevel::FAIL);
			auto &e = r.errors.back();
			e.has_location = true;
			e.comment = what + " diameter "+dim_to_string(dia);
			if(dia < ru->diameter_min) {
				e.comment += " is less than " + dim_to_string(ru->diameter_min);
			}
			else {
				e.comment += " is greater than " + dim_to_string(ru->diameter_max);
			}
			return &e;
		}
		return nullptr;
	}

	RulesCheckResult BoardRules::check_hole_size(const Board *brd) {
		RulesCheckResult r;
		r.level = RulesCheckErrorLevel::PASS;
		auto rules = dynamic_cast_vector<RuleHoleSize*>(get_rules_sorted(RuleID::HOLE_SIZE));
		for(const auto &it: brd->holes) {
			Net *net = it.second.net;
			for(const auto &it_hole: it.second.padstack.holes) {
				auto dia = it_hole.second.diameter;
				for(auto ru: rules) {
					if(ru->enabled && ru->match.match(net)) {
						if(auto e = check_hole(r, dia, ru, "Hole")) {
							e->location = it.second.placement.shift;
						}
						break;
					}
				}
			}
		}

		for(const auto &it: brd->vias) {
			Net *net = it.second.junction->net;
			for(const auto &it_hole: it.second.padstack.holes) {
				auto dia = it_hole.second.diameter;
				for(auto ru: rules) {
					if(ru->enabled && ru->match.match(net)) {
						if(auto e = check_hole(r, dia, ru, "Via")) {
							e->location = it.second.junction->position;
						}
						break;
					}
				}
			}
		}

		for(const auto &it: brd->packages) {
			for(const auto &it_pad: it.second.package.pads) {
				Net *net = it_pad.second.net;
				for(const auto &it_hole: it_pad.second.padstack.holes) {
					auto dia = it_hole.second.diameter;
					for(auto ru: rules) {
						if(ru->enabled && ru->match.match(net)) {
							if(auto e = check_hole(r, dia, ru, "Pad hole")) {
								auto p = it.second.placement;
								if(it.second.flip) {
									p.invert_angle();
								}
								e->location = p.transform(it_pad.second.placement.transform(it_hole.second.placement.shift));
							}
							break;
						}
					}
				}
			}
		}
		r.update();
		return r;
	}

	RulesCheckResult BoardRules::check_clearance_copper(const Board *brd, RulesCheckCache &cache) {
		RulesCheckResult r;
		r.level = RulesCheckErrorLevel::PASS;
		auto c = dynamic_cast<RulesCheckCacheBoardImage*>(cache.get_cache(RulesCheckCacheID::BOARD_IMAGE));
		std::set<int> layers;
		const auto &patches = c->get_canvas()->patches;
		for(const auto &it: patches) { //collect copper layers
			if(brd->get_layers().count(it.first.layer) && brd->get_layers().at(it.first.layer).copper) {
				layers.emplace(it.first.layer);
			}
		}

		for(const auto layer: layers) { //check each layer individually
			//assemble a list of patch pairs we'll need to check
			std::set<std::pair<CanvasPatch::PatchKey, CanvasPatch::PatchKey>> patch_pairs;
			for(const auto &it: patches) {
				for(const auto &it_other: patches) {
					if(layer == it.first.layer && it.first.layer == it_other.first.layer && it.first.net != it_other.first.net && it.first.type != PatchType::OTHER  && it.first.type != PatchType::TEXT && it_other.first.type != PatchType::OTHER && it_other.first.type != PatchType::TEXT) {//see if it needs to be checked against it_other
						std::pair<CanvasPatch::PatchKey, CanvasPatch::PatchKey> k = {it.first, it_other.first};
						auto k2 = k;
						std::swap(k2.first, k2.second);
						if(patch_pairs.count(k) == 0 && patch_pairs.count(k2) == 0) {
							patch_pairs.emplace(k);
						}
					}
				}
			}

			for(const auto &it: patch_pairs) {
				auto p1 = it.first;
				auto p2 = it.second;
				std::cout << "patch pair:" << p1.layer << " " << static_cast<int>(p1.type) << " " << (std::string)p1.net;
				std::cout << " - " << p2.layer << " " << static_cast<int>(p2.type) << " " << (std::string)p2.net << "\n";
				Net *net1 = p1.net?&brd->block->nets.at(p1.net):nullptr;
				Net *net2 = p2.net?&brd->block->nets.at(p2.net):nullptr;

				//figure out the clearance between this patch pair
				uint64_t clearance = 0;
				auto rule_clearance = get_clearance_copper(net1, net2, p1.layer);
				if(rule_clearance) {
					clearance = rule_clearance->get_clearance(p1.type, p2.type);
				}
				std::cout << "clearance: " << clearance << "\n";

				//expand one of them by the clearance
				ClipperLib::ClipperOffset ofs;
				ofs.ArcTolerance = 10e3;
				ofs.AddPaths(patches.at(p1), ClipperLib::jtRound, ClipperLib::etClosedPolygon);
				ClipperLib::Paths paths_ofs;
				ofs.Execute(paths_ofs, clearance);

				//intersect expanded and other patches
				ClipperLib::Clipper clipper;
				clipper.AddPaths(paths_ofs, ClipperLib::ptClip, true);
				clipper.AddPaths(patches.at(p2), ClipperLib::ptSubject, true);
				ClipperLib::Paths errors;
				clipper.Execute(ClipperLib::ctIntersection, errors, ClipperLib::pftNonZero, ClipperLib::pftNonZero);

				//no intersection: no clearance violation
				if(errors.size() > 0) {
					for(const auto &ite: errors) {
						r.errors.emplace_back(RulesCheckErrorLevel::FAIL);
						auto &e = r.errors.back();
						e.has_location = true;
						Accumulator<Coordi> acc;
						for(const auto &ite2: ite) {
							acc.accumulate({ite2.X, ite2.Y});
						}
						e.location = acc.get();
						e.comment = patch_type_names.at(p1.type) + "(" + (net1?net1->name:"") + ") near " + patch_type_names.at(p2.type) + "(" + (net2?net2->name:"") + ")";
						e.error_polygons = {ite};
					}
				}


				std::cout << "\n" << std::endl;

			}


		}
		r.update();
		return r;
	}

	RulesCheckResult BoardRules::check_clearance_copper_non_copper(const Board *brd, RulesCheckCache &cache) {
		RulesCheckResult r;
		r.level = RulesCheckErrorLevel::PASS;
		auto c = dynamic_cast<RulesCheckCacheBoardImage*>(cache.get_cache(RulesCheckCacheID::BOARD_IMAGE));
		const auto &patches = c->get_canvas()->patches;
		CanvasPatch::PatchKey npth_key;
		npth_key.layer = 10000;
		npth_key.net = UUID();
		npth_key.type = PatchType::HOLE_NPTH;
		if(patches.count(npth_key) == 0) {
			return r;
		}
		auto &patch_npth = patches.at(npth_key);

		for(const auto &it: patches) {
			if(brd->get_layers().count(it.first.layer) && brd->get_layers().at(it.first.layer).copper) { //need to check copper layer
				Net *net = it.first.net?&brd->block->nets.at(it.first.net):nullptr;

				auto clearance = get_clearance_copper_other(net, it.first.layer)->get_clearance(it.first.type, PatchType::HOLE_NPTH);

				//expand npth patch by clearance
				ClipperLib::ClipperOffset ofs;
				ofs.ArcTolerance = 10e3;
				ofs.AddPaths(patch_npth, ClipperLib::jtRound, ClipperLib::etClosedPolygon);
				ClipperLib::Paths paths_ofs;
				ofs.Execute(paths_ofs, clearance);

				//intersect expanded and this patch
				ClipperLib::Clipper clipper;
				clipper.AddPaths(paths_ofs, ClipperLib::ptClip, true);
				clipper.AddPaths(it.second, ClipperLib::ptSubject, true);
				ClipperLib::Paths errors;
				clipper.Execute(ClipperLib::ctIntersection, errors, ClipperLib::pftNonZero, ClipperLib::pftNonZero);

				//no intersection: no clearance violation
				if(errors.size() > 0) {
					for(const auto &ite: errors) {
						r.errors.emplace_back(RulesCheckErrorLevel::FAIL);
						auto &e = r.errors.back();
						e.has_location = true;
						Accumulator<Coordi> acc;
						for(const auto &ite2: ite) {
							acc.accumulate({ite2.X, ite2.Y});
						}
						e.location = acc.get();
						e.comment = patch_type_names.at(it.first.type) + "(" + (net?net->name:"") + ") near NPTH hole";
						e.error_polygons = {ite};
					}
				}
			}
		}
		r.update();


		//other cu
		std::set<int> layers;
		for(const auto &it: patches) { //collect copper layers
			if(brd->get_layers().count(it.first.layer) && brd->get_layers().at(it.first.layer).copper) {
				layers.emplace(it.first.layer);
			}
		}

		auto is_other = [](PatchType pt) {return pt==PatchType::OTHER || pt==PatchType::TEXT;};

		for(const auto layer: layers) { //check each layer individually
			//assemble a list of patch pairs we'll need to check
			std::set<std::pair<CanvasPatch::PatchKey, CanvasPatch::PatchKey>> patch_pairs;
			for(const auto &it: patches) {
				for(const auto &it_other: patches) {
					if(layer == it.first.layer && it.first.layer == it_other.first.layer && ((is_other(it.first.type) ^ is_other(it_other.first.type)))) {//check non-other against other
						std::pair<CanvasPatch::PatchKey, CanvasPatch::PatchKey> k = {it.first, it_other.first};
						auto k2 = k;
						std::swap(k2.first, k2.second);
						if(patch_pairs.count(k) == 0 && patch_pairs.count(k2) == 0) {
							patch_pairs.emplace(k);
						}
					}
				}
			}


			for(const auto &it: patch_pairs) {
				auto p_other = it.first;
				auto p_non_other = it.second;
				if(!is_other(p_other.type))
					std::swap(p_other, p_non_other);
				Net *net = p_non_other.net?&brd->block->nets.at(p_non_other.net):nullptr;

				//figure out the clearance between this patch pair
				uint64_t clearance = 0;
				auto rule_clearance = get_clearance_copper_other(net, p_non_other.layer);
				if(rule_clearance) {
					clearance = rule_clearance->get_clearance(p_non_other.type, p_other.type);
				}

				//expand one of them by the clearance
				ClipperLib::ClipperOffset ofs;
				ofs.ArcTolerance = 10e3;
				ofs.AddPaths(patches.at(p_non_other), ClipperLib::jtRound, ClipperLib::etClosedPolygon);
				ClipperLib::Paths paths_ofs;
				ofs.Execute(paths_ofs, clearance);

				//intersect expanded and other patches
				ClipperLib::Clipper clipper;
				clipper.AddPaths(paths_ofs, ClipperLib::ptClip, true);
				clipper.AddPaths(patches.at(p_other), ClipperLib::ptSubject, true);
				ClipperLib::Paths errors;
				clipper.Execute(ClipperLib::ctIntersection, errors, ClipperLib::pftNonZero, ClipperLib::pftNonZero);

				//no intersection: no clearance violation
				if(errors.size() > 0) {
					for(const auto &ite: errors) {
						r.errors.emplace_back(RulesCheckErrorLevel::FAIL);
						auto &e = r.errors.back();
						e.has_location = true;
						Accumulator<Coordi> acc;
						for(const auto &ite2: ite) {
							acc.accumulate({ite2.X, ite2.Y});
						}
						e.location = acc.get();
						e.comment = patch_type_names.at(p_non_other.type) + "(" + (net?net->name:"") + ") near " + patch_type_names.at(p_other.type);
						e.error_polygons = {ite};
					}
				}

			}
		}
		r.update();

		CanvasPatch::PatchKey outline_key;
		outline_key.layer = BoardLayers::L_OUTLINE;
		outline_key.net = UUID();
		outline_key.type = PatchType::OTHER;
		if(patches.count(outline_key) == 0) {
			return r;
		}
		auto &patch_outline = patches.at(outline_key);

		//cleanup board outline so that it conforms to nonzero filling rule
		ClipperLib::Paths board_outline;
		{
			ClipperLib::Clipper cl_outline;
			cl_outline.AddPaths(patch_outline, ClipperLib::ptSubject, true);
			cl_outline.Execute(ClipperLib::ctUnion, board_outline, ClipperLib::pftEvenOdd);
		}

		for(const auto &it: patches) {
			if(brd->get_layers().count(it.first.layer) && brd->get_layers().at(it.first.layer).copper) { //need to check copper layer
				Net *net = it.first.net?&brd->block->nets.at(it.first.net):nullptr;

				auto clearance = get_clearance_copper_other(net, it.first.layer)->get_clearance(it.first.type, PatchType::BOARD_EDGE);

				//contract board outline patch by clearance
				ClipperLib::ClipperOffset ofs;
				ofs.ArcTolerance = 10e3;
				ofs.AddPaths(board_outline, ClipperLib::jtRound, ClipperLib::etClosedPolygon);
				ClipperLib::Paths paths_ofs;
				ofs.Execute(paths_ofs, clearance*-1.0);

				//subtract board outline from patch
				ClipperLib::Clipper clipper;
				clipper.AddPaths(paths_ofs, ClipperLib::ptClip, true);
				clipper.AddPaths(it.second, ClipperLib::ptSubject, true);
				ClipperLib::Paths errors;
				clipper.Execute(ClipperLib::ctDifference, errors, ClipperLib::pftNonZero, ClipperLib::pftEvenOdd);

				//no remaining: no clearance violation
				if(errors.size() > 0) {
					for(const auto &ite: errors) {
						r.errors.emplace_back(RulesCheckErrorLevel::FAIL);
						auto &e = r.errors.back();
						e.has_location = true;
						Accumulator<Coordi> acc;
						for(const auto &ite2: ite) {
							acc.accumulate({ite2.X, ite2.Y});
						}
						e.location = acc.get();
						e.comment = patch_type_names.at(it.first.type) + "(" + (net?net->name:"") + ") near Board edge";
						e.error_polygons = {ite};
					}
				}
			}
		}


		r.update();
		return r;
	}

	RulesCheckResult BoardRules::check(RuleID id, const Board *brd, RulesCheckCache &cache) {
		switch(id) {
			case RuleID::TRACK_WIDTH :
				return BoardRules::check_track_width(brd);

			case RuleID::HOLE_SIZE :
				return BoardRules::check_hole_size(brd);

			case RuleID::CLEARANCE_COPPER :
				return BoardRules::check_clearance_copper(brd, cache);

			case RuleID::CLEARANCE_COPPER_OTHER :
				return BoardRules::check_clearance_copper_non_copper(brd, cache);

			default:
				return RulesCheckResult();
		}
	}
}
