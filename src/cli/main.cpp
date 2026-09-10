#include "cli.hpp"
#include <glibmm/init.h>
#include <glibmm/miscutils.h>

// The CLI has its own entry point, so exports do not need a GTK application or a display
int main(int argc, char *argv[])
{
    Glib::init();
    Glib::set_prgname("horizon-cli");
    return horizon::cli::run(argc, argv);
}
