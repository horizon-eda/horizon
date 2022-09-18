#include "symbol_pin_names_window.hpp"
#include "pool/gate.hpp"
#include "pool/entity.hpp"
#include "schematic/schematic_symbol.hpp"
#include "util/util.hpp"
#include "util/str_util.hpp"
#include "util/gtk_util.hpp"
#include "common/object_descr.hpp"
#include "util/csv.hpp"
#include "core/tool_data_window.hpp"
#include "imp/imp_interface.hpp"
#include "util/changeable.hpp"
#include <iostream>
#include <deque>
#include <algorithm>

namespace horizon {

class CheckButtonWithHighlight : public Gtk::CheckButton {
public:
    CheckButtonWithHighlight(const Glib::ustring &labels) : label_orig(labels), label_orig_casefold(labels.casefold())
    {
        label = Gtk::manage(new Gtk::Label(labels));
        label->show();
        add(*label);
    }

    bool search(const Glib::ustring &s)
    {
        if (s.empty()) {
            label->set_text(label_orig);
            return true;
        }

        // if one of them isn't ascii the casefold represantation might have more/less chars and mess up the
        // highlighting
        if (!s.is_ascii() || !label_orig.is_ascii())
            return false;
        const auto needle = s.casefold();
        if (auto pos = label_orig_casefold.find(needle); pos != Glib::ustring::npos) {
            Glib::ustring l = Glib::Markup::escape_text(label_orig.substr(0, pos))
                              + "<span bgcolor=\"yellow\" color=\"black\">"
                              + Glib::Markup::escape_text(label_orig.substr(pos, needle.size())) + "</span>"
                              + Glib::Markup::escape_text(label_orig.substr(pos + needle.size()));
            label->set_markup(l);
            return true;
        }
        label->set_text(label_orig);
        return false;
    }

private:
    Gtk::Label *label = nullptr;
    const Glib::ustring label_orig;
    const Glib::ustring label_orig_casefold;
};

class PinNamesBox : public Gtk::Box, public Changeable {
public:
    PinNamesBox(Component *c, const UUIDPath<2> &p)
        : Gtk::Box(Gtk::ORIENTATION_VERTICAL, 4), comp(c), path(p),
          pin(comp->entity->gates.at(path.at(0)).unit->pins.at(path.at(1)))
    {
        auto fbox = Gtk::manage(new Gtk::FlowBox);
        fbox->set_homogeneous(true);
        fbox->set_selection_mode(Gtk::SELECTION_NONE);
        fbox->set_min_children_per_line(2);
        pack_start(*fbox, true, true, 0);

        {
            auto primary_button = Gtk::manage(new CheckButtonWithHighlight(
                    pin.primary_name + " (primary, " + Pin::direction_abbreviations.at(pin.direction) + ")"));
            check_buttons.push_back(primary_button);
            if (comp->alt_pins.count(path))
                primary_button->set_active(comp->alt_pins.at(path).use_primary_name);
            primary_button->signal_toggled().connect([this, primary_button] {
                comp->alt_pins[path].use_primary_name = primary_button->get_active();
                s_signal_changed.emit();
            });
            fbox->add(*primary_button);
        }

        {
            for (const auto &[uu, name] : pin.names) {
                auto cb = Gtk::manage(new CheckButtonWithHighlight(
                        name.name + " (" + Pin::direction_abbreviations.at(name.direction) + ")"));
                check_buttons.push_back(cb);
                if (comp->alt_pins.count(path))
                    cb->set_active(comp->alt_pins.at(path).pin_names.count(uu));
                const UUID alt_uu = uu;
                cb->signal_toggled().connect([this, alt_uu, cb] {
                    add_remove_name(alt_uu, cb->get_active());
                    s_signal_changed.emit();
                });

                fbox->add(*cb);
            }
        }

        {
            auto cbox = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL));
            cbox->set_margin_start(3);
            custom_cb = Gtk::manage(new Gtk::CheckButton(""));

