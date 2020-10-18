#pragma once
#include "nlohmann/json_fwd.hpp"
#include "common/common.hpp"

namespace horizon {
using json = nlohmann::json;

class FileVersion {
public:
    FileVersion(unsigned int a);
    FileVersion(unsigned int a, unsigned int f);
    FileVersion(unsigned int a, const json &j);

    unsigned int get_app() const
    {
        return app;
    }

    unsigned int get_file() const
    {
        return file;
    }

    void serialize(json &j) const;

    void check(ObjectType type, const std::string &name, const class UUID &uu) const;

    std::string get_message(ObjectType type) const;

private:
    unsigned int app = 0;
    unsigned int file = 0;
};

} // namespace horizon
