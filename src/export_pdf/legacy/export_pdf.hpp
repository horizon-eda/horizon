#pragma once
#include <functional>
#include <string>

namespace horizon {
void export_pdf(const class Schematic &sch, const class PDFExportSettings &settings,
                std::function<void(std::string, double)> cb = nullptr);
}
