#pragma once

namespace horizon {
	template <typename T>class Accumulator {
		public:
			Accumulator():
				value()
			{}
			void accumulate(const T &v) {
				value = (value*n+v)/(n+1);
				n++;
			}
			T get() {
				return value;
			}
			size_t get_n() {
				return n;
			}
		private:
			T value;
			size_t n = 0;
	};
}
