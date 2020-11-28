#pragma once
#include <functional>
#include <string>
#include <vector>
#include "util/uuid.hpp"
#include <set>
#include "util/win32_undef.hpp"

namespace horizon {

enum class PoolUpdateStatus { INFO, FILE, FILE_ERROR, ERROR, DONE };
typedef std::function<void(PoolUpdateStatus, std::string, std::string)> pool_update_cb_t;

void pool_update(const std::string &pool_base_path, pool_update_cb_t status_cb = nullptr, bool parametric = false,
                 const std::vector<std::string> &filenames = {});
void pool_update_parametric(class PoolUpdatePool &pool, pool_update_cb_t status_cb = nullptr,
                            const std::set<UUID> &parts = {});
} // namespace horizon
