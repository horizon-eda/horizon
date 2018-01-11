#pragma once
#include "util/uuid.hpp"

namespace horizon {
	/**
	 * Interface for objects that have a UUID.
	 * Used by uuid_ptr.
	 */
	class UUIDProvider {
		public:
		virtual UUID get_uuid() const = 0;
		virtual ~UUIDProvider() {}
	};
}
