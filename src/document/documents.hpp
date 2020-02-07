#pragma once

namespace horizon {

/**
 * Tools use this class to actually access the core.
 * Since it doesn't make sense to shoehorn every core
 * into one interface, this class provides access to all
 * Core subclasses. Only one of c, y, etc. is not null.
 * r is always not null. Thus when accessing something on
 * a schematic, a Tool uses c.
 */
class Documents {
public:
    Documents(class IDocument *co);
    class IDocument *r;
    class IDocumentSchematic *c;
    class IDocumentSymbol *y;
    class IDocumentPadstack *a;
    class IDocumentPackage *k;
    class IDocumentBoard *b;
    class IDocumentFrame *f;
};
} // namespace horizon
