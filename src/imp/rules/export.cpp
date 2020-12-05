#include "export.hpp"
#include "widgets/location_entry.hpp"
#include "util/gtk_util.hpp"

namespace horizon {


class RuleExportBox : public Gtk::Box {
public:
    RuleExportBox(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x);
    static RuleExportBox *create();

    LocationEntry *location_entry = nullptr;
    Gtk::Entry *name_entry = nullptr;
    Gtk::TextView *notes_view = nullptr;
};

RuleExportBox::RuleExportBox(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x) : Gtk::Box(cobject)
{
    Gtk::Box *file_name_box;
    GET_WIDGET(file_name_box);
    location_entry = Gtk::manage(new LocationEntry(""));
    file_name_box->pack_start(*location_entry, true, true, 0);
    location_entry->show();

    GET_WIDGET(name_entry);
    GET_WIDGET(notes_view);
}

RuleExportBox *RuleExportBox::create()
{
    RuleExportBox *w;
    Glib::RefPtr<Gtk::Builder> x = Gtk::Builder::create();
    x->add_from_resource("/org/horizon-eda/horizon/imp/rules/rule_export.ui");
    x->get_widget_derived("box", w);
    w->reference();
    return w;
}


RuleExportDialog::RuleExportDialog() : Gtk::Dialog("Export rules", Gtk::DIALOG_MODAL | Gtk::DIALOG_USE_HEADER_BAR)
{
    add_button("Cancel", Gtk::RESPONSE_CANCEL);
    add_button("Export", Gtk::RESPONSE_OK);
    set_default_response(Gtk::RESPONSE_OK);
    get_content_area()->set_border_width(0);
    box = RuleExportBox::create();
    get_content_area()->pack_start(*box, true, true, 0);
    box->show();
}

std::string RuleExportDialog::get_filename() const
{
    return box->location_entry->get_filename();
}
RulesExportInfo RuleExportDialog::get_export_info() const
{
    RulesExportInfo info;
    info.name = box->name_entry->get_text();
    info.notes = box->notes_view->get_buffer()->get_text();
    return info;
}

} // namespace horizon