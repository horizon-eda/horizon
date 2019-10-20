#include "pool-prj-mgr-process.hpp"
#include "util/util.hpp"
#include "pool-mgr/editors/editor_window.hpp"
#include "preferences/preferences_provider.hpp"
#include "preferences/preferences.hpp"

namespace horizon {

template <typename T> constexpr bool any_of(T value, std::initializer_list<T> choices)
{
    return std::count(choices.begin(), choices.end(), value);
}


PoolProjectManagerProcess::PoolProjectManagerProcess(const UUID &uu, PoolProjectManagerProcess::Type ty,
                                                     const std::vector<std::string> &args,
                                                     const std::vector<std::string> &ienv, Pool *pool,
                                                     class PoolParametric *pool_parametric, bool read_only,
                                                     bool is_temp)
    : uuid(uu), type(ty)
{
    if (args.size())
        filename = args.front();
    if (Glib::file_test(filename, Glib::FILE_TEST_IS_REGULAR)) {
        auto info = Gio::File::create_for_path(filename)->query_info();
        mtime = info->modification_time();
    }
    bool is_imp =
            any_of(type, {PoolProjectManagerProcess::Type::IMP_SYMBOL, PoolProjectManagerProcess::Type::IMP_PACKAGE,
                          PoolProjectManagerProcess::Type::IMP_PADSTACK, PoolProjectManagerProcess::Type::IMP_BOARD,
                          PoolProjectManagerProcess::Type::IMP_SCHEMATIC, PoolProjectManagerProcess::Type::IMP_FRAME});
    if (is_imp) { // imp
        std::vector<std::string> argv;
        std::vector<std::string> env = ienv;
        auto envs = Glib::listenv();
        for (const auto &it : envs) {
            env.push_back(it + "=" + Glib::getenv(it));
        }
        auto exe_dir = get_exe_dir();
        auto imp_exe = Glib::build_filename(exe_dir, "horizon-imp");
        argv.push_back(imp_exe);
        switch (type) {
        case PoolProjectManagerProcess::Type::IMP_SYMBOL:
            argv.push_back("-y");
            argv.insert(argv.end(), args.begin(), args.end());
            break;
        case PoolProjectManagerProcess::Type::IMP_PADSTACK:
            argv.push_back("-a");
            argv.insert(argv.end(), args.begin(), args.end());
            break;
        case PoolProjectManagerProcess::Type::IMP_PACKAGE:
            argv.push_back("-k");
            argv.insert(argv.end(), args.begin(), args.end());
            break;
        case PoolProjectManagerProcess::Type::IMP_SCHEMATIC:
            argv.push_back("-c");
            argv.insert(argv.end(), args.begin(), args.end());
            break;
        case PoolProjectManagerProcess::Type::IMP_BOARD:
            argv.push_back("-b");
            argv.insert(argv.end(), args.begin(), args.end());
            break;
        case PoolProjectManagerProcess::Type::IMP_FRAME:
            argv.push_back("-f");
            argv.insert(argv.end(), args.begin(), args.end());
            break;
        default:;
        }
        if (read_only)
            argv.push_back("-r");
        proc = std::make_unique<EditorProcess>(argv, env, PreferencesProvider::get_prefs().capture_output);
        proc->signal_exited().connect([this](auto rc) {
            bool modified = false;
            if (Glib::file_test(filename, Glib::FILE_TEST_IS_REGULAR)) {
                auto info = Gio::File::create_for_path(filename)->query_info();
                auto new_mtime = info->modification_time();
                modified = new_mtime > mtime;
            }
            s_signal_exited.emit(rc, modified);
        });
        proc->signal_output().connect([this](std::string out, bool err) { s_signal_output.emit(out, err); });
    }
    else {
        switch (type) {
        case PoolProjectManagerProcess::Type::UNIT:
            win = new EditorWindow(ObjectType::UNIT, args.at(0), pool, pool_parametric, read_only, is_temp);
            break;
        case PoolProjectManagerProcess::Type::ENTITY:
            win = new EditorWindow(ObjectType::ENTITY, args.at(0), pool, pool_parametric, read_only, is_temp);
            break;
        case PoolProjectManagerProcess::Type::PART:
            win = new EditorWindow(ObjectType::PART, args.at(0), pool, pool_parametric, read_only, is_temp);
            break;
        default:;
        }
        if (args.size() >= 2) {
            win->set_original_filename(args.at(1));
        }
        win->present();
        win->signal_filename_changed().connect([this](std::string new_filename) { filename = new_filename; });
        filename = win->get_filename();

        win->signal_hide().connect([this] {
            auto need_update = win->get_need_update();
            delete win;
            win = nullptr;
            s_signal_exited.emit(0, need_update);
        });
    }
}

void PoolProjectManagerProcess::reload()
{
    if (auto w = dynamic_cast<EditorWindow *>(win)) {
        w->reload();
    }
}

std::string PoolProjectManagerProcess::get_filename()
{
    return filename;
}

} // namespace horizon
