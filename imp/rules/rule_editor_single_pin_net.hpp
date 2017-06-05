#pragma once
#include "rule_editor.hpp"

namespace horizon {
	class RuleEditorSinglePinNet: public RuleEditor {
		using RuleEditor::RuleEditor;
		public:
			void populate() override;
		private:
			class RuleSinglePinNet *rule2;
	};
}
