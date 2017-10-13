#pragma once
#include "gerber_writer.hpp"
#include "excellon_writer.hpp"
#include <memory>
#include <sstream>

namespace horizon {
	class GerberExporter {
		friend class CanvasGerber;
		public :
			GerberExporter(const class Board *b, const class FabOutputSettings *s);
			void generate();
			std::string get_log();

		private:
			const class Board *brd;
			const class FabOutputSettings *settings;
			std::map<int, GerberWriter> writers;
			GerberWriter *get_writer_for_layer(int l);
			ExcellonWriter *get_drill_writer(bool pth);
			std::unique_ptr<ExcellonWriter> drill_writer_pth;
			std::unique_ptr<ExcellonWriter> drill_writer_npth;
			std::stringstream log;
	};
}
