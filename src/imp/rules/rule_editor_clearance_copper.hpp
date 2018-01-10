#pragma once
#include "rule_editor.hpp"

namespace horizon {
	class RuleEditorClearanceCopper: public RuleEditor {
		using RuleEditor::RuleEditor;
		public:
			void populate() override;
		private:
			class RuleClearanceCopper *rule2;
			void set_some(int row, int col);
			std::map<std::pair<int, int>, class SpinButtonDim*> spin_buttons;
	};
}