            cbox->pack_start(*custom_cb, false, false, 0);
            custom_entry = Gtk::manage(new Gtk::Entry);
            if (comp->alt_pins.count(path))
                custom_entry->set_text(comp->alt_pins.at(path).custom_name);
            cbox->pack_start(*custom_entry, true, true, 0);
            custom_cb->signal_toggled().connect([this] {
                custom_entry->set_sensitive(custom_cb->get_active());
                comp->alt_pins[path].use_custom_name = custom_cb->get_active();
                s_signal_changed.emit();
            });
            custom_entry->set_sensitive(false);
            custom_entry->set_placeholder_text("Custom");
            custom_entry->signal_changed().connect([this] {
                comp->alt_pins[path].custom_name = custom_entry->get_text();
                s_signal_changed.emit();
            });
            if (comp->alt_pins.count(path))
                custom_cb->set_active(comp->alt_pins.at(path).use_custom_name);

            dir_combo = Gtk::manage(new Gtk::ComboBoxText);
            dir_combo->set_margin_start(8);
            widget_remove_scroll_events(*dir_combo);
            for (const auto &it : Pin::direction_names) {
                dir_combo->append(std::to_string(static_cast<int>(it.first)), it.second);
            }
            if (comp->alt_pins.count(path))
                dir_combo->set_active_id(std::to_string(static_cast<int>(comp->alt_pins.at(path).custom_direction)));
            else
                dir_combo->set_active_id(std::to_string(static_cast<int>(Pin::Direction::BIDIRECTIONAL)));

            dir_combo->signal_changed().connect([this] {
                comp->alt_pins[path].custom_direction =
                        static_cast<Pin::Direction>(std::stoi(dir_combo->get_active_id()));
                s_signal_changed.emit();
            });

            cbox->pack_start(*dir_combo, true, true, 0);
            pack_start(*cbox, false, false, 0);
        }
    }

    const Pin &get_pin() const
    {
        return pin;
    }

    void set_custom_name(const std::string &name)
    {
        if (!name.size())
            return;
        custom_entry->set_text(name);
        custom_cb->set_active(true);
    }

    bool set_search(const std::string &s)
    {
        bool found_any = false;
        for (auto ch : check_buttons) {
            if (ch->search(s))
                found_any = true;
        }
        return found_any;
    }

private:
    Component *comp;
    UUIDPath<2> path;
    const Pin &pin;
    Gtk::Entry *custom_entry = nullptr;
    Gtk::CheckButton *custom_cb = nullptr;
    Gtk::ComboBoxText *dir_combo = nullptr;
    std::vector<CheckButtonWithHighlight *> check_buttons;

    void add_remove_name(const UUID &name, bool add)
    {
        if (add)
            comp->alt_pins[path].pin_names.insert(name);
        else
            comp->alt_pins[path].pin_names.erase(name);
    }
};

class GatePinRow : public Gtk::Box {
public:
    GatePinRow(const std::string &label, Glib::RefPtr<Gtk::SizeGroup> sg_label)
        : Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 16)
    {
        auto la = Gtk::manage(new Gtk::Label(label));
        la->set_xalign(0);
        sg_label->add_widget(*la);
        pack_start(*la, false, false, 0);
    }

    void add_box(PinNamesBox *b)
    {
        box = b;
        pack_start(*b, true, true, 0);
    }

    PinNamesBox *get_box()
    {
        return box;
    }

    void set_search(const std::string &s)
    {
        get_parent()->set_visible(box->set_search(s));
    }

private:
    PinNamesBox *box = nullptr;
};

