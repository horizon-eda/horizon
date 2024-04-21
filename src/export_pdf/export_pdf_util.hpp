#pragma once
#include "util/podofo_inc.hpp"
#include "util/placement.hpp"

namespace horizon {
void render_picture(PoDoFo::PdfDocument &doc, PoDoFo::PdfPainter &painter, const class Picture &pic,
                    const Placement &tr = Placement());

PoDoFo::PdfFont &load_font(PoDoFo::PdfDocument &document);

} // namespace horizon
