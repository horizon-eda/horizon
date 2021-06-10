#pragma once
#include "common/common.hpp"
#include "svg_overlay.hpp"
#include "util/uuid.hpp"
#include <array>
#include <gtkmm.h>
#include <set>
#include <package/pad.hpp>

namespace horizon {
class FootprintGeneratorBase : public Gtk::Box {
public:
    FootprintGeneratorBase(const char *resource, class IDocumentPackage &c);
    Glib::PropertyProxy<bool> property_can_generate()
    {
        return p_property_can_generate.get_proxy();
    }

    virtual bool generate() = 0;

protected:
    Glib::Property<bool> p_property_can_generate;
    class PoolBrowserButton *browser_button = nullptr;

    SVGOverlay *overlay = nullptr;
    Gtk::Box *box_top = nullptr;
    class IDocumentPackage &core;
    class Package &package;

    virtual void update_pad_parameters(const Padstack *padstack, Pad &pad, const int64_t pad_width,
                                       const int64_t pad_height);
};
} // namespace horizon
