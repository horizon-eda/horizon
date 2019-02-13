#pragma once
#include "util/editor_process.hpp"

namespace horizon {
class PoolProjectManagerProcess {
public:
    enum class Type { IMP_SYMBOL, IMP_PADSTACK, IMP_PACKAGE, IMP_SCHEMATIC, IMP_BOARD, IMP_FRAME, UNIT, ENTITY, PART };
    PoolProjectManagerProcess(Type ty, const std::vector<std::string> &args, const std::vector<std::string> &env,
                              class Pool *pool, class PoolParametric *pool_parametric, bool read_only);
    Type type;
    std::unique_ptr<EditorProcess> proc = nullptr;
    class EditorWindow *win = nullptr;
    typedef sigc::signal<void, int, bool> type_signal_exited;
    type_signal_exited signal_exited()
    {
        return s_signal_exited;
    }
    void reload();

private:
    type_signal_exited s_signal_exited;
    Glib::TimeVal mtime;
};
} // namespace horizon
