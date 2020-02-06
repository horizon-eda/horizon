#pragma once
#include "footprint_generator_base.hpp"
#include "widgets/spin_button_dim.hpp"
namespace horizon {
class FootprintGeneratorDual : public FootprintGeneratorBase {
public:
    FootprintGeneratorDual(class IDocumentPackage *c);
    bool generate() override;

private:
    Gtk::SpinButton *sp_count = nullptr;
    SpinButtonDim *sp_spacing = nullptr;
    SpinButtonDim *sp_spacing_outer = nullptr;
    SpinButtonDim *sp_spacing_inner = nullptr;
    SpinButtonDim *sp_pitch = nullptr;
    SpinButtonDim *sp_pad_width = nullptr;
    SpinButtonDim *sp_pad_height = nullptr;
    std::deque<sigc::connection> sp_spacing_connections;
    unsigned int pad_count = 4;
    bool zigzag = false;
    void update_preview();
    enum class Mode { SPACING, SPACING_OUTER, SPACING_INNER, PAD_HEIGHT };
    void update_spacing(Mode mode);
};
} // namespace horizon
