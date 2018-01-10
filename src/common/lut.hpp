#pragma once
#include <map>

namespace horizon {

	/**
	 * Trivial bidirectional map for mapping strings to enums.
	 * Used for serializing and derserializing objects to/from json.
	 */
	template <typename T>class LutEnumStr {
		static_assert(std::is_enum<T>::value, "Must be an enum type");
		public :
			LutEnumStr(std::initializer_list<std::pair<const std::string, const T>> s) {
				for(auto it: s) {
					fwd.insert(it);
					rev.insert(std::make_pair(it.second, it.first));
				}
			}
			/**
			 * @returns the enum corresponding to string \p s
			 */
			const T lookup(const std::string &s) const {
				return fwd.at(s);
			}

			const T lookup(const std::string &s, T def) const {
				if(fwd.count(s))
					return fwd.at(s);
				else
					return def;
			}

			/**
			 * @returns the string corresponding to enum \p s
			 */
			const std::string &lookup_reverse(const T s) const {
				return rev.at(s);
			}
			
		private :
			std::map<std::string, T> fwd;
			std::map<T, std::string> rev;
		
	};
	
}
