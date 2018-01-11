#pragma once
#include <glibmm/checksum.h>
#include "common/common.hpp"

namespace horizon {
	class GerberHash {
		public:
			GerberHash();
			void update(const class Padstack &padstack);
			std::string get_digest();
			static std::string hash(const class Padstack &padstack);

		private:
			Glib::Checksum checksum;

			void update(const class Hole &hole);
			void update(const class Shape &shape);
			void update(int64_t i);
			void update(const Coordi &c);
			void update(const class Placement &p);
			void update(const class Polygon &p);
	};
};
