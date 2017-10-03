#pragma once
#include "rule_editor.hpp"

namespace horizon {
	class RuleEditorClearanceNPTHCopper: public RuleEditor {
		using RuleEditor::RuleEditor;
		public:
			void populate() override;
		private:
			class RuleClearanceNPTHCopper *rule2;
	};
}
