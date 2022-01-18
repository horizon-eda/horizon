#include "pin_names_editor.hpp"
#include "util/gtk_util.hpp"

namespace horizon {


class PinNameEditor : public Gtk::Box, public Changeable {
public:
    PinNameEditor(PinNamesEditor::PinNames &p, const UUID &uu)
        : Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 4), names(p), name(names.at(uu)), uuid(uu)
    {
        entry = Gtk::manage(new Gtk::Entry);
        entry->show();
        entry->set_text(name.name);
        entry->signal_changed().connect([this] { s_signal_changed.emit(); });
        entry->signal_activate().connect([this] { s_signal_activate.emit(); });
        entry_add_sanitizer(entry);
        pack_start(*entry, true, true, 0);

        combo = Gtk::manage(new Gtk::ComboBoxText);
        combo->show();
        pack_start(*combo, true, true, 0);
        for (const auto &it : Pin::direction_names) {
            combo->append(std::to_string(static_cast<int>(it.first)), it.second);
        }
        combo->set_active_id(std::to_string(static_cast<int>(name.direction)));
        combo->signal_changed().connect([this] { s_signal_changed.emit(); });

        auto bu = Gtk::manage(new Gtk::Button());
        bu->set_image_from_icon_name("list-remove-symbolic");
        bu->show();
        pack_start(*bu, false, false, 0);
        bu->signal_clicked().connect([this] {
            names.erase(uuid);
            s_signal_changed.emit();
            delete this;
        });
    }
    std::string get_name() const
    {
        return entry->get_text();
    }

    Pin::Direction get_direction() const
    {
        return static_cast<Pin::Direction>(std::stoi(combo->get_active_id()));
    }
    const UUID &get_uuid() const
    {
        return uuid;
    }

    void focus()
    {
        entry->grab_focus();
    }

    type_signal_changed signal_activate()
    {
        return s_signal_activate;
    }

private:
    PinNamesEditor::PinNames &names;
    Pin::AlternateName &name;
    UUID uuid;
    Gtk::Entry *entry = nullptr;
    Gtk::ComboBoxText *combo = nullptr;
    type_signal_changed s_signal_activate;
};

PinNamesEditor::PinNamesEditor(std::map<UUID, Pin::AlternateName> &n) : names(n)
{
    label = Gtk::manage(new Gtk::Label());
    label->set_xalign(0);
    label->set_ellipsize(Pango::ELLIPSIZE_END);
    add(*label);
    label->show();

    popover = Gtk::manage(new Gtk::Popover);
    set_popover(*popover);

    auto outer_box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL, 4));
    outer_box->property_margin() = 4;

    box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL, 4));
    outer_box->pack_start(*box, false, false, 0);


    auto add_button = Gtk::manage(new Gtk::Button);
    add_button->set_image_from_icon_name("list-add-symbolic", Gtk::ICON_SIZE_BUTTON);
    add_button->set_halign(Gtk::ALIGN_END);
    add_button->signal_clicked().connect(sigc::mem_fun(*this, &PinNamesEditor::add_name));

    outer_box->pack_start(*add_button, false, false, 0);

    popover->add(*outer_box);
    outer_box->show_all();

    for (const auto &[uu, name] : names) {
        create_name_editor(uu);
    }

    update_label();
}

void PinNamesEditor::add_name()
{
    auto uu = UUID::random();
    names[uu];
    auto ed = create_name_editor(uu);
    ed->focus();
    s_signal_changed.emit();
    update_label();
}

class PinNameEditor *PinNamesEditor::create_name_editor(const UUID &uu)
{
    auto ed = Gtk::manage(new PinNameEditor(names, uu));
    box->pack_start(*ed, false, false, 0);
    ed->signal_changed().connect([this, ed] {
        if (names.count(ed->get_uuid())) {
            names.at(ed->get_uuid()).name = ed->get_name();
            names.at(ed->get_uuid()).direction = ed->get_direction();
        }
        s_signal_changed.emit();
        update_label();
    });
    ed->signal_activate().connect([this, ed] {
        unsigned int idx = 0;
        auto children = box->get_children();
        for (auto ch : children) {
            if (ch == ed) {
                if (idx == (children.size() - 1))
                    add_name();
                else
                    dynamic_cast<PinNameEditor &>(*children.at(idx + 1)).focus();
                break;
            }
            idx++;
        }
    });
    ed->show();
    return ed;
}

void PinNamesEditor::update_label()
{
    if (names.size()) {
        std::string s;
        for (auto ch : box->get_children()) {
            if (auto ed = dynamic_cast<PinNameEditor *>(ch)) {
                if (s.size())
                    s += ", ";
                s += ed->get_name();
            }
        }
        label->set_text(s);
    }
    else {
        label->set_markup("<i>No alt. names set</i>");
    }
}

void PinNamesEditor::reload()
{
    for (auto ch : box->get_children()) {
        delete ch;
    }
    for (const auto &[uu, alt] : names) {
        create_name_editor(uu);
    }
    update_label();
}

} // namespace horizon
