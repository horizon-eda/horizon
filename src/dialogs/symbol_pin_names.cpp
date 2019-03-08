#include "symbol_pin_names.hpp"
#include "pool/gate.hpp"
#include "pool/entity.hpp"
#include "schematic/schematic_symbol.hpp"
#include "util/util.hpp"
#include "util/str_util.hpp"
#include "util/gtk_util.hpp"
#include "common/object_descr.hpp"
#include <iostream>
#include <deque>
#include <algorithm>

namespace horizon {

static void append_to_label(std::string &label, const std::string &x)
{
    if (label.size())
        label += ", ";
    label += x;
}


class PinNamesButton : public Gtk::MenuButton {
public:
    PinNamesButton(Component *c, const UUIDPath<2> &p)
        : Gtk::MenuButton(), comp(c), path(p), pin(comp->entity->gates.at(path.at(0)).unit->pins.at(path.at(1)))
    {
        auto popover = Gtk::manage(new Gtk::Popover);
        auto pbox = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL, 4));
        pbox->set_homogeneous(true);

        auto primary_button = Gtk::manage(new Gtk::CheckButton(pin.primary_name + " (primary)"));
        if (comp->pin_names.count(path))
            primary_button->set_active(comp->pin_names.at(path).count(-1));
        primary_button->signal_toggled().connect([this, primary_button] {
            add_remove_name(-1, primary_button->get_active());
            update_label();
        });
        pbox->pack_start(*primary_button, false, false, 0);

        {
            int i = 0;
            for (const auto &pin_name : pin.names) {
                auto cb = Gtk::manage(new Gtk::CheckButton(pin_name));
                if (comp->pin_names.count(path))
                    cb->set_active(comp->pin_names.at(path).count(i));
                cb->signal_toggled().connect([this, i, cb] {
                    add_remove_name(i, cb->get_active());
                    update_label();
                });
                i++;
                pbox->pack_start(*cb, false, false, 0);
            }
        }

        {
            auto cbox = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL));
            auto cb = Gtk::manage(new Gtk::CheckButton(""));
            cbox->pack_start(*cb, false, false, 0);
            auto entry = Gtk::manage(new Gtk::Entry);
            if (comp->custom_pin_names.count(path))
                entry->set_text(comp->custom_pin_names.at(path));
            cbox->pack_start(*entry, true, true, 0);
            cb->signal_toggled().connect([this, cb, entry] {
                entry->set_sensitive(cb->get_active());
                add_remove_name(-2, cb->get_active());
                update_label();
            });
            entry->set_sensitive(false);
            entry->set_placeholder_text("Custom");
            entry->signal_changed().connect([this, entry] {
                comp->custom_pin_names[path] = entry->get_text();
                update_label();
            });
            if (comp->pin_names.count(path))
                cb->set_active(comp->pin_names.at(path).count(-2));

            pbox->pack_start(*cbox, false, false, 0);
        }

        popover->add(*pbox);
        pbox->show_all();

        update_label();
        set_popover(*popover);
    }

private:
    Component *comp;
    UUIDPath<2> path;
    const Pin &pin;

    void add_remove_name(int name, bool add)
    {
        if (add) {
            comp->pin_names[path].insert(name);
        }
        else {
            comp->pin_names[path].erase(name);
        }
    }

    void update_label()
    {
        std::string label;
        if (comp->pin_names.count(path)) {
            const auto &names = comp->pin_names.at(path);
            if (names.size() == 0) {
                label = pin.primary_name + " (default)";
            }
            else {
                for (const auto &it : names) {
                    if (it == -2) {
                        // nop, see later
                    }
                    else if (it == -1) {
                        append_to_label(label, pin.primary_name);
                    }
                    else {
                        if (it >= 0 && it < ((int)pin.names.size()))
                            append_to_label(label, pin.names.at(it));
                    }
                }
                if (names.count(-2) && comp->custom_pin_names.count(path)) {
                    append_to_label(label, comp->custom_pin_names.at(path));
                }
            }
        }
        else {
            label = pin.primary_name + " (default)";
        }
        set_label(label);
    }
};


