#pragma once
#include <gtkmm.h>

namespace horizon {
Glib::RefPtr<Gtk::EntryCompletion> create_pool_manufacturer_completion(class Pool *pool);
}
