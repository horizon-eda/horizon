#pragma once
#include <gtkmm.h>
#include <array>
#include <set>
namespace horizon {

class StepExportWindow : public Gtk::Window {
    friend class GerberLayerEditor;

public:
    StepExportWindow(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x, const class Board *b, class Pool *p);
    static StepExportWindow *create(Gtk::Window *p, const class Board *b, class Pool *po);

    void set_can_export(bool v);

private:
    const class Board *brd;
    class Pool *pool;
    Gtk::HeaderBar *header = nullptr;
    Gtk::Entry *filename_entry = nullptr;
    Gtk::Button *filename_button = nullptr;
    Gtk::Button *export_button = nullptr;
    Gtk::Switch *include_3d_models_switch = nullptr;

    Gtk::TextView *log_textview = nullptr;
    Gtk::Spinner *spinner = nullptr;

    Glib::Dispatcher export_dispatcher;
    std::mutex msg_queue_mutex;
    std::deque<std::string> msg_queue;
    bool export_running = false;


    void handle_export();
    void set_is_busy(bool v);

    void export_thread(std::string filename, bool include_models);
};
} // namespace horizon
