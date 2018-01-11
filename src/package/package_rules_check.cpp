#include "pool/package.hpp"
#include "util/util.hpp"
#include "rules/cache.hpp"
#include "util/accumulator.hpp"
#include "board/board_layers.hpp"
#include <ctype.h>

namespace horizon {
	RulesCheckResult PackageRules::check_package(const class Package *pkg) {
		RulesCheckResult r;
		r.level = RulesCheckErrorLevel::PASS;
		auto rule = dynamic_cast<RulePackageChecks*>(get_rule(RuleID::PACKAGE_CHECKS));

		if(pkg->name.size() == 0) {
			r.errors.emplace_back(RulesCheckErrorLevel::FAIL);
			auto &x = r.errors.back();
			x.comment = "Package has no name";
			x.has_location = false;
		}

		int silk_refdes_count = 0;
		int assy_refdes_count = 0;
		const Text *silkscreen_refdes = nullptr;
		for(const auto &it: pkg->texts) {
			const auto &txt = it.second;
			if(txt.text == "$RD" && (txt.layer == BoardLayers::TOP_SILKSCREEN || txt.layer == BoardLayers::BOTTOM_SILKSCREEN)) {
				silk_refdes_count++;
				silkscreen_refdes = &txt;
			}
			if(txt.text == "$RD" && (txt.layer == BoardLayers::TOP_ASSEMBLY || txt.layer == BoardLayers::BOTTOM_ASSEMBLY)) {
				assy_refdes_count++;
			}
		}
		if(silk_refdes_count != 1) {
			r.errors.emplace_back(RulesCheckErrorLevel::FAIL);
			auto &x = r.errors.back();
			x.comment = "Package has " + std::to_string(silk_refdes_count) + " silkscreen reference designators, need 1";
			x.has_location = false;
		}
		if(assy_refdes_count != 1) {
			r.errors.emplace_back(RulesCheckErrorLevel::FAIL);
			auto &x = r.errors.back();
			x.comment = "Package has " + std::to_string(assy_refdes_count) + " assembly reference designators, need 1";
			x.has_location = false;
		}
		if(silkscreen_refdes) {
			if(silkscreen_refdes->width != 0.15_mm) {
				r.errors.emplace_back(RulesCheckErrorLevel::FAIL);
				auto &x = r.errors.back();
				x.comment = "Silkscreen refdes must have 0.15mm line width";
				x.has_location = false;
			}
			if(silkscreen_refdes->size != 1_mm) {
				r.errors.emplace_back(RulesCheckErrorLevel::FAIL);
				auto &x = r.errors.back();
				x.comment = "Silkscreen refdes must be 1mm in size";
				x.has_location = false;
			}
		}

		int courtyard_top_count = 0;
		int courtyard_bottom_count = 0;

		for(const auto &it: pkg->polygons) {
			if(it.second.layer == BoardLayers::TOP_COURTYARD) {
				courtyard_top_count++;
			}
			else if(it.second.layer == BoardLayers::BOTTOM_COURTYARD) {
				courtyard_bottom_count++;
			}
		}

		if(courtyard_top_count > 1 || courtyard_bottom_count > 1) {
			r.errors.emplace_back(RulesCheckErrorLevel::FAIL);
			auto &x = r.errors.back();
			x.comment = "Too many courtyard polygons";
			x.has_location = false;
		}
		if((courtyard_top_count + courtyard_bottom_count) < 1) {
			r.errors.emplace_back(RulesCheckErrorLevel::FAIL);
			auto &x = r.errors.back();
			x.comment = "Too few courtyard polygons";
			x.has_location = false;
		}

		std::set<std::string> pad_names;
		for(const auto &it: pkg->pads) {
			const auto &name = it.second.name;
			if(pad_names.insert(name).second == false) {
				r.errors.emplace_back(RulesCheckErrorLevel::FAIL);
				auto &x = r.errors.back();
				x.comment = "Duplicate pad name "+name;
				x.location = it.second.placement.shift;
				x.has_location = true;
			}
			if(name.size() == 0) {
				r.errors.emplace_back(RulesCheckErrorLevel::FAIL);
				auto &x = r.errors.back();
				x.comment = "Pad has no name";
				x.location = it.second.placement.shift;
				x.has_location = true;
			}

			auto is_valid_pad_name_char = [] (char c) {
				return isupper(c) || isdigit(c);
			};

			if(!std::all_of(name.begin(), name.end(), is_valid_pad_name_char)) {
				r.errors.emplace_back(RulesCheckErrorLevel::FAIL);
				auto &x = r.errors.back();
				x.comment = "Invalid pad name: must only contain digits and uppercase letters";
				x.location = it.second.placement.shift;
				x.has_location = true;
			}
		}

		for(const auto &it: pkg->polygons) {
			if(it.second.layer == BoardLayers::TOP_SILKSCREEN || it.second.layer == BoardLayers::BOTTOM_SILKSCREEN) {
				r.errors.emplace_back(RulesCheckErrorLevel::FAIL);
				auto &x = r.errors.back();
				x.comment = "Don't use polygons on silkscreen, use lines instead";
				x.has_location = false;
			}
		}


		r.update();
		return r;
	}

	RulesCheckResult PackageRules::check(RuleID id, const Package *pkg, RulesCheckCache &cache) {
		switch(id) {
			case RuleID::PACKAGE_CHECKS :
				return check_package(pkg);

			default:
				return RulesCheckResult();
		}
	}
}
