#include "project_properties.hpp"
#include "block/block.hpp"
#include "widgets/project_meta_editor.hpp"

namespace horizon {

ProjectPropertiesDialog::ProjectPropertiesDialog(Gtk::Window *parent, Block &block)
    : Gtk::Dialog("Project properties", *parent,
                  Gtk::DialogFlags::DIALOG_MODAL | Gtk::DialogFlags::DIALOG_USE_HEADER_BAR)
{
    add_button("Cancel", Gtk::ResponseType::RESPONSE_CANCEL);
    add_button("OK", Gtk::ResponseType::RESPONSE_OK);
    set_default_response(Gtk::ResponseType::RESPONSE_OK);
    set_default_size(400, 300);

    auto ed = Gtk::manage(new ProjectMetaEditor(block.project_meta));
    ed->property_margin() = 20;

    get_content_area()->pack_start(*ed, true, true, 0);
    get_content_area()->set_border_width(0);

    show_all();
}

} // namespace horizon
