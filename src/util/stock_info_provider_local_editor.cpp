#include "stock_info_provider_local.hpp"
#include "stock_info_provider_local_editor.hpp"
#include <glibmm.h>

namespace horizon {

StockInfoProviderLocalEditor::StockInfoProviderLocalEditor(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x,
                                                           std::shared_ptr<StockInfoRecordLocal> r)
    : Gtk::Window(cobject), record(r)
{
    stock_adj = Glib::RefPtr<Gtk::Adjustment>::cast_dynamic(x->get_object("stock_adj"));
    price_adj = Glib::RefPtr<Gtk::Adjustment>::cast_dynamic(x->get_object("price_adj"));
    x->get_widget("loc_entry", loc_entry);
    x->get_widget("save_button", save_button);
    x->get_widget("cancel_button", cancel_button);
    x->get_widget("part_label", part_label);
    x->get_widget("last_updated_label", last_updated_label);
    x->get_widget("last_updated_value_label", last_updated_value_label);

    stock_adj->set_value(record->stock);
    price_adj->set_value(record->price);
    loc_entry->set_text(record->location);

    // TODO: Part number??
    part_label->set_text((std::string)record->uuid);

    if (record->last_updated.to_unix() > 0) {
        last_updated_value_label->set_text(record->last_updated.format("%c"));
    }
    else {
        last_updated_value_label->hide();
        last_updated_label->hide();
    }

    save_button->signal_clicked().connect([this] {
        record->stock = stock_adj->get_value();
        record->price = price_adj->get_value();
        record->location = loc_entry->get_text();
        record->save();
        this->close();
    });

    cancel_button->signal_clicked().connect([this] { this->close(); });
}


StockInfoProviderLocalEditor *StockInfoProviderLocalEditor::create(std::shared_ptr<StockInfoRecordLocal> r)
{
    StockInfoProviderLocalEditor *w;
    Glib::RefPtr<Gtk::Builder> x = Gtk::Builder::create();
    x->add_from_resource("/org/horizon-eda/horizon/util/stock_info_provider_local_editor.ui");
    x->get_widget_derived("window", w, r);
    w->reference();
    return w;
}

} // namespace horizon
