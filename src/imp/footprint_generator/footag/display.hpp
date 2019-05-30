#pragma once
#include "core/core_package.hpp"
#include <gtkmm.h>
extern "C" {
#include "footag/footag.h"
}
namespace horizon {
class FootagDisplay : public Gtk::Box {
public:
    FootagDisplay(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x, CorePackage *c, enum footag_type type);
    static FootagDisplay *create(CorePackage *c, enum footag_type type);

    ~FootagDisplay() override;
    bool generate();
    bool isopen();

private:
    CorePackage *core;
    Gtk::Label *reference_label = nullptr;
    Gtk::Label *reference_title = nullptr;
    Gtk::CheckButton *autofit;
    Gtk::Label *hint_label = nullptr;
    Package ppkg;
    class PreviewCanvas *canvas_package = nullptr;

    struct footag_ctx *ctx = NULL;
    struct footag_param *params;

    void calc_and_display();
    void calc(Package *pkg, const struct footag_spec *s);
    void display();
    void help(const struct footag_param *p);
};
} // namespace horizon
