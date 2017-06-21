#pragma once
#include "rule_editor.hpp"

namespace horizon {
	class RuleEditorClearanceSilkscreenExposedCopper: public RuleEditor {
		using RuleEditor::RuleEditor;
		public:
			void populate() override;
		private:
			Gtk::Grid *grid=nullptr;
			int top = 0;
			class SpinButtonDim *create_sp_dim(const std::string &label);
	};
}
