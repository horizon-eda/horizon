#include "export_pdf.hpp"
#include "canvas_pdf.hpp"
#include <podofo/podofo.h>
#include "util/util.hpp"
#include "schematic/schematic.hpp"
#include "export_pdf_util.hpp"
#include "schematic/iinstancce_mapping_provider.hpp"
#include "util/bbox_accumulator.hpp"
#include "pool/part.hpp"

namespace horizon {

static void cb_nop(std::string, double)
{
}

class MyInstanceMappingProvider : public IInstanceMappingProvider {
public:
    MyInstanceMappingProvider(const Schematic &sch) : top(sch)
    {
    }
    void set_instance_path(const UUIDVec &p)
    {
        instance_path = p;
    }

    const class BlockInstanceMapping *get_block_instance_mapping() const override
    {
        if (instance_path.size())
            return &top.block->block_instance_mappings.at(instance_path);
        else
            return nullptr;
    }

    unsigned int get_sheet_index_for_path(const class UUID &sheet, const UUIDVec &path) const
    {
        return top.sheet_mapping.sheet_numbers.at(uuid_vec_append(path, sheet));
    }

    unsigned int get_sheet_index(const class UUID &sheet) const override
    {
        return get_sheet_index_for_path(sheet, instance_path);
    }

    unsigned int get_sheet_total() const override
    {
        return top.sheet_mapping.sheet_total;
    }


private:
    const Schematic &top;
    UUIDVec instance_path;
};

#if PODOFO_VERSION_MAJOR != 0 || PODOFO_VERSION_MINOR != 9 || PODOFO_VERSION_PATCH != 5
#define HAVE_OUTLINE
#endif

using Callback = std::function<void(std::string, double)>;

class PDFExporter {
public:
    PDFExporter(const class PDFExportSettings &settings, Callback callback)
        : document(settings.output_filename.c_str()), font(document.CreateFont("Helvetica")),
          canvas(painter, *font, settings), cb(callback)
    {
        canvas.use_layer_colors = false;
    }

    void export_pdf(const class Schematic &sch)
    {
        cb("Initializing", 0);
        auto info = document.GetInfo();
        info->SetCreator("Horizon EDA");
        info->SetProducer("Horizon EDA");
        if (sch.block->project_meta.count("author")) {
            info->SetAuthor(sch.block->project_meta.at("author"));
        }
        std::string title = "Schematic";
        if (sch.block->project_meta.count("project_title")) {
            title = sch.block->project_meta.at("project_title");
        }
        info->SetTitle(title);
        MyInstanceMappingProvider prv(sch);

#ifdef HAVE_OUTLINE
        outlines = document.GetOutlines();
        PoDoFo::PdfOutlineItem *proot = nullptr;
#else
        PoDoFo::PdfOutlineItem *proot = nullptr;
#endif

        export_schematic(sch, {}, prv, proot);
        for (auto &[path, number, rect] : annotations) {
            auto page = document.GetPage(number);
            auto annot = page->CreateAnnotation(PoDoFo::ePdfAnnotation_Link, rect);
            annot->SetBorderStyle(0, 0, 0);
            annot->SetDestination(first_pages.at(path));
        }
        for (auto &[url, number, rect] : datasheet_annotations) {
            auto page = document.GetPage(number);
            auto annot = page->CreateAnnotation(PoDoFo::ePdfAnnotation_Link, rect);
            annot->SetBorderStyle(0, 0, 0);

            PoDoFo::PdfAction action(PoDoFo::ePdfAction_URI, &document);
            action.SetURI(PoDoFo::PdfString(url));
            annot->SetAction(action);
        }
        document.Close();
    }

private:
    PoDoFo::PdfStreamedDocument document;
    PoDoFo::PdfPainterMM painter;
    PoDoFo::PdfFont *font = nullptr;
    std::map<UUIDVec, PoDoFo::PdfDestination> first_pages;
    std::vector<std::tuple<UUIDVec, unsigned int, PoDoFo::PdfRect>> annotations;
    std::vector<std::tuple<std::string, unsigned int, PoDoFo::PdfRect>> datasheet_annotations;
    PoDoFo::PdfOutlines *outlines = nullptr;
    CanvasPDF canvas;
    Callback cb;

