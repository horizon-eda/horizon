#pragma once
#include <gtkmm.h>
#include "pool/package.hpp"
#include <optional>
#include "util/window_state_store.hpp"
#include "util/paned_state_store.hpp"

namespace horizon {
class ImportKiCadPackageWindow : public Gtk::Window {
public:
    ImportKiCadPackageWindow(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x,
                             class PoolProjectManagerAppWindow &aw);
    static ImportKiCadPackageWindow *create(class PoolProjectManagerAppWindow &aw);

private:
    class PoolProjectManagerAppWindow &appwin;
    Gtk::FileChooserWidget *chooser_widget = nullptr;
    class PreviewCanvas *canvas = nullptr;
    Gtk::Label *package_name_label = nullptr;
    Gtk::Box *import_error_box = nullptr;
    std::optional<Package> package;
    class LogView *log_view = nullptr;
    Gtk::Button *import_button;

    void update();
    void handle_import();

    WindowStateStore window_state_store;
    std::optional<PanedStateStore> paned1_state_store;
    std::optional<PanedStateStore> paned2_state_store;
};
} // namespace horizon
