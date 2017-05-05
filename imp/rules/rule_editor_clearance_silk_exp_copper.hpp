#pragma once
#include "rule_editor.hpp"

namespace horizon {
	class RuleEditorClearanceSilkscreenExposedCopper: public RuleEditor {
		using RuleEditor::RuleEditor;
		public:
			void populate() override;
		private:
			class RuleClearanceSilkscreenExposedCopper *rule2;
	};
}
