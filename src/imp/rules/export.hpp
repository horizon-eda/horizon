#pragma once
#include <gtkmm.h>
#include "rules/rules_import_export.hpp"
#include "rules/rule.hpp"

namespace horizon {
class RuleExportDialog : public Gtk::Dialog {
public:
    RuleExportDialog();

    std::string get_filename() const;
    RulesExportInfo get_export_info() const;

private:
    class RuleExportBox *box = nullptr;
};
} // namespace horizon