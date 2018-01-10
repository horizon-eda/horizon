#pragma once
#include "rule_editor.hpp"

namespace horizon {
	class RuleEditorHoleSize: public RuleEditor {
		using RuleEditor::RuleEditor;
		public:
			void populate() override;
		private:
			class RuleHoleSize *rule2;
	};
}
