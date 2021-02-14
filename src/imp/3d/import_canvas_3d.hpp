#pragma once
#include "canvas3d/canvas3d.hpp"

namespace horizon {
class ImpPackage::ImportCanvas3D : public Canvas3D {
public:
    ImportCanvas3D(class ImpPackage &aimp) : imp(aimp)
    {
    }

protected:
    STEPImporter::Faces import_step(const std::string &filename_rel, const std::string &filename_abs) override;

private:
    ImpPackage &imp;
};
} // namespace horizon
