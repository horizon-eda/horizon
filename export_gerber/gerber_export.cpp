#include "gerber_export.hpp"
#include "canvas_gerber.hpp"

namespace horizon {
	GerberExporter::GerberExporter(CoreBoard *c, const CAMJob &j, const std::string &prefix): core(c), job(j) {

		for(const auto &it: job.layers) {
			if(core->get_layers().count(it.first))
				writers.emplace(it.first, prefix+it.second);
		}

		drill_writer_pth = std::make_unique<ExcellonWriter>(prefix+job.drill_pth);
		if(!job.merge_npth) {
			drill_writer_npth = std::make_unique<ExcellonWriter>(prefix+job.drill_npth);
		}
	}

	void GerberExporter::save() {
		CanvasGerber ca(this);
		ca.set_core(core);
		ca.update(*core->get_board());

		for(auto &it: writers) {
			it.second.write_format();
			it.second.write_apertures();
			it.second.write_lines();
			it.second.write_pads();
			it.second.close();
			log << "Wrote layer "<< core->get_layers().at(it.first).name  << " to gerber file " << it.second.get_filename() << std::endl;
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
		if(job.merge_npth) {
			return drill_writer_pth.get();
		}
		else {
			return pth?drill_writer_pth.get():drill_writer_npth.get();
		}
	}
}
