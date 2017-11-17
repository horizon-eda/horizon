#pragma once
#include <string>
#include <functional>

namespace horizon {

#ifdef G_OS_WIN32
	#undef ERROR
#endif

	enum class PoolUpdateStatus {INFO, FILE, ERROR, DONE};
	typedef std::function<void(PoolUpdateStatus, std::string)> pool_update_cb_t;

	void pool_update(const std::string &pool_base_path, pool_update_cb_t status_cb = nullptr);
}
