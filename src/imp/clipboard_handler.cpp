#include "clipboard_handler.hpp"
#include <nlohmann/json.hpp>
#include "core/clipboard/clipboard.hpp"
namespace horizon {
using json = nlohmann::json;

ClipboardHandler::ClipboardHandler(ClipboardBase &cl) : clip(cl)
{
}


void ClipboardHandler::copy(std::set<SelectableRef> selection, const Coordi &cp)
{
    auto j = clip.process(selection);
    j["cursor_pos"] = cp.as_array();
    clipboard_serialized = j.dump();

    Glib::RefPtr<Gtk::Clipboard> refClipboard = Gtk::Clipboard::get();

    // Targets:
    std::vector<Gtk::TargetEntry> targets;
    targets.push_back(Gtk::TargetEntry("imp-buffer"));

    refClipboard->set(targets, sigc::mem_fun(*this, &ClipboardHandler::on_clipboard_get),
                      sigc::mem_fun(*this, &ClipboardHandler::on_clipboard_clear));
}

void ClipboardHandler::on_clipboard_get(Gtk::SelectionData &selection_data, guint /* info */)
{
    const std::string target = selection_data.get_target();
    if (target == "imp-buffer") {
        selection_data.set("imp-buffer", clipboard_serialized);
    }
}
void ClipboardHandler::on_clipboard_clear()
{
}
} // namespace horizon
