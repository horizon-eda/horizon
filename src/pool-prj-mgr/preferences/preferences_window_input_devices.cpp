#include "preferences_window_input_devices.hpp"
#include "preferences/preferences.hpp"
#include "util/gtk_util.hpp"
#include "preferences_row.hpp"
#include "util/str_util.hpp"
#include "help_texts.hpp"

namespace horizon {

class PreferencesRowDevice : public PreferencesRow {
public:
    PreferencesRowDevice(Glib::RefPtr<Gdk::Device> dev, Preferences &prefs, Glib::RefPtr<Gtk::SizeGroup> sg);

    void set_current(bool c);
    std::string get_device_name() const;

private:
    Glib::RefPtr<Gdk::Device> device;
};

static std::string get_source_name(Gdk::InputSource src)
{
    static const std::map<Gdk::InputSource, std::string> names = {
            {Gdk::SOURCE_TRACKPOINT, "Trackpoint"}, {Gdk::SOURCE_TOUCHPAD, "Touchpad"}, {Gdk::SOURCE_MOUSE, "Mouse"},
            {Gdk::SOURCE_CURSOR, "Cursor"},         {Gdk::SOURCE_ERASER, "Eraser"},     {Gdk::SOURCE_PEN, "Pen"},
            {Gdk::SOURCE_TABLET_PAD, "Tablet pad"},
    };
    if (names.count(src))
        return names.at(src);
    return "Unknown";
}

PreferencesRowDevice::PreferencesRowDevice(Glib::RefPtr<Gdk::Device> dev, Preferences &prefs,
                                           Glib::RefPtr<Gtk::SizeGroup> sg)
    : PreferencesRow(dev->get_name(), "", prefs), device(dev)
{
    auto box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 10));
    const auto name = dev->get_name();

    auto combo = Gtk::manage(new Gtk::ComboBoxText);
    combo->set_valign(Gtk::ALIGN_CENTER);
    sg->add_widget(*combo);

    using T = InputDevicesPrefs::Device::Type;
    static const std::map<T, std::string> type_names = {{T::TOUCHPAD, "Touchpad"}, {T::TRACKPOINT, "Trackpoint"}};
    combo->append(std::to_string(static_cast<int>(T::AUTO)), "Auto (" + get_source_name(dev->get_source()) + ")");
    combo->append(std::to_string(static_cast<int>(T::TOUCHPAD)), "Touchpad");
    combo->append(std::to_string(static_cast<int>(T::TRACKPOINT)), "Trackpoint");
    if (prefs.input_devices.prefs.devices.count(dev->get_name()))
        combo->set_active_id(
                std::to_string(static_cast<int>(prefs.input_devices.prefs.devices.at(dev->get_name()).type)));
    else
        combo->set_active_id(std::to_string(static_cast<int>(T::AUTO)));

    box->pack_start(*combo, false, false, 0);
    pack_start(*box, false, false, 0);
    box->show_all();


    combo->signal_changed().connect([this, combo, name] {
        auto v = static_cast<T>(std::stoi(combo->get_active_id()));
        preferences.input_devices.prefs.devices[name].type = v;
        preferences.signal_changed().emit();
    });
}


void PreferencesRowDevice::set_current(bool c)
{
    if (c)
        set_subtitle("Current device");
    else
        set_subtitle("");
}

std::string PreferencesRowDevice::get_device_name() const
{
    return device->get_name();
}

class PreferencesRowDeviceType : public PreferencesRow {
public:
    PreferencesRowDeviceType(InputDevicesPrefs::Device::Type type, Preferences &prefs);

private:
    const InputDevicesPrefs::Device::Type type;
};

static const std::map<InputDevicesPrefs::Device::Type, std::string> type_names = {
        {InputDevicesPrefs::Device::Type::MOUSE, "Mouse"},
        {InputDevicesPrefs::Device::Type::TOUCHPAD, "Touchpad"},
        {InputDevicesPrefs::Device::Type::TRACKPOINT, "Trackpoint"},
};

