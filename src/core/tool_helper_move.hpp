#pragma once
#include "core.hpp"

namespace horizon {
class ToolHelperMove : public virtual ToolBase {
public:
    ToolHelperMove(class Core *c, ToolID tid) : ToolBase(c, tid)
    {
    }
    static Orientation transform_orientation(Orientation orientation, bool rotate, bool reverse = false);

protected:
    void move_init(const Coordi &c);
    void move_do(const Coordi &delta);
    void move_do_cursor(const Coordi &c);
    void move_mirror_or_rotate(const Coordi &center, bool rotate);

    void cycle_mode();
    std::string mode_to_string();
    Coordi get_coord(const Coordi &c);
    Coordi get_delta() const;

private:
    Coordi last;
    Coordi origin;

    enum class Mode { X, Y, ARB };
    Mode mode = Mode::ARB;
};
} // namespace horizon
