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

std::optional<int> run(int argc, char *argv[])
{
    if (argc < 2)
        return {};
    const std::string command = argv[1];
    if (command == "--version") {
        if (argc != 2) {
            std::cerr << "horizon-eda: --version does not take arguments\n";
            return 2;
        }
        std::cout << "horizon-eda " << Version::get_string() << "\n";
        return 0;
    }
    if (command != "export")
        return {};

    Options options;
    try {
        options = parse_options(std::vector<std::string>(argv + 2, argv + argc));
    }
    catch (const std::exception &e) {
        std::cerr << "horizon-eda: " << e.what() << "\nUse horizon-eda export --help for usage\n";
        return 2;
    }
    if (options.help) {
        std::cout << export_help(argc > 2 && argv[2][0] != '-' ? argv[2] : "");
        return 0;
    }

    ExportLog log(options.quiet);
    try {
        Gio::init();
        setup_locale();
        PoolManager::init(false);
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
        std::cerr << "horizon-eda: " << e.what() << "\n";
        return 2;
    }
    catch (const PoDoFo::PdfError &e) {
        std::cerr << "horizon-eda: PDF export failed: " << e.what() << "\n";
    }
    catch (const std::exception &e) {
        std::cerr << "horizon-eda: " << e.what() << "\n";
    }
    catch (const Glib::Error &e) {
        std::cerr << "horizon-eda: " << e.what() << "\n";
    }
    catch (...) {
        std::cerr << "horizon-eda: export failed with an unknown error\n";
    }
    return 1;
}
} // namespace horizon::cli