PreferencesRowDeviceType::PreferencesRowDeviceType(InputDevicesPrefs::Device::Type ty, Preferences &prefs)
    : PreferencesRow(type_names.at(ty), "NONE", prefs), type(ty)
{
    auto box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 10));
    {
        auto x = Gtk::manage(new Gtk::CheckButton("Invert zoom"));
        box->pack_start(*x, false, false, 0);
        if (prefs.input_devices.prefs.device_types.count(type))
            x->set_active(prefs.input_devices.prefs.device_types.at(type).invert_zoom);
        x->signal_toggled().connect([this, x] {
            preferences.input_devices.prefs.device_types[type].invert_zoom = x->get_active();
            preferences.signal_changed().emit();
        });
    }
    {
        auto x = Gtk::manage(new Gtk::CheckButton("Invert pan"));
        box->pack_start(*x, false, false, 0);
        if (prefs.input_devices.prefs.device_types.count(type))
            x->set_active(prefs.input_devices.prefs.device_types.at(type).invert_pan);
        x->signal_toggled().connect([this, x] {
            preferences.input_devices.prefs.device_types[type].invert_pan = x->get_active();
            preferences.signal_changed().emit();
        });
    }

    pack_start(*box, false, false, 0);
    box->show_all();
}

InputDevicesPreferencesEditor::InputDevicesPreferencesEditor(Preferences &prefs)
    : Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 20), preferences(prefs)
{
    set_homogeneous(true);
    property_margin() = 20;
    set_halign(Gtk::ALIGN_CENTER);
    auto sgv = Gtk::SizeGroup::create(Gtk::SIZE_GROUP_VERTICAL);
    {
        auto box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL, 10));
        auto gr = Gtk::manage(new PreferencesGroup("Devices"));
        {
            auto la = Gtk::manage(new Gtk::Label("No devices found"));
            la->get_style_context()->add_class("dim-label");
            la->property_margin() = 10;
            sgv->add_widget(*la);
            gr->set_placeholder(*la);
            la->show();
        }

        auto sg = Gtk::SizeGroup::create(Gtk::SIZE_GROUP_HORIZONTAL);

        signal_realize().connect([this, gr, sg, sgv] {
            auto seat = get_window()->get_display()->get_default_seat();
            auto devices = seat->get_slaves(Gdk::SEAT_CAPABILITY_ALL_POINTING);
            for (auto dev : devices) {
                const std::string name = dev->get_name();
                if (dev->get_vendor_id().size() == 0 && dev->get_product_id().size() == 0)
                    continue;
                auto row = Gtk::manage(new PreferencesRowDevice(dev, preferences, sg));
                gr->add_row(*row);
                rows.push_back(row);
                sgv->add_widget(*row);
            }
        });

        add_events(Gdk::POINTER_MOTION_MASK);
        signal_motion_notify_event().connect([this](GdkEventMotion *ev) {
            auto dev = gdk_event_get_source_device((GdkEvent *)ev);
            for (auto row : rows) {
                row->set_current(row->get_device_name() == gdk_device_get_name(dev));
            }
            return false;
        });

        box->pack_start(*gr, false, false, 0);
        gr->show();

        {
            auto la = Gtk::manage(new Gtk::Label(HelpTexts::INPUT_DEVICES));
            la->set_line_wrap(true);
            la->set_max_width_chars(0);
            la->set_xalign(0);
            la->set_margin_start(2);
            box->pack_start(*la, false, false, 0);
            la->show();
        }
        pack_start(*box, true, true, 0);
        box->show();
    }

    {
        auto box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL, 10));
        auto gr = Gtk::manage(new PreferencesGroup("Device Types"));
        using Type = InputDevicesPrefs::Device::Type;
        for (const auto type : {Type::MOUSE, Type::TOUCHPAD, Type::TRACKPOINT}) {
            auto row = Gtk::manage(new PreferencesRowDeviceType(type, preferences));
            sgv->add_widget(*row);
            gr->add_row(*row);
        }

        box->pack_start(*gr, false, false, 0);
        gr->show();

        {
            auto la = Gtk::manage(new Gtk::Label(HelpTexts::INPUT_DEVICE_TYPES));
            la->set_line_wrap(true);
            la->set_max_width_chars(0);
            la->set_xalign(0);
            la->set_margin_start(2);
            box->pack_start(*la, false, false, 0);
            la->show();
        }

        pack_start(*box, true, true, 0);
        box->show();
    }
}
} // namespace horizon
