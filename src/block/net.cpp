#include "block/net.hpp"
#include "block/block.hpp"
#include "common/lut.hpp"

namespace horizon {

	static const LutEnumStr<Net::PowerSymbolStyle> power_symbol_style_lut = {
		{"gnd",     Net::PowerSymbolStyle::GND},
		{"dot",     Net::PowerSymbolStyle::DOT},
		{"antenna",	Net::PowerSymbolStyle::ANTENNA},
	};

	Net::Net(const UUID &uu, const json &j, Block &block): Net(uu ,j)
		{
		net_class = &block.net_classes.at(j.at("net_class").get<std::string>());
		if(j.count("power_symbol_style"))
			power_symbol_style = power_symbol_style_lut.lookup(j.at("power_symbol_style"));
	}
	Net::Net(const UUID &uu, const json &j):
			uuid(uu),
			name(j.at("name").get<std::string>()),
			is_power(j.value("is_power", false))
	{
		net_class.uuid = j.at("net_class").get<std::string>();
		if(j.count("diffpair")) {
			UUID diffpair_uuid(j.at("diffpair").get<std::string>());
			if(diffpair_uuid) {
				diffpair.uuid = diffpair_uuid;
				diffpair_master = true;
			}
		}
	}

	Net::Net (const UUID &uu): uuid(uu){};

	UUID Net::get_uuid() const {
		return uuid;
	}

	json Net::serialize() const {
		json j;
		j["name"] = name;
		j["is_power"] = is_power;
		j["net_class"] = net_class->uuid;
		j["power_symbol_style"] = power_symbol_style_lut.lookup_reverse(power_symbol_style);
		if(diffpair_master)
			j["diffpair"] = diffpair->uuid;
		return j;
	}

	bool Net::is_named() const {
		return name.size()>0;
	}
}
