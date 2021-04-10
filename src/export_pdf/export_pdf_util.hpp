#pragma once
#include <podofo/podofo.h>

namespace horizon {
void render_picture(PoDoFo::PdfDocument &doc, PoDoFo::PdfPainterMM &painter, const class Picture &pic);
}
