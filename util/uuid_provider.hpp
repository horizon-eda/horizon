#pragma once
#include "uuid.hpp"

namespace horizon {
	class UUIDProvider {
		public:
		virtual UUID get_uuid() const = 0;
		virtual ~UUIDProvider() {}
	};
}
