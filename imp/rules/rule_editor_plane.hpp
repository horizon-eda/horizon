#pragma once
#include "rule_editor.hpp"

namespace horizon {
	class RuleEditorPlane: public RuleEditor {
		using RuleEditor::RuleEditor;
		public:
			void populate() override;
		private:
			class RulePlane *rule2;
	};
}
