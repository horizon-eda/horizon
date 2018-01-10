#include "pool_cached.hpp"
#include "padstack.hpp"
#include "package.hpp"
#include "part.hpp"
#include "unit.hpp"
#include "entity.hpp"
#include "symbol.hpp"
#include <glibmm/fileutils.h>
#include <glibmm/miscutils.h>
#include <giomm/file.h>

namespace horizon {
	PoolCached::PoolCached(const std::string &bp, const std::string &cp): Pool(bp), cache_path(cp) {}

	std::string PoolCached::get_filename(ObjectType ty, const UUID &uu) {
		auto filename_cached = Glib::build_filename(cache_path, get_flat_filename(ty, uu));
		if(Glib::file_test(filename_cached, Glib::FILE_TEST_IS_REGULAR)) {
			return filename_cached;
		}
		else {
			auto fn = Pool::get_filename(ty, uu);
			auto src = Gio::File::create_for_path(fn);
			auto dst = Gio::File::create_for_path(filename_cached);
			src->copy(dst);
			return fn;
		}
	}
}
