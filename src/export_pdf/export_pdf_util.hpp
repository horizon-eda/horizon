#pragma once
#include <podofo/podofo.h>
#include "util/placement.hpp"

namespace horizon {
void render_picture(PoDoFo::PdfDocument &doc, PoDoFo::PdfPainter &painter, const class Picture &pic,
                    const Placement &tr = Placement());
}
