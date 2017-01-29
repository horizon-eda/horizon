#include "clearance.hpp"
#include "json.hpp"
#include "constraints.hpp"
#include "common.hpp"

namespace horizon {
	Clearance::Clearance(const UUIDPath<2> &uu, const json &j, Constraints &co):
		netclass_a(&co.net_classes.at(uu.at(0))),
		netclass_b(&co.net_classes.at(uu.at(1))),
		routing_clearance(j.at("routing_clearance"))
	{

	}

	Clearance::Clearance(const json &j): routing_clearance(j.at("routing_clearance")) {}
	Clearance::Clearance(): routing_clearance(1_mm){}

	json Clearance::serialize() const {
		json j;
		j["routing_clearance"] = routing_clearance;
		return j;
	}
}
