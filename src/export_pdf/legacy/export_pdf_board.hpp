#pragma once
#include <functional>

namespace horizon {
void export_pdf(const class Board &brd, const class PDFExportSettings &settings,
                std::function<void(std::string, double)> cb = nullptr);
}
