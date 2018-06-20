#include "pool-prj-mgr-process.hpp"
#include "util/util.hpp"
#include "pool-mgr/editors/editor_window.hpp"

namespace horizon {

template <typename T> constexpr bool any_of(T value, std::initializer_list<T> choices)
{
    return std::count(choices.begin(), choices.end(), value);
}


PoolProjectManagerProcess::PoolProjectManagerProcess(PoolProjectManagerProcess::Type ty,
                                                     const std::vector<std::string> &args,
                                                     const std::vector<std::string> &ienv, Pool *pool)
    : type(ty)
{
    std::string filename;
    if (args.size())
        filename = args.front();
    if (Glib::file_test(filename, Glib::FILE_TEST_IS_REGULAR)) {
        auto info = Gio::File::create_for_path(filename)->query_info();
        mtime = info->modification_time();
    }
    bool is_imp =
            any_of(type, {PoolProjectManagerProcess::Type::IMP_SYMBOL, PoolProjectManagerProcess::Type::IMP_PACKAGE,
                          PoolProjectManagerProcess::Type::IMP_PADSTACK, PoolProjectManagerProcess::Type::IMP_BOARD,
                          PoolProjectManagerProcess::Type::IMP_SCHEMATIC});
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
        default:;
        }
        proc = std::make_unique<EditorProcess>(argv, env);
        proc->signal_exited().connect([this, filename](auto rc) {
            bool modified = false;
            if (Glib::file_test(filename, Glib::FILE_TEST_IS_REGULAR)) {
                auto info = Gio::File::create_for_path(filename)->query_info();
                auto new_mtime = info->modification_time();
                modified = new_mtime > mtime;
            }
            s_signal_exited.emit(rc, modified);
        });
    }
    else {
        switch (type) {
        case PoolProjectManagerProcess::Type::UNIT:
            win = new EditorWindow(ObjectType::UNIT, args.at(0), pool);
            break;
        case PoolProjectManagerProcess::Type::ENTITY:
            win = new EditorWindow(ObjectType::ENTITY, args.at(0), pool);
            break;
        case PoolProjectManagerProcess::Type::PART:
            win = new EditorWindow(ObjectType::PART, args.at(0), pool);
            break;
        default:;
        }
        win->present();

        win->signal_hide().connect([this] {
            auto need_update = win->get_need_update();
            delete win;
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

} // namespace horizon