class GatePinEditor : public Gtk::ListBox, public Changeable {
public:
    GatePinEditor(Component *c, const Gate *g) : Gtk::ListBox(), comp(c), gate(g)
    {
        sg = Gtk::SizeGroup::create(Gtk::SIZE_GROUP_HORIZONTAL);
        set_header_func(&header_func_separator);
        set_selection_mode(Gtk::SELECTION_SINGLE);
        std::deque<const Pin *> pins_sorted;
        for (const auto &it : gate->unit->pins) {
            pins_sorted.push_back(&it.second);
        }
        std::sort(pins_sorted.begin(), pins_sorted.end(),
                  [](const auto &a, const auto &b) { return strcmp_natural(a->primary_name, b->primary_name) < 0; });
        boxes.reserve(pins_sorted.size());
        for (const auto it : pins_sorted) {
            auto box = Gtk::manage(new GatePinRow(it->primary_name, sg));

            auto bu = Gtk::manage(new PinNamesBox(comp, UUIDPath<2>(gate->uuid, it->uuid)));
            bu->signal_changed().connect([this] { s_signal_changed.emit(); });
            bu->show();
            box->add_box(bu);

            box->set_margin_start(16);
            box->set_margin_end(8);
            box->set_margin_top(4);
            box->set_margin_bottom(4);

            boxes.push_back(bu);
            insert(*box, -1);
        }
    }
    Component *comp;
    const Gate *gate;

    const std::vector<PinNamesBox *> &get_boxes()
    {
        return boxes;
    }

    void set_search(const std::string &s)
    {
        search_string = s;
        trim(search_string);
        for (auto ch : get_children()) {
            auto &row = dynamic_cast<Gtk::ListBoxRow &>(*ch);
            auto &box = dynamic_cast<GatePinRow &>(*row.get_child());
            box.set_search(search_string);
        }
    }

private:
    Glib::RefPtr<Gtk::SizeGroup> sg;
    std::vector<PinNamesBox *> boxes;

    std::string search_string;
};

void SymbolPinNamesWindow::handle_import()
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
            auto &boxes = editor->get_boxes();
            for (const auto &it : csv) {
                std::string primary_name = it.at(0);
                std::string custom_name = it.at(1);
                trim(primary_name);
                trim(custom_name);
                auto box = std::find_if(boxes.begin(), boxes.end(),
                                        [&primary_name](auto &x) { return x->get_pin().primary_name == primary_name; });
                if (box != boxes.end()) {
                    (*box)->set_custom_name(custom_name);
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

SymbolPinNamesWindow::SymbolPinNamesWindow(Gtk::Window *parent, ImpInterface *intf, SchematicSymbol &s)
    : ToolWindow(parent, intf), sym(s)
{
    set_title("Symbol " + s.component->refdes + s.gate->suffix + " pin names");
    set_default_size(300, 700);
    auto box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL, 0));

    auto mode_combo = Gtk::manage(new Gtk::ComboBoxText());

    for (const auto &it : object_descriptions.at(ObjectType::SCHEMATIC_SYMBOL)
                                  .properties.at(ObjectProperty::ID::PIN_NAME_DISPLAY)
                                  .enum_items) {
        mode_combo->append(std::to_string(it.first), it.second);
    }
    mode_combo->set_active_id(std::to_string(static_cast<int>(sym.pin_display_mode)));
    mode_combo->signal_changed().connect([this, mode_combo] {
        sym.pin_display_mode = static_cast<SchematicSymbol::PinDisplayMode>(std::stoi(mode_combo->get_active_id()));
        emit_event(ToolDataWindow::Event::UPDATE);
    });

    auto box2 = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 4));
    box2->property_margin() = 8;
    box->pack_start(*box2, false, false, 0);
    box2->pack_start(*mode_combo, true, true, 0);

    auto import_button = Gtk::manage(new Gtk::Button("Import custom pin names"));
    import_button->signal_clicked().connect(sigc::mem_fun(*this, &SymbolPinNamesWindow::handle_import));
    box2->pack_start(*import_button, false, false, 0);

    search_button = Gtk::manage(new Gtk::ToggleButton);
    search_button->set_image_from_icon_name("edit-find-symbolic", Gtk::ICON_SIZE_BUTTON);
    box2->pack_start(*search_button, false, false, 0);

    search_revealer = Gtk::manage(new Gtk::Revealer);
    search_revealer->set_transition_duration(100);
    auto search_box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL, 0));

    {
        auto sep = Gtk::manage(new Gtk::Separator(Gtk::ORIENTATION_HORIZONTAL));
        search_box->pack_start(*sep, false, false, 0);
    }

    search_entry = Gtk::manage(new Gtk::SearchEntry);
    search_entry->property_margin() = 8;
    search_box->pack_start(*search_entry, false, false, 0);

    search_revealer->add(*search_box);
    box->pack_start(*search_revealer, false, false, 0);

    {
        auto sep = Gtk::manage(new Gtk::Separator(Gtk::ORIENTATION_HORIZONTAL));
        box->pack_start(*sep, false, false, 0);
    }
    search_button->signal_toggled().connect([this] {
        search_revealer->set_reveal_child(search_button->get_active());
        if (search_button->get_active())
            search_entry->grab_focus();
        update_search();
    });
    search_entry->signal_changed().connect(sigc::mem_fun(*this, &SymbolPinNamesWindow::update_search));
    search_entry->signal_stop_search().connect([this] { search_button->set_active(false); });

    editor = Gtk::manage(new GatePinEditor(sym.component, sym.gate));
    editor->signal_changed().connect([this] { emit_event(ToolDataWindow::Event::UPDATE); });
    editor->signal_selected_rows_changed().connect([this] { emit_event(ToolDataWindow::Event::UPDATE); });
    auto sc = Gtk::manage(new Gtk::ScrolledWindow());
    sc->set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC);
    sc->add(*editor);
    box->pack_start(*sc, true, true, 0);

    signal_key_press_event().connect([this](GdkEventKey *ev) {
        if (ev->keyval == GDK_KEY_f && (ev->state & GDK_CONTROL_MASK)) {
            search_button->set_active(true);
            return true;
        }
        return false;
    });


    add(*box);

    show_all();
}

