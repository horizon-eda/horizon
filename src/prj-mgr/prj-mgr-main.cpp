
#include "prj-mgr-app.hpp"
#include "util/util.hpp"

int main(int argc, char* argv[]) {
	auto application = horizon::ProjectManagerApplication::create();
	horizon::create_config_dir();

	// Start the application, showing the initial window,
	// and opening extra views for any files that it is asked to open,
	// for instance as a command-line parameter.
	// run() will return when the last window has been closed.
	return application->run(argc, argv);
}

