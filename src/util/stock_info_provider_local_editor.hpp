#pragma once
#include <gtkmm.h>
#include "common/common.hpp"

namespace horizon {

class StockInfoProviderLocal;
class StockInfoRecordLocal;

class StockInfoProviderLocalEditor : public Gtk::Window {

public:
    StockInfoProviderLocalEditor(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x,
                                 std::shared_ptr<class StockInfoRecordLocal> r);
    static StockInfoProviderLocalEditor *create(std::shared_ptr<class StockInfoRecordLocal> r);

    virtual ~StockInfoProviderLocalEditor(){};

private:
    std::shared_ptr<class StockInfoRecordLocal> record;

    Glib::RefPtr<Gtk::Adjustment> stock_adj;
    Glib::RefPtr<Gtk::Adjustment> price_adj;
    Gtk::Entry *loc_entry = nullptr;
    Gtk::Label *part_label = nullptr;
    Gtk::Label *last_updated_label = nullptr;
    Gtk::Label *last_updated_value_label = nullptr;

    Gtk::Button *save_button = nullptr;
    Gtk::Button *cancel_button = nullptr;
};
} // namespace horizon
