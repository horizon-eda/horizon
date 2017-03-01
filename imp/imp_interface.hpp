#pragma once
#include "dialogs/dialogs.hpp"


namespace horizon {
	class ImpInterface {
		public:
			ImpInterface(class ImpBase *i);
			Dialogs dialogs;
			void tool_bar_set_tip(const std::string &s);
			void tool_bar_flash(const std::string &s);

		private:
			ImpBase *imp;
	};
}
