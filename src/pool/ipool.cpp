#include "ipool.hpp"
#include "common/common.hpp"

namespace horizon {
const std::map<ObjectType, std::string> IPool::type_names = {
        {ObjectType::UNIT, "units"},         {ObjectType::SYMBOL, "symbols"},   {ObjectType::ENTITY, "entities"},
        {ObjectType::PADSTACK, "padstacks"}, {ObjectType::PACKAGE, "packages"}, {ObjectType::PART, "parts"},
        {ObjectType::FRAME, "frames"},       {ObjectType::DECAL, "decals"},
};
}
