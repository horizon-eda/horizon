#include "clipboard_padstack.hpp"
#include "document/idocument_padstack.hpp"
#include "pool/padstack.hpp"
#include "nlohmann/json.hpp"

namespace horizon {
void ClipboardPadstack::serialize(json &j)
{
    ClipboardBase::serialize(j);
    j["holes"] = json::object();
    j["shapes"] = json::object();
    const auto &ps = doc.get_padstack();
    for (const auto &it : selection) {
        switch (it.type) {
        case ObjectType::SHAPE:
            j["shapes"][(std::string)it.uuid] = ps.shapes.at(it.uuid).serialize();
            break;

        case ObjectType::HOLE:
            j["holes"][(std::string)it.uuid] = ps.holes.at(it.uuid).serialize();
            break;

        default:;
        }
    }
}

IDocument &ClipboardPadstack::get_doc()
{
    return doc;
}
} // namespace horizon
