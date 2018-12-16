#include "padstack_preview.hpp"
#include "pool/package.hpp"
#include "pool/pool.hpp"
#include "canvas/canvas.hpp"
#include "util/util.hpp"
#include "util/sqlite.hpp"
#include "common/object_descr.hpp"
#include "preview_canvas.hpp"

namespace horizon {
PadstackPreview::PadstackPreview(class Pool &p) : Gtk::Box(Gtk::ORIENTATION_VERTICAL, 0), pool(p)
{

    canvas_padstack = Gtk::manage(new PreviewCanvas(pool, true));

    top_box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 4));
    top_box->property_margin() = 8;
    {
        auto la = Gtk::manage(new Gtk::Label("Package"));
        la->set_margin_start(4);
        la->get_style_context()->add_class("dim-label");
        la->show();
        top_box->pack_start(*la, false, false, 0);

        package_label = Gtk::manage(new Gtk::Label());
        package_label->set_use_markup(true);
        package_label->set_track_visited_links(false);
        package_label->signal_activate_link().connect(
                [this](const std::string &url) {
                    s_signal_goto.emit(ObjectType::PACKAGE, UUID(url));
                    return true;
                },
                false);
        package_label->show();
        top_box->pack_start(*package_label, false, false, 0);
    }


    pack_start(*top_box, false, false, 0);
    top_box->set_no_show_all(true);

    sep = Gtk::manage(new Gtk::Separator(Gtk::ORIENTATION_HORIZONTAL));
    sep->set_no_show_all(true);
    pack_start(*sep, false, false, 0);


    canvas_padstack->show();
    pack_start(*canvas_padstack, true, true, 0);


    load(UUID());
}

void PadstackPreview::load(const UUID &uu)
{
    canvas_padstack->clear();
    package_label->set_text("");
    top_box->hide();
    sep->hide();
    if (!uu) {
        return;
    }
    SQLite::Query q(pool.db, "SELECT package FROM padstacks WHERE uuid = ?");
    q.bind(1, uu);
    if (q.step()) {
        UUID pkg_uuid = q.get<std::string>(0);
        if (pkg_uuid) {
            auto pkg = pool.get_package(pkg_uuid);
            package_label->set_markup("<a href=\"" + (std::string)pkg->uuid + "\">"
                                      + Glib::Markup::escape_text(pkg->name) + "</a>");
            top_box->show();
            sep->show();
        }
    }
    canvas_padstack->load(ObjectType::PADSTACK, uu);
}
} // namespace horizon
