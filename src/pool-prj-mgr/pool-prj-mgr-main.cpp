#include "pool-prj-mgr-app.hpp"
#include "util/util.hpp"
#include "pool/pool_manager.hpp"
#include "util/exception_util.hpp"
#ifdef G_OS_WIN32
#include <windows.h>
#endif
#include "util/automatic_prefs.hpp"
#include "util/stock_info_provider.hpp"
#include "util/installation_uuid.hpp"

int main(int argc, char *argv[])
{
#ifdef __OpenBSD__
    extern char *argv0;
    argv0 = argv[0];
#endif

#ifdef G_OS_WIN32
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX | SEM_NOOPENFILEERRORBOX);
#endif

    gtk_disable_setlocale();
    auto application = horizon::PoolProjectManagerApplication::create();
    horizon::setup_locale();
    horizon::create_config_dir();
    horizon::PoolManager::init();
    horizon::AutomaticPreferences::get();
    horizon::StockInfoProvider::init_db();
    horizon::install_signal_exception_handler();
    horizon::InstallationUUID::get();

    // Start the application, showing the initial window,
    // and opening extra views for any files that it is asked to open,
    // for instance as a command-line parameter.
    // run() will return when the last window has been closed.
    return application->run(argc, argv);
}
