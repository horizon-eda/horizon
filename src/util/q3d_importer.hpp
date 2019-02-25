#pragma once
#include <string>

#include "flatbuffers/flatbuffers.h"
#include "object_generated.h"

#include "str_util.hpp"

namespace Q3DImporter {
class Object {
public:
    Object(const std::string &filename) : buffer(nullptr)
    {
        std::ifstream ifs;
        ifs.open(filename.c_str(), std::ios::binary | std::ios::in);
        if (!ifs.is_open()) {
            std::cout << "error loading" << filename << std::endl;
            return;
        }
        ifs.seekg(0, std::ios::end);
        const int length = ifs.tellg();
        ifs.seekg(0, std::ios::beg);

        char *data = new char[length];
        ifs.read(data, length);
        ifs.close();

        auto ver = flatbuffers::Verifier((uint8_t *)data, length);
        if (!q3d::VerifyObjectBuffer(ver)) {
            std::cout << "invalid Q3D" << filename << std::endl;
            delete[] data;
            return;
        }

        buffer.reset(data);
    }

    const q3d::Object *getRoot()
    {
        return q3d::GetObject(buffer.get());
    }

private:
    std::unique_ptr<char> buffer;
};
} // namespace Q3DImporter
