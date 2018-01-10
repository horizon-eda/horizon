#pragma once
#include "rule_editor.hpp"

namespace horizon {
	class RuleEditorPackageChecks: public RuleEditor {
		using RuleEditor::RuleEditor;
		public:
			void populate() override;
		private:
			class RulePackageChecks *rule2;
	};
}
