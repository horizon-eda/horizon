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
Documents::Documents(IDocument *doc)
    : r(doc), c(nullptr), y(dynamic_cast<IDocumentSymbol *>(r)), o(nullptr),
      co(dynamic_cast<IDocumentSchematicBlockSymbol *>(r)), a(dynamic_cast<IDocumentPadstack *>(r)),
      k(dynamic_cast<IDocumentPackage *>(r)), b(dynamic_cast<IDocumentBoard *>(r)),
      f(dynamic_cast<IDocumentFrame *>(r)), d(dynamic_cast<IDocumentDecal *>(r))
{
    if (co) {
        if (co->get_block_symbol_mode())
            assert(o = dynamic_cast<IDocumentBlockSymbol *>(r));
        else
            assert(c = dynamic_cast<IDocumentSchematic *>(r));
    }
}
} // namespace horizon
