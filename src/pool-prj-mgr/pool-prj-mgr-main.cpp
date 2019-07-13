#include "pool-prj-mgr-app.hpp"
#include "util/util.hpp"
#include "pool/pool_manager.hpp"
#include "util/exception_util.hpp"

int main(int argc, char *argv[])
{
    gtk_disable_setlocale();
    auto application = horizon::PoolProjectManagerApplication::create();
    horizon::setup_locale();
    horizon::create_config_dir();
    horizon::PoolManager::init();
    horizon::install_signal_exception_handler();

    // Start the application, showing the initial window,
    // and opening extra views for any files that it is asked to open,
    // for instance as a command-line parameter.
    // run() will return when the last window has been closed.
    return application->run(argc, argv);
}
