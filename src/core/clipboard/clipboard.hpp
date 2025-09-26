#pragma once
#include "canvas/selectables.hpp"
#include <nlohmann/json_fwd.hpp>

namespace horizon {
using json = nlohmann::json;
class ClipboardBase {
public:
    json process(const std::set<SelectableRef> &sel);

    static std::unique_ptr<ClipboardBase> create(class IDocument &doc);
    virtual ~ClipboardBase()
    {
    }

protected:
    virtual void expand_selection();
    virtual void serialize(json &j);
    virtual json serialize_junction(const class Junction &ju);

    virtual class IDocument &get_doc() = 0;
    std::set<SelectableRef> selection;
};

class ClipboardGeneric : public ClipboardBase {
public:
    ClipboardGeneric(IDocument &d) : doc(d)
    {
    }

protected:
    IDocument &doc;
    IDocument &get_doc() override
    {
        return doc;
    }
};


} // namespace horizon
