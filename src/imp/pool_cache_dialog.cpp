#include "pool_cache_dialog.hpp"
#include "widgets/pool_cache_box.hpp"

namespace horizon {


PoolCacheDialog::PoolCacheDialog(Gtk::Window *parent, class IPool &pool, const std::set<std::string> &items_modified)
    : Gtk::Dialog("Update project pool", *parent,
                  Gtk::DialogFlags::DIALOG_MODAL | Gtk::DialogFlags::DIALOG_USE_HEADER_BAR)

{
    add_button("Cancel", Gtk::ResponseType::RESPONSE_CANCEL);
    auto ok_button = add_button("Update checked", Gtk::ResponseType::RESPONSE_OK);
    ok_button->signal_clicked().connect([this] { filenames = cache_box->update_checked(); });
    set_default_response(Gtk::ResponseType::RESPONSE_OK);
    set_default_size(-1, 500);

    cache_box = PoolCacheBox::create_for_imp(pool, items_modified);
    get_content_area()->pack_start(*cache_box, true, true, 0);
    cache_box->show();
}

} // namespace horizon
