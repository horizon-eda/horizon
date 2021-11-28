#pragma once
#include "rule_editor.hpp"

namespace horizon {
class RuleEditorShortedPads : public RuleEditor {
    using RuleEditor::RuleEditor;

public:
    void populate() override;

private:
    class RuleShortedPads *rule2;
};
} // namespace horizon
