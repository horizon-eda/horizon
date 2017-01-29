#pragma once
#include "common.hpp"
#include "uuid_provider.hpp"

namespace horizon {
	class PositionProvider: public UUIDProvider {
		public:
		virtual Coordi get_position() const = 0;
		virtual ~PositionProvider() {}
	};

}
