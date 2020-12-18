#include "clipboard_package.hpp"
#include "document/idocument_package.hpp"
#include "pool/package.hpp"
#include "nlohmann/json.hpp"

namespace horizon {
void ClipboardPackage::serialize(json &j)
{
    ClipboardBase::serialize(j);
    j["pads"] = json::object();

    const auto &pkg = doc.get_package();
    for (const auto &it : selection) {
        switch (it.type) {
        case ObjectType::PAD:
            j["pads"][(std::string)it.uuid] = pkg.pads.at(it.uuid).serialize();
            break;

        default:;
        }
    }
}

IDocument &ClipboardPackage::get_doc()
{
    return doc;
}
} // namespace horizon
