#include "imp.hpp"
#include "imp_board.hpp"
#include "imp_package.hpp"
#include "imp_padstack.hpp"
#include "imp_schematic.hpp"
#include "imp_symbol.hpp"
#include "imp_frame.hpp"
#include "imp_decal.hpp"
#include "pool/part.hpp"
#include "pool/pool.hpp"
#include "pool/symbol.hpp"
#include "pool/unit.hpp"
#include "util/util.hpp"
#include "util/uuid.hpp"
#include "pool/pool_manager.hpp"
#include "util/exception_util.hpp"
#include "util/automatic_prefs.hpp"
#include <curl/curl.h>
#include <fstream>
#include <iostream>
#include <memory>
#ifdef G_OS_WIN32
#include <windows.h>
#endif

using json = nlohmann::json;

static void expect_filename_arg(const std::vector<std::string> &filenames, const char *name, size_t i)
{
    if (filenames.size() <= i || !Glib::file_test(filenames.at(i), Glib::FILE_TEST_EXISTS)) {
        g_warning("%s filename argument at %i missing or does not exist", name, static_cast<int>(i));
    }
}

static void expect_dir_arg(const std::vector<std::string> &filenames, const char *name, size_t i)
{
    if (filenames.size() <= i || filenames.at(i).size() == 0) {
        g_warning("%s directory argument at %i missing", name, static_cast<int>(i));
    }
}

static void expect_pool_env(const std::string &pool_base_path)
{
    if (pool_base_path.length() <= 0) {
        g_warning("HORIZON_POOL dir env var is not set");
    }
    else if (!Glib::file_test(pool_base_path, Glib::FILE_TEST_EXISTS)) {
        g_warning("HORIZON_POOL dir does not exist");
    }
    else if (!Glib::file_test(Glib::build_filename(pool_base_path, "pool.json"), Glib::FILE_TEST_EXISTS)) {
        g_warning("HORIZON_POOL dir does not contain pool.json");
    }
}

