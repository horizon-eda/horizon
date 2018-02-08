#include "selection_filter_dialog.hpp"

namespace horizon {

SelectionFilterDialog *SelectionFilterDialog::create(Gtk::Window *parent, SelectionFilter *sf, Core *c)
{
    SelectionFilterDialog *dialog;
    Glib::RefPtr<Gtk::Builder> builder = Gtk::Builder::create();
    builder->add_from_resource("/net/carrotIndustries/horizon/imp/selection_filter.ui");
    builder->get_widget_derived("window", dialog, sf, c);
    dialog->set_transient_for(*parent);
    return dialog;
}

SelectionFilterDialog::SelectionFilterDialog(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &builder,
                                             SelectionFilter *sf, Core *c)
    : Gtk::Window(cobject), selection_filter(sf), core(c)
{
    builder->get_widget("listbox", listbox);
    builder->get_widget("select_all", select_all);

    select_all->signal_clicked().connect([this] {
        for (auto cb : checkbuttons) {
            cb->set_active(true);
        }
    });

    for (const auto &it : object_descriptions) {
        auto ot = it.first;
        if (ot == ObjectType::POLYGON)
            continue;
        if (core->has_object_type(ot)) {
            auto row = Gtk::manage(new Gtk::ListBoxRow());
            auto bbox = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 2));

            auto cb = Gtk::manage(new Gtk::CheckButton());
            cb->set_active(true);
            cb->signal_toggled().connect([this, ot, cb] { selection_filter->object_filter[ot] = cb->get_active(); });
            checkbuttons.push_back(cb);
            bbox->pack_start(*cb, false, false, 0);

            auto label = Gtk::manage(new Gtk::Label(it.second.name_pl));
            bbox->pack_start(*label, false, false, 0);

            row->add(*bbox);
            listbox->add(*row);
            row->show_all();
        }
    }

    listbox->signal_row_activated().connect([this](Gtk::ListBoxRow *row) {
        auto box = dynamic_cast<Gtk::Box *>(row->get_child());
        auto current_cb = dynamic_cast<Gtk::CheckButton *>(box->get_children()[0]);

        for (auto cb_other : checkbuttons) {
            cb_other->set_active(cb_other == current_cb);
        }
    });
}
} // namespace horizon
