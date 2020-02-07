#pragma once
#include "core/tool.hpp"
#include "tool_helper_restrict.hpp"

namespace horizon {
class ToolHelperMove : public virtual ToolBase, public ToolHelperRestrict {
public:
    ToolHelperMove(class IDocument *c, ToolID tid) : ToolBase(c, tid)
    {
    }
    static Orientation transform_orientation(Orientation orientation, bool rotate, bool reverse = false);

protected:
    void move_init(const Coordi &c);
    void move_do(const Coordi &delta);
    void move_do_cursor(const Coordi &c);
    void move_mirror_or_rotate(const Coordi &center, bool rotate);

    Coordi get_delta() const;

private:
    Coordi last;
    Coordi origin;
};
} // namespace horizon
