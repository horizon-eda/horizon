#pragma once
#include "util/uuid.hpp"
#include "logger.hpp"
#include "common/object_descr.hpp"
#include <map>
#include <tuple>
#include <string>

namespace horizon {
	template<typename T, typename... Args1> void load_and_log(std::map<UUID,T> &map, ObjectType type, std::tuple<Args1...> args, Logger::Domain dom=Logger::Domain::UNSPECIFIED) {
		auto uu = std::get<0>(args);
		try {
			map.emplace(std::piecewise_construct, std::forward_as_tuple(uu), std::forward<std::tuple<Args1...> >(args));
		}
		catch (const std::exception &e) {
			Logger::log_warning("couldn't load " + object_descriptions.at(type).name + " " + (std::string)uu, dom, e.what());
		}
		catch (...) {
			Logger::log_warning("couldn't load " + object_descriptions.at(type).name + " " + (std::string)uu, dom);
		}
	}
}
