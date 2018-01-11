#include "export_pdf/export_pdf.hpp"
#include "canvas/canvas_cairo.hpp"

namespace horizon {


	void export_pdf(const std::string &filename, const Schematic &sch, Core *c) {
		double width = 842;
		double height = 595;
		Cairo::RefPtr<Cairo::PdfSurface> surface = Cairo::PdfSurface::create(filename, width, height);

		Cairo::RefPtr<Cairo::Context> cr = Cairo::Context::create(surface);
		CanvasCairo ca(cr);
		{
			auto &sheet = sch.sheets.begin()->second;
			for(const auto &la: sheet.get_layers()) {
				ca.set_layer_display(la.first, LayerDisplay(true, LayerDisplay::Mode::FILL, la.second.color));
			}
		}
		double nm_to_pt = 352778;
		cr->scale(2.83465, -2.83465);
		std::vector<const Sheet*> sheets;
		for(const auto &it: sch.sheets) {
			sheets.push_back(&it.second);
		}
		std::sort(sheets.begin(), sheets.end(), [](auto a, auto b){return a->index < b->index;});

		for(const auto sheet: sheets) {
			width = sheet->frame.get_width()/nm_to_pt;
			height = sheet->frame.get_height()/nm_to_pt;
			surface->set_size(width, height);

			cr->save();
			double shift = sheet->frame.get_height()/1e6;
			cr->translate(0, -shift);
			ca.update(*sheet);

			cr->show_page();
			cr->restore();
		}

	}


}
