#pragma once
#include <gtkmm.h>
#include "rules/rule.hpp"

namespace horizon {
class RuleImportDialog : public Gtk::Dialog {
public:
    using Gtk::Dialog::Dialog;
    virtual std::unique_ptr<RuleImportMap> get_import_map() const = 0;

    virtual ~RuleImportDialog()
    {
    }
};

std::unique_ptr<RuleImportDialog> make_rule_import_dialog(const class RulesImportInfo &info, class IDocument &doc);

} // namespace horizon
