#include "symbol_pin_names.hpp"
#include "pool/gate.hpp"
#include "pool/entity.hpp"
#include "schematic/schematic_symbol.hpp"
#include "util/util.hpp"
#include "util/str_util.hpp"
#include "util/gtk_util.hpp"
#include "common/object_descr.hpp"
#include "util/csv.hpp"
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
            custom_cb = Gtk::manage(new Gtk::CheckButton(""));
            cbox->pack_start(*custom_cb, false, false, 0);
            custom_entry = Gtk::manage(new Gtk::Entry);
            if (comp->custom_pin_names.count(path))
                custom_entry->set_text(comp->custom_pin_names.at(path));
            cbox->pack_start(*custom_entry, true, true, 0);
            custom_cb->signal_toggled().connect([this] {
                custom_entry->set_sensitive(custom_cb->get_active());
                add_remove_name(-2, custom_cb->get_active());
                update_label();
            });
            custom_entry->set_sensitive(false);
            custom_entry->set_placeholder_text("Custom");
            custom_entry->signal_changed().connect([this] {
                comp->custom_pin_names[path] = custom_entry->get_text();
                update_label();
            });
            if (comp->pin_names.count(path))
                custom_cb->set_active(comp->pin_names.at(path).count(-2));

            pbox->pack_start(*cbox, false, false, 0);
        }

        popover->add(*pbox);
        pbox->show_all();

        update_label();
        set_popover(*popover);
    }

    void set_custom_name(const std::string &name)
    {
        if (!name.size())
            return;
        custom_entry->set_text(name);
        custom_cb->set_active(true);
    }

    const Pin &get_pin() const
    {
        return pin;
    }

private:
    Component *comp;
    UUIDPath<2> path;
    const Pin &pin;
    Gtk::Entry *custom_entry = nullptr;
    Gtk::CheckButton *custom_cb = nullptr;

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
        buttons.reserve(pins_sorted.size());
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

            box->set_margin_start(16);
            box->set_margin_end(8);
            box->set_margin_top(4);
            box->set_margin_bottom(4);

            buttons.push_back(bu);
            insert(*box, -1);
        }
    }
    Component *comp;
    const Gate *gate;

    std::vector<PinNamesButton *> &get_buttons()
    {
        return buttons;
    }

private:
    Glib::RefPtr<Gtk::SizeGroup> sg;
    Glib::RefPtr<Gtk::SizeGroup> sg_combo;
    std::vector<PinNamesButton *> buttons;
};

void SymbolPinNamesDialog::handle_import()
{
    GtkFileChooserNative *native =
            gtk_file_chooser_native_new("Open", GTK_WINDOW(gobj()), GTK_FILE_CHOOSER_ACTION_OPEN, "_Open", "_Cancel");
    auto chooser = Glib::wrap(GTK_FILE_CHOOSER(native));
    auto filter = Gtk::FileFilter::create();
    filter->set_name("CSV documents");
    filter->add_pattern("*.csv");
    filter->add_pattern("*.CSV");
    chooser->add_filter(filter);

    if (gtk_native_dialog_run(GTK_NATIVE_DIALOG(native)) == GTK_RESPONSE_ACCEPT) {
        auto filename = chooser->get_filename();
        try {
            auto ifs = make_ifstream(filename);
            if (!ifs.is_open()) {
                throw std::runtime_error("file " + filename + " not opened");
            }
            CSV::Csv csv;
            ifs >> csv;
            ifs.close();
            csv.expand(2);
            auto &buttons = editor->get_buttons();
            for (const auto &it : csv) {
                std::string primary_name = it.at(0);
                std::string custom_name = it.at(1);
                trim(primary_name);
                trim(custom_name);
                auto bu = std::find_if(buttons.begin(), buttons.end(),
                                       [&primary_name](auto &x) { return x->get_pin().primary_name == primary_name; });
                if (bu != buttons.end()) {
                    (*bu)->set_custom_name(custom_name);
                }
            }
        }
        catch (const std::exception &e) {
            Gtk::MessageDialog md(*this, "Error importing", false /* use_markup */, Gtk::MESSAGE_ERROR,
                                  Gtk::BUTTONS_OK);
            md.set_secondary_text(e.what());
            md.run();
        }
        catch (...) {
            Gtk::MessageDialog md(*this, "Error importing", false /* use_markup */, Gtk::MESSAGE_ERROR,
                                  Gtk::BUTTONS_OK);
            md.set_secondary_text("unknown error");
            md.run();
        }
    }
}

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

    for (const auto &it : object_descriptions.at(ObjectType::SCHEMATIC_SYMBOL)
                                  .properties.at(ObjectProperty::ID::PIN_NAME_DISPLAY)
                                  .enum_items) {
        mode_combo->append(std::to_string(it.first), it.second);
    }
    mode_combo->set_active_id(std::to_string(static_cast<int>(sym->pin_display_mode)));
    mode_combo->signal_changed().connect([this, mode_combo] {
        sym->pin_display_mode = static_cast<SchematicSymbol::PinDisplayMode>(std::stoi(mode_combo->get_active_id()));
    });

    auto box2 = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 4));
    box2->property_margin() = 8;
    box->pack_start(*box2, false, false, 0);
    box2->pack_start(*mode_combo, true, true, 0);

    auto import_button = Gtk::manage(new Gtk::Button("Import custom pin names"));
    import_button->signal_clicked().connect(sigc::mem_fun(*this, &SymbolPinNamesDialog::handle_import));
    box2->pack_start(*import_button, false, false, 0);

    auto sep = Gtk::manage(new Gtk::Separator(Gtk::ORIENTATION_HORIZONTAL));
    box->pack_start(*sep, false, false, 0);

    editor = Gtk::manage(new GatePinEditor(sym->component, sym->gate));
    auto sc = Gtk::manage(new Gtk::ScrolledWindow());
    sc->add(*editor);
    box->pack_start(*sc, true, true, 0);


    get_content_area()->pack_start(*box, true, true, 0);
    get_content_area()->set_border_width(0);

    show_all();
}
} // namespace horizon
