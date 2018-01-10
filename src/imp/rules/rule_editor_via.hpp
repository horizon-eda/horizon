#pragma once
#include "rule_editor.hpp"

namespace horizon {
	class RuleEditorVia: public RuleEditor {
		using RuleEditor::RuleEditor;
		public:
			void populate() override;
		private:
			class RuleVia *rule2;
	};
}
