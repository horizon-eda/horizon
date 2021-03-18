#pragma once
#include "common/picture.hpp"
#include <list>

namespace horizon {
void pictures_load(const std::list<std::map<UUID, Picture> *> &pictures, const std::string &dir,
                   const std::string &suffix);

void pictures_save(const std::list<const std::map<UUID, Picture> *> &pictures, const std::string &dir,
                   const std::string &suffix);

class PictureKeeper {
public:
    void save(const std::map<UUID, Picture> &pics);
    void restore(std::map<UUID, Picture> &pics);

private:
    std::map<UUID, std::shared_ptr<const PictureData>> pictures;
};

} // namespace horizon
