#pragma once
#include "rule_editor.hpp"

namespace horizon {
class RuleEditorLayerPair : public RuleEditor {
    using RuleEditor::RuleEditor;

public:
    void populate() override;

private:
    class RuleLayerPair *rule2;
};
} // namespace horizon
