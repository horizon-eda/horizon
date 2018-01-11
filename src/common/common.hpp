#pragma once
#include <stdint.h>
#include <vector>
#include <algorithm>
#include <type_traits>
#include <math.h>
#include <array>
#include "common/lut.hpp"

namespace horizon {
	enum class Orientation {LEFT, RIGHT, UP, DOWN};
	/**
	 * Used in conjunction with a UUID/UUIDPath to identify an object.
	 */
	enum class ObjectType {
		INVALID, JUNCTION, LINE, SYMBOL_PIN, ARC, SCHEMATIC_SYMBOL,
		TEXT, LINE_NET, COMPONENT, NET, NET_LABEL, POWER_SYMBOL, BUS,
		BUS_LABEL, BUS_RIPPER, POLYGON, POLYGON_VERTEX, POLYGON_EDGE, POLYGON_ARC_CENTER,
		HOLE, PAD, BOARD_PACKAGE, TRACK, VIA, SHAPE, BOARD, SCHEMATIC,
		UNIT, ENTITY, SYMBOL, PACKAGE, PADSTACK, PART, PLANE, DIMENSION, NET_CLASS, BOARD_HOLE
	};
	enum class PatchType {OTHER, TRACK, PAD, PAD_TH, VIA, PLANE, HOLE_PTH, HOLE_NPTH, BOARD_EDGE, TEXT};

	extern const LutEnumStr<PatchType> patch_type_lut;
	extern const LutEnumStr<ObjectType> object_type_lut;
	extern const LutEnumStr<Orientation> orientation_lut;

	/**
	 * Your typical coordinate class.
	 * Supports some mathematical operators as required.
	 * Unless otherwise noted, 1 equals 1 nm (that is nanometer, not nautical mile)
	 * Instead of instantiating the template on your own, you want to use Coordf (float) for calculations
	 * that will end up only on screen and Coordi (int64_t) for everything else.
	 */
	template <typename T>class Coord {
		public :
			T x;
			T y;
			
			//WTF, but works
			//template<typename U = T>
			//Coord(double ix, double iy, typename std::enable_if<std::is_same<U, float>::value>::type* = 0) : x((float)ix), y((float)iy) { }
			
			
			Coord(T ix, T iy): x(ix), y(iy) {}
			Coord(): x(0), y(0) {}
			Coord(std::vector<T> v): x(v.at(0)), y(v.at(1)) {}
			operator Coord<float>() const {return Coord<float>(x, y);}
			operator Coord<double>() const {return Coord<double>(x, y);}
			Coord<T> operator+ (const Coord<T> &a) const {return Coord<T>(x+a.x, y+a.y);}
			Coord<T> operator- (const Coord<T> &a) const {return Coord<T>(x-a.x, y-a.y);}
			Coord<T> operator* (const Coord<T> &a) const {return Coord<T>(x*a.x, y*a.y);}
			Coord<T> operator* (T r) const {return Coord<T>(x*r, y*r);}
			Coord<T> operator/ (T r) const {return Coord<T>(x/r, y/r);}
			bool operator== (const Coord<T>  &a) const {return a.x == x && a.y==y;}
			bool operator!= (const Coord<T>  &a) const {return !(a==*this);}
			bool operator< (const Coord<T>  &a) const {
				if(x < a.x)
					return true;
				if(x > a.x)
					return false;
				return y < a.y;
			}
			
			/**
			 * @returns element-wise minimum of \p a and \p b
			 */
			static Coord<T> min(const Coord<T> &a, const Coord<T> &b) {
				return Coord<T>(std::min(a.x, b.x), std::min(a.y, b.y));
			}

			/**
			 * @returns element-wise maximum of \p a and \p b
			 */
			static Coord<T> max(const Coord<T> &a, const Coord<T> &b) {
				return Coord<T>(std::max(a.x, b.x), std::max(a.y, b.y));
			}
			
			/**
			 * @param r magnitude
			 * @param phi angle in radians
			 * @returns coordinate specified by \p r and \p phi
			 */
			static Coord<float> euler(float r, float phi) {
				return Coord<float>(r*cos(phi), r*sin(phi));
			}
			
			/**
			 * @param a other coordinate
			 * @returns dot product of \p a and this
			 */
			T dot(const Coord<T> &a) const {return x*a.x+y*a.y;}

			/**
			 * @returns squared magnitude of this
			 */
			T mag_sq() const {return x*x+y*y;}

			void operator+= (const Coord<T> a) {x+=a.x; y+=a.y;}
			void operator-= (const Coord<T> a) {x-=a.x; y-=a.y;}
			void operator*= (T a) {x*=a; y*=a;}
			/*json serialize() {
				return {x,y};
			}*/
			std::array<T, 2> as_array () const {return {x, y};}
	};
	

	typedef Coord<float> Coordf;
	typedef Coord<int64_t> Coordi;
	
	class Color {
		public :
			float r;
			float g;
			float b;
			Color(double ir, double ig, double ib): r(ir), g(ig), b(ib) {}
			//Color(unsigned int ir, unsigned ig, unsigned ib): r(ir/255.), g(ig/255.), b(ib/255.) {}
			static Color new_from_int(unsigned int ir, unsigned ig, unsigned ib) {
				return Color(ir/255.0, ig/255.0, ib/255.0);
				
			}
			Color(): r(0), g(0), b(0) {}
	};
	
	constexpr int64_t operator "" _mm(long double i) {
		return i*1e6;
	}
	constexpr int64_t operator "" _mm(unsigned long long int i) {
		return i*1000000;
	}

}