    void export_schematic(const Schematic &sch, const UUIDVec &path, MyInstanceMappingProvider &prv,
                          PoDoFo::PdfOutlineItem *parent)
    {
        if (Block::instance_path_too_long(path, __FUNCTION__))
            return;
        prv.set_instance_path(path);
        Schematic my_sch = sch;
        my_sch.expand(false, &prv);
        bool first = true;
        auto sheets = my_sch.get_sheets_sorted();
        for (const auto sheet : sheets) {
            const auto idx = prv.get_sheet_index_for_path(sheet->uuid, path);
            const auto progress = (double)idx / prv.get_sheet_total();
            cb("Exporting sheet " + format_m_of_n(idx, prv.get_sheet_total()), progress);
            auto page =
                    document.CreatePage(PoDoFo::PdfRect(0, 0, to_pt(sheet->frame.width), to_pt(sheet->frame.height)));
            painter.SetPage(page);
            painter.SetLineCapStyle(PoDoFo::ePdfLineCapStyle_Round);
            painter.SetFont(font);
            painter.SetColor(0, 0, 0);
            painter.SetTextRenderingMode(PoDoFo::ePdfTextRenderingMode_Invisible);

            for (const auto &[uu, pic] : sheet->pictures) {
                if (!pic.on_top)
                    render_picture(document, painter, pic);
            }
            for (const auto &[uu_sym, sym] : sheet->block_symbols) {
                for (const auto &[uu, pic] : sym.symbol.pictures) {
                    if (!pic.on_top)
                        render_picture(document, painter, pic, sym.placement);
                }
            }

            canvas.update(*sheet);

            for (const auto &[uu, pic] : sheet->pictures) {
                if (pic.on_top)
                    render_picture(document, painter, pic);
            }
            for (const auto &[uu_sym, sym] : sheet->block_symbols) {
                for (const auto &[uu, pic] : sym.symbol.pictures) {
                    if (pic.on_top)
                        render_picture(document, painter, pic, sym.placement);
                }
            }

            auto dest = PoDoFo::PdfDestination(page);
            if (first) {
                first_pages.emplace(path, dest);
                first = false;
            }

            {
                const auto &items = canvas.get_selectables().get_items();
                const auto &items_ref = canvas.get_selectables().get_items_ref();
                const auto n = items.size();
                for (size_t i = 0; i < n; i++) {
                    const auto &it = items.at(i);
                    const auto &ir = items_ref.at(i);
                    if (ir.type == ObjectType::SCHEMATIC_BLOCK_SYMBOL) {
                        if (it.is_box()) {
                            BBoxAccumulator<float> acc;
                            for (const auto &c : it.get_corners()) {
                                acc.accumulate(c);
                            }
                            const auto [a, b] = acc.get();
                            PoDoFo::PdfRect rect(to_pt(a.x), to_pt(a.y), to_pt(b.x - a.x), to_pt(b.y - a.y));
                            annotations.emplace_back(
                                    uuid_vec_append(path, sheet->block_symbols.at(ir.uuid).block_instance->uuid),
                                    page->GetPageNumber() - 1, rect);
                        }
                    }
                    else if (ir.type == ObjectType::SCHEMATIC_SYMBOL) {
                        if (it.is_box()) {
                            const auto &sym = sheet->symbols.at(ir.uuid);
                            if (sym.component->part && sym.component->part->get_datasheet().size()) {
                                BBoxAccumulator<float> acc;

                                for (const auto &c : it.get_corners()) {
                                    acc.accumulate(c);
                                }
                                const auto [a, b] = acc.get();
                                PoDoFo::PdfRect rect(to_pt(a.x), to_pt(a.y), to_pt(b.x - a.x), to_pt(b.y - a.y));
                                datasheet_annotations.emplace_back(sym.component->part->get_datasheet(),
                                                                   page->GetPageNumber() - 1, rect);
                            }
                        }
                    }
                }
            }

            painter.FinishPage();

#ifdef HAVE_OUTLINE
            PoDoFo::PdfOutlineItem *sheet_node;
            if (parent) {
                sheet_node = parent->CreateChild(sheet->name, dest);
            }
            else {
                sheet_node = outlines->CreateRoot(sheet->name);
                sheet_node->SetDestination(dest);
            }
#endif

            for (auto sym : sheet->get_block_symbols_sorted()) {
#ifdef HAVE_OUTLINE
                auto sym_node = sheet_node->CreateChild(sym->block_instance->refdes, dest);
                sym_node->SetTextFormat(PoDoFo::ePdfOutlineFormat_Italic);
#else
                PoDoFo::PdfOutlineItem *sym_node = nullptr;
#endif
                export_schematic(*sym->schematic, uuid_vec_append(path, sym->block_instance->uuid), prv, sym_node);
            }
        }
    }
};

void export_pdf(const class Schematic &sch, const class PDFExportSettings &settings, Callback cb)
{
    if (!cb)
        cb = &cb_nop;
    PDFExporter ex(settings, cb);
    ex.export_pdf(sch);
}
} // namespace horizon
