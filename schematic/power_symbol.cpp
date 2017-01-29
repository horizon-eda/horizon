#include "power_symbol.hpp"
#include "lut.hpp"
#include "json.hpp"
#include "sheet.hpp"

namespace horizon {


	PowerSymbol::PowerSymbol(const UUID &uu, const json &j, Sheet &sheet, Block &block):
		uuid(uu),
		junction(&sheet.junctions.at(j.at("junction").get<std::string>())),
		net(&block.nets.at(j.at("net").get<std::string>())),
		mirror(j.value("mirror", false)),
		temp(false)
	{
	}

	UUID PowerSymbol::get_uuid() const {
		return uuid;
	}

	void PowerSymbol::update_refs(Sheet &sheet, Block &block) {
		junction.update(sheet.junctions);
		net.update(block.nets);
	}

	PowerSymbol::PowerSymbol(const UUID &uu): uuid(uu) {}

	json PowerSymbol::serialize() const {
		json j;
		j["junction"] = (std::string)junction->uuid;
		j["net"] = (std::string)net->uuid;
		j["mirror"] = mirror;
		return j;
	}
}
