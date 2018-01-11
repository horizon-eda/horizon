#include "gerber_export.hpp"
#include "canvas_gerber.hpp"
#include "board/board.hpp"
#include "board/fab_output_settings.hpp"
#include <glibmm/miscutils.h>

namespace horizon {
	GerberExporter::GerberExporter(const Board *b, const FabOutputSettings *s): brd(b), settings(s) {

		for(const auto &it: settings->layers) {
			if(brd->get_layers().count(it.first) && it.second.enabled)
				writers.emplace(it.first, Glib::build_filename(settings->output_directory,  settings->prefix+it.second.filename));
		}

		drill_writer_pth = std::make_unique<ExcellonWriter>(Glib::build_filename(settings->output_directory, settings->prefix+settings->drill_pth_filename));
		if(settings->drill_mode == FabOutputSettings::DrillMode::INDIVIDUAL) {
			drill_writer_npth = std::make_unique<ExcellonWriter>(Glib::build_filename(settings->output_directory, settings->prefix+settings->drill_npth_filename));
		}
		else {
			drill_writer_npth = nullptr;
		}
	}

	void GerberExporter::generate() {
		CanvasGerber ca(this);
		ca.outline_width = settings->outline_width;
		ca.update(*brd);

		for(auto &it: writers) {
			it.second.write_format();
			it.second.write_apertures();
			it.second.write_regions();
			it.second.write_lines();
			it.second.write_pads();
			it.second.close();
			log << "Wrote layer "<< brd->get_layers().at(it.first).name  << " to gerber file " << it.second.get_filename() << std::endl;
		}
		for(auto it: {drill_writer_npth.get(), drill_writer_pth.get()}) {
			if(it) {
				it->write_format();
				it->write_header();
				it->write_holes();
				it->close();
				log << "Wrote excellon drill file " << it->get_filename() << std::endl;
			}
		}
	}

	std::string GerberExporter::get_log() {
		return log.str();
	}

	GerberWriter *GerberExporter::get_writer_for_layer(int l) {
		if(writers.count(l)) {
			return &writers.at(l);
		}
		return nullptr;
	}

	ExcellonWriter *GerberExporter::get_drill_writer(bool pth) {
		if(settings->drill_mode == FabOutputSettings::DrillMode::MERGED) {
			return drill_writer_pth.get();
		}
		else {
			return pth?drill_writer_pth.get():drill_writer_npth.get();
		}
	}
}
