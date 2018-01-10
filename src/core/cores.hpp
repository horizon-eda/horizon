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
	class Cores {
		public :
			Cores(class Core *co);
			Core *r;
			class CoreSchematic *c;
			class CoreSymbol *y;
			class CorePadstack *a;
			class CorePackage *k;
			class CoreBoard *b;
	};

}
