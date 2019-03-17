#include "exception_util.hpp"
#include <gtk/gtk.h>
#include <glibmm.h>
#include <string.h>

namespace horizon {

static void exception_handler(void)
{
    char *ex = nullptr;
    const char *ex_type = "unknown exception";
    try {
        throw;
    }
    catch (const Glib::Error &e) {
        ex = strdup(e.what().c_str());
        ex_type = "Glib::Error";
    }
    catch (const std::exception &e) {
        ex = strdup(e.what());
        ex_type = "std::exception";
    }
    catch (...) {
    }
    auto dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_NONE,
                                         "Exception in signal handler");
    gtk_dialog_add_buttons(GTK_DIALOG(dialog), "Exit", 1, "Ignore", 2, NULL);
    gtk_message_dialog_format_secondary_text(
            GTK_MESSAGE_DIALOG(dialog),
            "%s: %s\nLooks like this is a bug. Please report this and describe what you did previously.", ex_type, ex);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == 1) {
        exit(1);
    }
    gtk_widget_destroy(dialog);
    free(ex);
}

void install_signal_exception_handler()
{
    Glib::add_exception_handler(&exception_handler);
}

} // namespace horizon
