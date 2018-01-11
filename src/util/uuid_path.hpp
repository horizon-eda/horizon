#pragma once
#include "util/uuid.hpp"
#include <iostream>
#include <array>

namespace horizon {

	/**
	 * Stores a sequence of up to 3 UUIDs.
	 * Is used in various places for referencing objects owned by another object,
	 * i.e. Pin of a Unit
	 */
	template<unsigned int N> class UUIDPath {
		public :
			UUIDPath() {}
			UUIDPath(const UUID &uu):path({uu, uu}) {}
			UUIDPath(const UUID &uu0, const UUID &uu1):path({uu0, uu1}) {}
			UUIDPath(const UUID &uu0, const UUID &uu1, const UUID &uu2):path({uu0, uu1, uu2}) {}
			/**
			 * Constructs UUIDPath from up to two UUIDs separated by a /
			 */
			UUIDPath(const std::string &str) {
				if(N==1) {
					path[0] = str;
				}
				if(N==2) {
					path[0] = str.substr(0, 36);
					path[1] = str.substr(37, 36);
				}
			}
			/**
			 * @returns a string representation of the path with the individual UUIDs separated by a /
			 */
			operator std::string() const {
				if(N==1) {
					return path[0];
				}if(N==2) {
					return (std::string)path[0]+ "/" + (std::string)path[1];
				}
			}
			bool operator<(const UUIDPath<N> &other) const {
				for(unsigned int i(0);i<N;i++) {
					if(path[i]<other.path[i]) {
						return true;
					}
					if(path[i]>other.path[i]) {
						return false;
					}
				}
				return false;
			}
			bool operator==(const UUIDPath<N> &other) const {
				for(unsigned int i(0);i<N;i++) {
					if(path[i] != other.path[i]) {
						return false;
					}
				}
				return true;
			}
			const UUID &at(unsigned int i) const {
				return path.at(i);
			}


		private :
			std::array<UUID, 3> path;
	};
}