int main(int argc, char *argv[])
{
#ifdef G_OS_WIN32
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX | SEM_NOOPENFILEERRORBOX);
#endif

    gtk_disable_setlocale();
    Gio::init();
    horizon::PoolManager::init();
    horizon::AutomaticPreferences::get();

    Glib::OptionContext options;
    options.set_summary("horizon interactive manipulator");
    options.set_description("The 'HORIZON_POOL' env var should be set directory of the pool being used.");
    options.set_help_enabled();

    Glib::OptionGroup group("imp", "imp");

    bool mode_symbol = false;
    Glib::OptionEntry entry;
    entry.set_long_name("symbol");
    entry.set_short_name('y');
    entry.set_description("Symbol mode");
    group.add_entry(entry, mode_symbol);

    bool mode_sch = false;
    Glib::OptionEntry entry2;
    entry2.set_long_name("schematic");
    entry2.set_short_name('c');
    entry2.set_description("Schematic mode");
    group.add_entry(entry2, mode_sch);

    bool mode_padstack = false;
    Glib::OptionEntry entry3;
    entry3.set_long_name("padstack");
    entry3.set_short_name('a');
    entry3.set_description("Padstack mode");
    group.add_entry(entry3, mode_padstack);

    bool mode_package = false;
    Glib::OptionEntry entry4;
    entry4.set_long_name("package");
    entry4.set_short_name('k');
    entry4.set_description("Package mode");
    group.add_entry(entry4, mode_package);

    bool mode_board = false;
    Glib::OptionEntry entry5;
    entry5.set_long_name("board");
    entry5.set_short_name('b');
    entry5.set_description("Board mode");
    group.add_entry(entry5, mode_board);

    bool mode_frame = false;
    Glib::OptionEntry entry7;
    entry7.set_long_name("frame");
    entry7.set_short_name('f');
    entry7.set_description("Frame mode");
    group.add_entry(entry7, mode_frame);

    bool mode_decal = false;
    Glib::OptionEntry entry8;
    entry8.set_long_name("decal");
    entry8.set_short_name('d');
    entry8.set_description("Decal mode");
    group.add_entry(entry8, mode_decal);

    bool read_only = false;
    Glib::OptionEntry entry6;
    entry6.set_long_name("read-only");
    entry6.set_short_name('r');
    entry6.set_description("Read only");
    group.add_entry(entry6, read_only);

    bool temp_mode_bool = false;
    Glib::OptionEntry entry9;
    entry9.set_long_name("temp");
    entry9.set_short_name('t');
    entry9.set_description("Temporary file mode");
    group.add_entry(entry9, temp_mode_bool);

    std::vector<std::string> filenames;
    Glib::OptionEntry entry_f;
    entry_f.set_long_name(G_OPTION_REMAINING);
    entry_f.set_short_name(' ');
    entry_f.set_description("Filename");
    group.add_entry_filename(entry_f, filenames);

    options.set_main_group(group);
    options.parse(argc, argv);

    horizon::ImpBase::TempMode temp_mode = horizon::ImpBase::TempMode::NO;
    if (temp_mode_bool)
        temp_mode = horizon::ImpBase::TempMode::YES;

    auto pool_base_path = Glib::getenv("HORIZON_POOL");
    horizon::setup_locale();

    horizon::create_cache_and_config_dir();
    std::unique_ptr<horizon::ImpBase> imp = nullptr;
    if (mode_sch) {
        expect_filename_arg(filenames, "blocks", 0);
        expect_dir_arg(filenames, "pictures", 1);
        expect_pool_env(pool_base_path);
        imp.reset(new horizon::ImpSchematic(filenames, {pool_base_path}));
    }
    else if (mode_symbol) {
        expect_filename_arg(filenames, "symbol", 0);
        expect_pool_env(pool_base_path);
        imp.reset(new horizon::ImpSymbol(filenames.at(0), pool_base_path, temp_mode));
        if (filenames.size() > 1)
            imp->set_suggested_filename(filenames.at(1));
    }
    else if (mode_padstack) {
        expect_filename_arg(filenames, "padstack", 0);
        expect_pool_env(pool_base_path);
        imp.reset(new horizon::ImpPadstack(filenames.at(0), pool_base_path, temp_mode));
        if (filenames.size() > 1)
            imp->set_suggested_filename(filenames.at(1));
    }
    else if (mode_package) {
        expect_filename_arg(filenames, "package", 0);
        expect_pool_env(pool_base_path);
        imp.reset(new horizon::ImpPackage(filenames.at(0), pool_base_path, temp_mode));
        if (filenames.size() > 1)
            imp->set_suggested_filename(filenames.at(1));
    }
    else if (mode_board) {
        expect_filename_arg(filenames, "board", 0);
        expect_filename_arg(filenames, "planes", 1);
        expect_filename_arg(filenames, "blocks", 2);
        expect_dir_arg(filenames, "pictures", 3);
        expect_pool_env(pool_base_path);
        imp.reset(new horizon::ImpBoard(filenames, {pool_base_path}));
    }
    else if (mode_frame) {
        expect_filename_arg(filenames, "frame", 0);
        expect_pool_env(pool_base_path);
        imp.reset(new horizon::ImpFrame(filenames.at(0), pool_base_path, temp_mode));
        if (filenames.size() > 1)
            imp->set_suggested_filename(filenames.at(1));
    }
    else if (mode_decal) {
        expect_filename_arg(filenames, "decal", 0);
        expect_pool_env(pool_base_path);
        imp.reset(new horizon::ImpDecal(filenames.at(0), pool_base_path, temp_mode));
        if (filenames.size() > 1)
            imp->set_suggested_filename(filenames.at(1));
    }
    else {
        std::cout << "wrong invocation" << std::endl;
        std::cout << options.get_help().c_str();
        return 1;
    }
    imp->set_read_only(read_only);

    horizon::install_signal_exception_handler();
    imp->run(argc, argv);

    return 0;
}
