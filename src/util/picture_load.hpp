#pragma once
#include "common/picture.hpp"
#include <list>

namespace horizon {
void pictures_load(const std::list<std::map<UUID, Picture> *> &pictures, const std::string &dir,
                   const std::string &suffix);

void pictures_save(const std::list<const std::map<UUID, Picture> *> &pictures, const std::string &dir,
                   const std::string &suffix);

} // namespace horizon
