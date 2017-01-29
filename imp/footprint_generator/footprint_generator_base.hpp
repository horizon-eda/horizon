#pragma once
#include <gtkmm.h>
#include <array>
#include <set>
#include "common.hpp"
#include "uuid.hpp"
#include "core/core_package.hpp"
#include "widgets/chooser_buttons.hpp"
#include "svg_overlay.hpp"
namespace horizon {
	class FootprintGeneratorBase: public Gtk::Box {
		public:
			FootprintGeneratorBase(const char *resource, CorePackage *c);
			Glib::PropertyProxy<bool> property_can_generate() { return p_property_can_generate.get_proxy(); }

			virtual bool generate() = 0;

		protected :
			Glib::Property<bool> p_property_can_generate;
			PadstackButton *padstack_button = nullptr;

			SVGOverlay *overlay = nullptr;
			Gtk::Box *box_top = nullptr;
			CorePackage *core;
	};

}
