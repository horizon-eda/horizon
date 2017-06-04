#include "export_pdf.hpp"
#include "canvas/canvas_cairo.hpp"

namespace horizon {


	void export_pdf(const std::string &filename, const Schematic &sch, Core *c) {
		int width = 842;
		int height = 595;
		Cairo::RefPtr<Cairo::PdfSurface> surface = Cairo::PdfSurface::create(filename, width, height);

		Cairo::RefPtr<Cairo::Context> cr = Cairo::Context::create(surface);
		CanvasCairo ca(cr);
		for(const auto &it: sch.sheets) {
			ca.update(it.second);

			cr->show_page();
		}

	}


}
