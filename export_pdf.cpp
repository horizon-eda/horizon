#include "export_pdf.hpp"
#include "canvas/canvas_cairo.hpp"

namespace horizon {


	void export_pdf(const std::string &filename, const Schematic &sch, Core *c) {
		double width = 842;
		double height = 595;
		Cairo::RefPtr<Cairo::PdfSurface> surface = Cairo::PdfSurface::create(filename, width, height);

		Cairo::RefPtr<Cairo::Context> cr = Cairo::Context::create(surface);
		CanvasCairo ca(cr);
		double nm_to_pt = 352778;
		cr->scale(2.83465, -2.83465);

		for(const auto &it: sch.sheets) {
			width = it.second.frame.get_width()/nm_to_pt;
			height = it.second.frame.get_height()/nm_to_pt;
			surface->set_size(width, height);

			cr->save();
			double shift = it.second.frame.get_height()/1e6;
			cr->translate(0, -shift);
			ca.update(it.second);

			cr->show_page();
			cr->restore();
		}

	}


}
