#include "pool-prj-mgr-app.hpp"
#include "util/util.hpp"
#include "pool/pool_manager.hpp"

int main(int argc, char *argv[])
{
    horizon::setup_locale();
    auto application = horizon::PoolProjectManagerApplication::create();
    horizon::create_config_dir();
    horizon::PoolManager::init();

    // Start the application, showing the initial window,
    // and opening extra views for any files that it is asked to open,
    // for instance as a command-line parameter.
    // run() will return when the last window has been closed.
    return application->run(argc, argv);
}
