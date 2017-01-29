#pragma once


namespace horizon {
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
