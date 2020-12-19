#include "tool_import_kicad_package.hpp"
#include "document/idocument_package.hpp"
#include "document/idocument_decal.hpp"
#include "imp/imp_interface.hpp"
#include "util/kicad_package_parser.hpp"
#include "sexpr/sexpr_parser.h"
#include "util/util.hpp"
#include <iostream>
#include <sstream>

namespace horizon {

bool ToolImportKiCadPackage::can_begin()
{
    return doc.k || doc.d;
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
    if (auto r = imp->dialogs.ask_kicad_package_filename()) {
        SEXPR::PARSER parser;
        std::unique_ptr<SEXPR::SEXPR> sexpr_data(parser.Parse(slurp_from_file(*r)));
        if (doc.k) {
            KiCadPackageParser kp(doc.k->get_package(), doc.k->get_pool());
            kp.parse(sexpr_data.get());
        }
        else if (doc.d) {
            KiCadDecalParser kp(doc.d->get_decal());
            kp.parse(sexpr_data.get());
        }
        else {
            return ToolResponse::end();
        }
        return ToolResponse::commit();
    }
    else {
        return ToolResponse::end();
    }
}

ToolResponse ToolImportKiCadPackage::update(const ToolArgs &args)
{

    return ToolResponse();
}
} // namespace horizon
