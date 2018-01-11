#pragma once
#include "util/uuid.hpp"

namespace horizon {
	
	/**
	 * Interface for classes that store objects identified by UUID (e.g.\ Line or Junction)
	 */
	class ObjectProvider {
		public :
			virtual class Junction *get_junction(const UUID &uu) {
				return nullptr;
			}
	};
	
	
}
