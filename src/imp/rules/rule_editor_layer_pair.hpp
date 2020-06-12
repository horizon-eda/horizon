#pragma once
#include "rule_editor.hpp"

namespace horizon {
class RuleEditorLayerPair : public RuleEditor {
    using RuleEditor::RuleEditor;

public:
    void populate() override;

private:
    class RuleLayerPair *rule2;
    Gtk::ComboBoxText &make_layer_combo(int &v);
};
} // namespace horizon
