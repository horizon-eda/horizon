#include "clipboard.hpp"
#include "canvas/canvas_cairo.hpp"
#include "nlohmann/json.hpp"
#include "document/idocument.hpp"

namespace horizon {
ClipboardManager::ClipboardManager(Documents &ds) : buffer(ds), docs(ds)
{
}

void ClipboardManager::copy(std::set<SelectableRef> selection, const Coordi &cp)
{
    std::cout << "copy" << std::endl;
    buffer.load(selection);
    cursor_pos = cp;

    Glib::RefPtr<Gtk::Clipboard> refClipboard = Gtk::Clipboard::get();

    // Targets:
    std::vector<Gtk::TargetEntry> targets;
    targets.push_back(Gtk::TargetEntry("imp-buffer"));
    // targets.push_back( Gtk::TargetEntry("image/png") );
    targets.push_back(Gtk::TargetEntry("image/svg+xml"));

    refClipboard->set(targets, sigc::mem_fun(*this, &ClipboardManager::on_clipboard_get),
                      sigc::mem_fun(*this, &ClipboardManager::on_clipboard_clear));
}

void ClipboardManager::on_clipboard_get(Gtk::SelectionData &selection_data, guint /* info */)
{
    const std::string target = selection_data.get_target();
    std::cout << "get target " << target << std::endl;
    if (target == "imp-buffer") {
        json j = buffer.serialize();
        j["cursor_pos"] = cursor_pos.as_array();
        selection_data.set("imp-buffer", j.dump());
    }
    else if (target == "image/png") {
        auto pb = Gtk::IconTheme::get_default()->load_icon("weather-clear", 32);
        selection_data.set_pixbuf(pb);
    }
    else if (target == "image/svg+xml") {

        std::stringstream stream;
        {
            Cairo::RefPtr<Cairo::SvgSurface> surface = Cairo::SvgSurface::create_for_stream(
                    [&stream](const unsigned char *c, unsigned int n) -> Cairo::ErrorStatus {
                        stream.write(reinterpret_cast<const char *>(c), n);
                        return CAIRO_STATUS_SUCCESS;
                    },
                    10e3, 10e3);

            Cairo::RefPtr<Cairo::Context> cr = Cairo::Context::create(surface);
            cr->scale(1, -1);
            cr->translate(1e3, -1e3);
            CanvasCairo ca(cr);
            ca.update(buffer, docs.r->get_layer_provider());
        }
        selection_data.set("image/svg+xml", stream.str());
    }
}
void ClipboardManager::on_clipboard_clear()
{
    // buffer.clear();
    std::cout << "clipboard clear" << std::endl;
}
} // namespace horizon
