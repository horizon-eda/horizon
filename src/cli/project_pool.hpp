#pragma once
#include "temp_dir.hpp"
#include "pool/pool.hpp"
#include <memory>

namespace horizon::cli {
class ExportPool {
public:
    // Copy and index the local pool for this export, including model files when STEP export needs them
    explicit ExportPool(const std::string &directory, bool include_models = false);
    // Return the indexed pool, which stays available for as long as this ExportPool exists
    Pool &get_pool()
    {
        return *pool;
    }

private:
    TempDir temporary;
    std::unique_ptr<Pool> pool;
};
} // namespace horizon::cli
