#pragma once
#include "rule_editor.hpp"

namespace horizon {
class RuleEditorThermals : public RuleEditor {
    using RuleEditor::RuleEditor;

public:
    void populate() override;

private:
    class RuleThermals *rule2;
    std::set<Gtk::Widget *> widgets_thermal_only;
    void update_thermal();

    class MatchPadsEditor *match_pads_editor = nullptr;

    void update_pads();
};
} // namespace horizon
