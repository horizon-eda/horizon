#include "documents.hpp"
#include "idocument.hpp"
#include "idocument_board.hpp"
#include "idocument_package.hpp"
#include "idocument_padstack.hpp"
#include "idocument_schematic.hpp"
#include "idocument_symbol.hpp"
#include "idocument_frame.hpp"
#include "idocument_decal.hpp"
#include "idocument_block_symbol.hpp"
#include <assert.h>

namespace horizon {
Documents::Documents(IDocument *co)
    : r(co), c(nullptr), y(dynamic_cast<IDocumentSymbol *>(r)), o(nullptr), a(dynamic_cast<IDocumentPadstack *>(r)),
      k(dynamic_cast<IDocumentPackage *>(r)), b(dynamic_cast<IDocumentBoard *>(r)),
      f(dynamic_cast<IDocumentFrame *>(r)), d(dynamic_cast<IDocumentDecal *>(r))
{
    if (auto bs = dynamic_cast<IDocumentBlockSymbol *>(r)) {
        if (bs->get_block_symbol_mode())
            o = bs;
        else
            assert(c = dynamic_cast<IDocumentSchematic *>(r));
    }
}
} // namespace horizon
