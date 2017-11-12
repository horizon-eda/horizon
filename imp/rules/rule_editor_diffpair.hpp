#pragma once
#include "rule_editor.hpp"

namespace horizon {
	class RuleEditorDiffpair: public RuleEditor {
		using RuleEditor::RuleEditor;
		public:
			void populate() override;
		private:
			class RuleDiffpair *rule2;
	};
}
