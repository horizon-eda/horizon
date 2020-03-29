#include "tool_import_kicad_package.hpp"
#include "document/idocument_package.hpp"
#include "pool/package.hpp"
#include "imp/imp_interface.hpp"
#include "util/kicad_package_parser.hpp"
#include "sexpr/sexpr_parser.h"
#include "util/util.hpp"
#include <iostream>
#include <sstream>

namespace horizon {

ToolImportKiCadPackage::ToolImportKiCadPackage(IDocument *c, ToolID tid) : ToolBase(c, tid)
{
}

bool ToolImportKiCadPackage::can_begin()
{
    return doc.k;
}

static std::string slurp_from_file(const std::string &filename)
{
    auto ifs = make_ifstream(filename);
    if (!ifs.is_open()) {
        throw std::runtime_error("not opened");
    }
    std::stringstream text;
    text << ifs.rdbuf();
    return text.str();
}


ToolResponse ToolImportKiCadPackage::begin(const ToolArgs &args)
{
    bool r;
    std::string filename;

    std::tie(r, filename) = imp->dialogs.ask_kicad_package_filename();
    if (!r) {
        return ToolResponse::end();
    }

    SEXPR::PARSER parser;
    std::unique_ptr<SEXPR::SEXPR> sexpr_data(parser.Parse(slurp_from_file(filename)));
    KiCadPackageParser kp(*doc.k->get_package(), doc.k->get_pool());
    kp.parse(sexpr_data.get());
    return ToolResponse::commit();
}

ToolResponse ToolImportKiCadPackage::update(const ToolArgs &args)
{

    return ToolResponse();
}
} // namespace horizon
