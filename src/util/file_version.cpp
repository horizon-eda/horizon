#include "file_version.hpp"
#include "nlohmann/json.hpp"
#include "logger/logger.hpp"
#include "common/object_descr.hpp"
#include "util/uuid.hpp"

namespace horizon {
FileVersion::FileVersion(unsigned int a) : app(a), file(a)
{
}

FileVersion::FileVersion(unsigned int a, unsigned int f) : app(a), file(f)
{
}

FileVersion::FileVersion(unsigned int a, const json &j) : app(a), file(j.value("version", 0))
{
}

void FileVersion::serialize(json &j) const
{
    if (app)
        j["version"] = app;
}

void FileVersion::check(ObjectType type, const std::string &name, const class UUID &uu) const
{
    if (file > app) {
        Logger::log_critical(object_descriptions.at(type).name + " " + name + " file version is newer than app",
                             Logger::Domain::VERSION,
                             "File:" + std::to_string(file) + " App:" + std::to_string(app)
                                     + " UUID:" + (std::string)uu);
    }
}

std::string FileVersion::get_message(ObjectType type) const
{
    const auto &t = object_descriptions.at(type).name;
    if (app > file) {
        return "This " + t + " has been created with an older version of Horizon EDA. Saving will update it to the latest version that might be incompatible with older versions of Horizon EDA.";
    }
    else if (file > app) {
        return "This " + t + " has been created with a newer version of Horizon EDA. Some content may not display correctly. To preserve fidelity, this " + t + " has been opened read-only.";
    }
    else {
        return "";
    }
}
} // namespace horizon
