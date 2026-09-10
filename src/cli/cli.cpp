#include "cli.hpp"
#include "options.hpp"
#include "export.hpp"
#include "logger/logger.hpp"
#include "pool/pool_manager.hpp"
#include "util/util.hpp"
#include "util/version.hpp"
#include "util/podofo_inc.hpp"
#include <giomm/init.h>
#include <glibmm/error.h>
#include <glibmm/optioncontext.h>
#include <glibmm/optiongroup.h>
#include <iostream>
#include <memory>
#include <atomic>

namespace horizon::cli {
class ExportLog {
public:
    // Some existing exporters write progress to stdout, so redirect that to stderr for the CLI
    // With --quiet we discard it instead
    explicit ExportLog(bool quiet)
    {
        previous = std::cout.rdbuf(quiet ? &discard : std::cerr.rdbuf());
    }
    // Put stdout back when we are done handling the command
    ~ExportLog()
    {
        std::cout.rdbuf(previous);
    }

private:
    class Discard : public std::streambuf {
        // Report success when discarding a character, so --quiet does not make the stream appear broken
        int_type overflow(int_type ch) override
        {
            return traits_type::not_eof(ch);
        }
    } discard;
    std::streambuf *previous;
};

// Parse the command, initialize the shared project services and return an exit status for automation
int run(int argc, char *argv[])
{
    Options options;
    try {
        // Stop at the command name so the exporter can parse its own options with a separate GLib group
        Glib::OptionContext context("COMMAND [ARGS...]");
        context.set_help_enabled(false);
        context.set_strict_posix();
        context.set_summary("Command-line tools for Horizon-EDA");
        context.set_description(
                "Commands:\n"
                "  export         Export project documentation, fabrication and assembly files\n\n"
                "Run 'horizon-cli COMMAND --help' for command options");
        Glib::OptionGroup group("cli", "General options");
        bool version = false;
        bool help = false;
        Glib::OptionEntry version_entry;
        version_entry.set_long_name("version");
        version_entry.set_description("Show application version");
        group.add_entry(version_entry, version);
        Glib::OptionEntry help_entry;
        help_entry.set_long_name("help");
        help_entry.set_short_name('h');
        help_entry.set_description("Show this help");
        group.add_entry(help_entry, help);
        context.set_main_group(group);
        context.parse(argc, argv);
        if (version) {
            if (argc != 1)
                throw UsageError("--version does not take arguments");
            std::cout << "horizon-cli " << Version::get_string() << "\n";
            return 0;
        }
        if (help || argc == 1) {
            std::cout << context.get_help().raw();
            return 0;
        }
        const std::string command = argv[1];
        if (command != "export")
            throw UsageError("unknown command: " + command);
        options = parse_options(std::vector<std::string>(argv + 2, argv + argc));
    }
    catch (const Glib::Error &e) {
        std::cerr << "horizon-cli: " << e.what() << "\nUse horizon-cli --help for usage\n";
        return 2;
    }
    catch (const std::exception &e) {
        std::cerr << "horizon-cli: " << e.what() << "\nUse horizon-cli --help for usage\n";
        return 2;
    }
    if (options.help) {
        std::cout << options.help_text;
        return 0;
    }

    ExportLog log(options.quiet);
    try {
        Gio::init();
        setup_locale();
        create_cache_and_config_dir();
        PoolManager::init();
        auto load_warning = std::make_shared<std::atomic<bool>>(false);
        Logger::get().set_log_handler([quiet = options.quiet, load_warning](const Logger::Item &item) {
            const bool warning = item.level == Logger::Level::WARNING || item.level == Logger::Level::CRITICAL;
            if (warning)
                *load_warning = true;
            if (warning || (!quiet && item.level == Logger::Level::INFO)) {
                std::cerr << Logger::level_to_string(item.level) << ": " << item.message;
                if (!item.detail.empty())
                    std::cerr << ": " << item.detail;
                std::cerr << "\n";
            }
        });
        if (!options.quiet)
            std::cerr << "Loading " << options.project << "\n";
        run_export(options, [load_warning] {
            if (*load_warning)
                throw std::runtime_error("project loading reported warnings, refusing to export incomplete data");
        });
        if (!options.quiet)
            std::cerr << "Exported to " << options.output << "\n";
        return 0;
    }
    catch (const UsageError &e) {
        std::cerr << "horizon-cli: " << e.what() << "\n";
        return 2;
    }
    catch (const PoDoFo::PdfError &e) {
        std::cerr << "horizon-cli: PDF export failed: " << e.what() << "\n";
    }
    catch (const std::exception &e) {
        std::cerr << "horizon-cli: " << e.what() << "\n";
    }
    catch (const Glib::Error &e) {
        std::cerr << "horizon-cli: " << e.what() << "\n";
    }
    catch (...) {
        std::cerr << "horizon-cli: export failed with an unknown error\n";
    }
    return 1;
}
} // namespace horizon::cli
