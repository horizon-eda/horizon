#pragma once

namespace horizon::cli {
// Run a command without starting the GUI and return its exit status to the shell
int run(int argc, char *argv[]);
} // namespace horizon::cli
