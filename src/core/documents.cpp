#include "documents.hpp"
#include "idocument.hpp"
#include "idocument_board.hpp"
#include "idocument_package.hpp"
#include "idocument_padstack.hpp"
#include "idocument_schematic.hpp"
#include "idocument_symbol.hpp"
#include "idocument_frame.hpp"

namespace horizon {
Documents::Documents(IDocument *co)
    : r(co), c(dynamic_cast<IDocumentSchematic *>(r)), y(dynamic_cast<IDocumentSymbol *>(r)),
      a(dynamic_cast<IDocumentPadstack *>(r)), k(dynamic_cast<IDocumentPackage *>(r)),
      b(dynamic_cast<IDocumentBoard *>(r)), f(dynamic_cast<IDocumentFrame *>(r))
{
}
} // namespace horizon
