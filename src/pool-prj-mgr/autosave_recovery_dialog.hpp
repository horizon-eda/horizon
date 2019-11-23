#pragma once
#include <gtkmm.h>

namespace horizon {
class AutosaveRecoveryDialog : public Gtk::MessageDialog {
public:
    AutosaveRecoveryDialog(Gtk::Window *parent);

    enum class Result { USE, KEEP, DELETE };

    Result get_result() const
    {
        return result;
    }

private:
    Result result = Result::KEEP;
};
} // namespace horizon
