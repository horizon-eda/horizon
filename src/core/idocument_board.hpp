#pragma once
#include "idocument.hpp"

namespace horizon {
class IDocumentBoard : public virtual IDocument {
public:
    virtual class Board *get_board() = 0;
    virtual class ViaPadstackProvider *get_via_padstack_provider() = 0;
    virtual class FabOutputSettings *get_fab_output_settings() = 0;
    virtual class PDFExportSettings *get_pdf_export_settings() = 0;
};
} // namespace horizon
