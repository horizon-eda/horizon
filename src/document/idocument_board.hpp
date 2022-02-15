#pragma once
#include "idocument.hpp"

namespace horizon {
class IDocumentBoard : public virtual IDocument {
public:
    virtual class Board *get_board() = 0;
    virtual class GerberOutputSettings &get_gerber_output_settings() = 0;
    virtual class PDFExportSettings &get_pdf_export_settings() = 0;
    virtual class STEPExportSettings &get_step_export_settings() = 0;
    virtual class PnPExportSettings &get_pnp_export_settings() = 0;
    virtual class BoardColors &get_colors() = 0;
};
} // namespace horizon
