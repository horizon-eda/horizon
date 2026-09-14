#include "temp_dir.hpp"
#include <glib.h>
#include <stdexcept>

namespace horizon::cli {
// Create a directory in the system's temporary location for this export
// GLib replaces the XXXXXX suffix so separate exports do not share a directory
TempDir::TempDir()
{
    GError *error = nullptr;
    auto name = g_dir_make_tmp("horizon-export-XXXXXX", &error);
    if (!name) {
        // Keep the message before freeing the GLib error, then pass it on to the CLI
        const std::string message = error->message;
        g_error_free(error);
        throw std::runtime_error(message);
    }
    path = std::filesystem::u8path(name);
    // The path now has its own copy, so we can free the string returned by GLib
    g_free(name);
}

// Remove the directory and its contents when this object goes out of scope
// Use the error_code overload so cleanup does not throw while handling an export failure
TempDir::~TempDir()
{
    std::error_code error;
    std::filesystem::remove_all(path, error);
}
} // namespace horizon::cli
