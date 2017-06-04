#pragma once
#include "gerber_writer.hpp"
#include "excellon_writer.hpp"
#include "cam_job.hpp"
#include <memory>
#include <sstream>

namespace horizon {
	class GerberExporter {
		friend class CanvasGerber;
		public :
			GerberExporter(const class Board *b, const CAMJob &j, const std::string &prefix);
			void save();
			std::string get_log();

		private:
			const Board *brd;
			const CAMJob &job;
			std::map<int, GerberWriter> writers;
			GerberWriter *get_writer_for_layer(int l);
			ExcellonWriter *get_drill_writer(bool pth);
			std::unique_ptr<ExcellonWriter> drill_writer_pth;
			std::unique_ptr<ExcellonWriter> drill_writer_npth;
			std::stringstream log;
	};
}