void SymbolPinNamesWindow::update_search()
{
    if (search_revealer->get_reveal_child())
        editor->set_search(search_entry->get_text());
    else
        editor->set_search("");
}

static void ensure_row_visible(GtkListBox *box, GtkListBoxRow *row)
{
    GtkWidget *header;
    gint y, height;
    GtkAllocation allocation;

    GtkAdjustment *adjustment = gtk_list_box_get_adjustment(box);

    gtk_widget_get_allocation(GTK_WIDGET(row), &allocation);
    y = allocation.y;
    height = allocation.height;

    /* If the row has a header, we want to ensure that it is visible as well. */
    header = gtk_list_box_row_get_header(row);
    if (GTK_IS_WIDGET(header) && gtk_widget_is_drawable(header)) {
        gtk_widget_get_allocation(header, &allocation);
        y = allocation.y;
        height += allocation.height;
    }

    gtk_adjustment_clamp_page(adjustment, y, y + height);
}

void SymbolPinNamesWindow::go_to_pin(const UUID &uu)
{
    search_button->set_active(false);
    for (auto ed : editor->get_boxes()) {
        if (ed->get_pin().uuid == uu) {
            editor->unselect_all();
            auto row = dynamic_cast<Gtk::ListBoxRow *>(ed->get_ancestor(GTK_TYPE_LIST_BOX_ROW));
            editor->select_row(*row);
            ensure_row_visible(editor->gobj(), row->gobj());
            break;
        }
    }
}

UUID SymbolPinNamesWindow::get_selected_pin()
{
    auto row = editor->get_selected_row();
    if (row) {
        auto row2 = dynamic_cast<GatePinRow *>(row->get_child());
        return row2->get_box()->get_pin().uuid;
    }
    else {
        return UUID();
    }
}

} // namespace horizon
