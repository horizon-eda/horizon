#pragma once
#include "rule_editor.hpp"

namespace horizon {
class RuleEditorConnectivity : public RuleEditor {
    using RuleEditor::RuleEditor;

public:
    void populate() override;

private:
    class RuleConnectivity *rule2;
};
} // namespace horizon
