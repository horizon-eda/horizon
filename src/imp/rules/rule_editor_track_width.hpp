#pragma once
#include "rule_editor.hpp"

namespace horizon {
	class RuleEditorTrackWidth: public RuleEditor {
		using RuleEditor::RuleEditor;
		public:
			void populate() override;
		private:
			class RuleTrackWidth *rule2;
	};
}
