#pragma once
#include "rule_editor.hpp"

namespace horizon {
class RuleEditorViaDefinitions : public RuleEditorWithoutEnable {
    using RuleEditorWithoutEnable::RuleEditorWithoutEnable;

public:
    void populate() override;

private:
    class RuleViaDefinitions *rule2;
    Gtk::Box *via_definitions_box = nullptr;
    Gtk::Box *via_definition_box = nullptr;

    class ViaDefinitionEditor *defintion_editor = nullptr;
    void update_definition_editor(class ViaDefinition &def);

    class ViaDefinitionButton &add_via_definition_button(const class ViaDefinition &def);
    Gtk::RadioButton via_definitions_rb_group;
};
} // namespace horizon
