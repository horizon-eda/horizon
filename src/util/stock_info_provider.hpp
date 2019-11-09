#pragma once
#include <gtkmm.h>
#include <memory>
#include <list>
#include "util/uuid.hpp"

namespace horizon {

class StockInfoRecord {
public:
    virtual void append(const StockInfoRecord &other)
    {
    }
    virtual const UUID &get_uuid() const = 0;
    virtual ~StockInfoRecord()
    {
    }
};

class StockInfoProvider {
public:
    virtual void add_columns(Gtk::TreeView *treeview,
                             Gtk::TreeModelColumn<std::shared_ptr<StockInfoRecord>> column) = 0;
    virtual Gtk::Widget *create_status_widget() = 0;
    virtual void update_parts(const std::list<UUID> &parts) = 0;
    virtual std::list<std::shared_ptr<StockInfoRecord>> get_records() = 0;
    virtual ~StockInfoProvider()
    {
    }
    Glib::Dispatcher dispatcher;
};


} // namespace horizon
