#pragma once

namespace horizon {
class PoolEditorInterface {
public:
    virtual void reload()
    {
    }
    virtual void save()
    {
        needs_save = false;
    }
    bool get_needs_save()
    {
        return needs_save;
    }

protected:
    bool needs_save = false;
};
} // namespace horizon
