#pragma once
#include <gtkmm.h>
#include "common/common.hpp"
#include "util/uuid.hpp"

namespace horizon {
class DuplicateWindow : public Gtk::Window {
public:
    friend class DuplicateUnitWidget;
    friend class DuplicateEntityWidget;
    friend class DuplicatePartWidget;
    DuplicateWindow(class Pool *p, ObjectType ty, const UUID &uu);
    bool get_duplicated() const;

private:
    class Pool *pool = nullptr;
    class DuplicateBase *duplicate_widget = nullptr;
    bool duplicated = false;
    void handle_duplicate();
};
} // namespace horizon