class GatePinEditor : public Gtk::ListBox {
public:
    GatePinEditor(Component *c, const Gate *g) : Gtk::ListBox(), comp(c), gate(g)
    {
        sg = Gtk::SizeGroup::create(Gtk::SIZE_GROUP_HORIZONTAL);
        sg_combo = Gtk::SizeGroup::create(Gtk::SIZE_GROUP_HORIZONTAL);
        set_header_func(&header_func_separator);
        set_selection_mode(Gtk::SELECTION_NONE);
        std::deque<const Pin *> pins_sorted;
        for (const auto &it : gate->unit->pins) {
            pins_sorted.push_back(&it.second);
        }
        std::sort(pins_sorted.begin(), pins_sorted.end(),
                  [](const auto &a, const auto &b) { return strcmp_natural(a->primary_name, b->primary_name) < 0; });
        for (const auto it : pins_sorted) {
            auto box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 16));
            auto la = Gtk::manage(new Gtk::Label(it->primary_name));
            la->set_xalign(0);
            sg->add_widget(*la);
            box->pack_start(*la, false, false, 0);

            auto bu = Gtk::manage(new PinNamesButton(comp, UUIDPath<2>(gate->uuid, it->uuid)));
            sg_combo->add_widget(*bu);
            bu->show();
            box->pack_start(*bu, true, true, 0);

            /*auto combo = Gtk::manage(new Gtk::ComboBoxText());
            sg_combo->add_widget(*combo);

            combo->append(std::to_string(-1), it->primary_name);
            unsigned int i = 0;
            for (const auto &it_pin_name : it->names) {
                combo->append(std::to_string(i++), it_pin_name);
            }
            combo->set_active(0);
            auto path = UUIDPath<2>(gate->uuid, it->uuid);
            if (comp->pin_names.count(path)) {
                combo->set_active(1 + comp->pin_names.at(path));
            }

            combo->signal_changed().connect(sigc::bind<Gtk::ComboBoxText *, UUIDPath<2>>(
                    sigc::mem_fun(*this, &GatePinEditor::changed), combo, path));
            box->pack_start(*combo, false, false, 0);


            auto ed = Gtk::manage(new Gtk::Entry);
            ed->set_placeholder_text("Custom pin name");
            ed->show();
            if (comp->custom_pin_names.count(path)) {
                ed->set_text(comp->custom_pin_names.at(path));
            }
            ed->signal_changed().connect([this, ed, path] {
                std::string v = ed->get_text();
                trim(v);
                comp->custom_pin_names[path] = v;
            });

            box->pack_start(*ed, true, true, 0);
            */

            box->set_margin_start(16);
            box->set_margin_end(8);
            box->set_margin_top(4);
            box->set_margin_bottom(4);


            insert(*box, -1);
        }
    }
    Component *comp;
    const Gate *gate;

private:
    Glib::RefPtr<Gtk::SizeGroup> sg;
    Glib::RefPtr<Gtk::SizeGroup> sg_combo;

    void changed(Gtk::ComboBoxText *combo, UUIDPath<2> path)
    {
        // comp->pin_names[path] = combo->get_active_row_number() - 1;
    }
};


SymbolPinNamesDialog::SymbolPinNamesDialog(Gtk::Window *parent, SchematicSymbol *s)
    : Gtk::Dialog("Symbol " + s->component->refdes + s->gate->suffix + " pin names", *parent,
                  Gtk::DialogFlags::DIALOG_MODAL | Gtk::DialogFlags::DIALOG_USE_HEADER_BAR),
      sym(s)
{
    add_button("Cancel", Gtk::ResponseType::RESPONSE_CANCEL);
    add_button("OK", Gtk::ResponseType::RESPONSE_OK);
    set_default_response(Gtk::ResponseType::RESPONSE_OK);
    set_default_size(300, 700);

    auto box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL, 0));
    auto mode_combo = Gtk::manage(new Gtk::ComboBoxText());
    mode_combo->set_margin_start(8);
    mode_combo->set_margin_end(8);
    mode_combo->set_margin_top(8);
    mode_combo->set_margin_bottom(8);
    for (const auto &it : object_descriptions.at(ObjectType::SCHEMATIC_SYMBOL)
                                  .properties.at(ObjectProperty::ID::PIN_NAME_DISPLAY)
                                  .enum_items) {
        mode_combo->append(std::to_string(it.first), it.second);
    }
    mode_combo->set_active_id(std::to_string(static_cast<int>(sym->pin_display_mode)));
    mode_combo->signal_changed().connect([this, mode_combo] {
        sym->pin_display_mode = static_cast<SchematicSymbol::PinDisplayMode>(std::stoi(mode_combo->get_active_id()));
    });


    box->pack_start(*mode_combo, false, false, 0);

    auto ed = Gtk::manage(new GatePinEditor(sym->component, sym->gate));
    auto sc = Gtk::manage(new Gtk::ScrolledWindow());
    sc->add(*ed);
    box->pack_start(*sc, true, true, 0);


    get_content_area()->pack_start(*box, true, true, 0);
    get_content_area()->set_border_width(0);

    show_all();
}
} // namespace horizon
