#include "tool_edit_symbol_pin_names.hpp"
#include "document/idocument_schematic.hpp"
#include "schematic/schematic.hpp"
#include "core/tool_data_window.hpp"
#include "imp/imp_interface.hpp"
#include "dialogs/symbol_pin_names_window.hpp"
#include <iostream>

namespace horizon {

ToolEditSymbolPinNames::ToolEditSymbolPinNames(IDocument *c, ToolID tid) : ToolBase(c, tid), ToolHelperGetSymbol(c, tid)
{
}

bool ToolEditSymbolPinNames::can_begin()
{
    return get_symbol();
}

ToolResponse ToolEditSymbolPinNames::begin(const ToolArgs &args)
{
    sym = get_symbol();
    if (!sym) {
        return ToolResponse::end();
    }

    selection.clear();
    auto &hl = imp->get_highlights();
    hl.clear();
    hl.emplace(ObjectType::SCHEMATIC_SYMBOL, sym->uuid);
    imp->update_highlights();
    win = imp->dialogs.show_symbol_pin_names_window(sym);
    return ToolResponse();
}

ToolResponse ToolEditSymbolPinNames::update(const ToolArgs &args)
{
    if (args.type == ToolEventType::DATA) {
        if (auto data = dynamic_cast<const ToolDataWindow *>(args.data.get())) {
            if (data->event == ToolDataWindow::Event::CLOSE) {
                return ToolResponse::revert();
            }
            else if (data->event == ToolDataWindow::Event::OK) {
                return ToolResponse::commit();
            }
            else if (data->event == ToolDataWindow::Event::UPDATE) {
                auto pin = win->get_selected_pin();
                auto &hl = imp->get_highlights();
                hl.clear();
                if (pin)
                    hl.emplace(ObjectType::SYMBOL_PIN, pin, sym->uuid);
                else
                    hl.emplace(ObjectType::SCHEMATIC_SYMBOL, sym->uuid);
                imp->update_highlights();
                sym->apply_pin_names();
            }
        }
    }
    else if (args.type == ToolEventType::CLICK) {
        if (args.target.type == ObjectType::SYMBOL_PIN) {
            if (args.target.path.at(0) == sym->uuid) {
                win->go_to_pin(args.target.path.at(1));
            }
        }
    }
    return ToolResponse();
}
} // namespace horizon
