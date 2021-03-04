#pragma once
#include <gtkmm.h>
#include <array>
#include <set>
namespace horizon {


class SchematicPropertiesDialog : public Gtk::Dialog {
public:
    SchematicPropertiesDialog(Gtk::Window *parent, class Schematic &c, class IPool &pool, class IPool &pool_caching);


private:
    class Schematic &sch;


    void ok_clicked();
};
} // namespace horizon
