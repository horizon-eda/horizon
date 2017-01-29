#pragma once
#ifdef WIN32_UUID
	#include "uuid_win32.hpp"
#else
	#include <uuid.h>
#endif

#include <iostream>

namespace horizon {
	class UUID {
		public :
			UUID();
			static UUID random();
			UUID(const char *str);
			UUID(const std::string &str);
			operator std::string() const {
				char str[40];
				uuid_unparse(uu, str);
				return std::string(str);
			}
			
			operator bool() const {
				return !uuid_is_null(uu);
			}

			friend bool operator== (const UUID &self, const UUID &other);
			friend bool operator!= (const UUID &self, const UUID &other);
			friend bool operator<  (const UUID &self, const UUID &other);
			friend bool operator>  (const UUID &self, const UUID &other);
		private :
			uuid_t uu;
	};
}
