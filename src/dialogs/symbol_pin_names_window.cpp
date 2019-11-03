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

class PinNamesBox : public Gtk::FlowBox, public Changeable {
public:
    PinNamesBox(Component *c, const UUIDPath<2> &p, Glib::RefPtr<Gtk::SizeGroup> sg_combo)
        : Gtk::FlowBox(), comp(c), path(p), pin(comp->entity->gates.at(path.at(0)).unit->pins.at(path.at(1)))
    {
        set_homogeneous(true);
        set_selection_mode(Gtk::SELECTION_NONE);
        set_min_children_per_line(2);

        {
            auto primary_button = Gtk::manage(new Gtk::CheckButton(pin.primary_name + " (primary)"));
            if (comp->pin_names.count(path))
                primary_button->set_active(comp->pin_names.at(path).count(-1));
            primary_button->signal_toggled().connect([this, primary_button] {
                add_remove_name(-1, primary_button->get_active());
                s_signal_changed.emit();
            });
            sg_combo->add_widget(*primary_button);
            add(*primary_button);
        }

        {
            int i = 0;
            for (const auto &pin_name : pin.names) {
                auto cb = Gtk::manage(new Gtk::CheckButton(pin_name));
                if (comp->pin_names.count(path))
                    cb->set_active(comp->pin_names.at(path).count(i));
                cb->signal_toggled().connect([this, i, cb] {
                    add_remove_name(i, cb->get_active());
                    s_signal_changed.emit();
                });
                i++;
                sg_combo->add_widget(*cb);
                add(*cb);
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
                s_signal_changed.emit();
            });
            custom_entry->set_sensitive(false);
            custom_entry->set_placeholder_text("Custom");
            custom_entry->signal_changed().connect([this] {
                comp->custom_pin_names[path] = custom_entry->get_text();
                s_signal_changed.emit();
            });
            if (comp->pin_names.count(path))
                custom_cb->set_active(comp->pin_names.at(path).count(-2));
            sg_combo->add_widget(*cbox);
            add(*cbox);
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

private:
    PinNamesBox *box = nullptr;
};

class GatePinEditor : public Gtk::ListBox, public Changeable {
public:
    GatePinEditor(Component *c, const Gate *g) : Gtk::ListBox(), comp(c), gate(g)
    {
        sg = Gtk::SizeGroup::create(Gtk::SIZE_GROUP_HORIZONTAL);
        sg_combo = Gtk::SizeGroup::create(Gtk::SIZE_GROUP_HORIZONTAL);
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

            auto bu = Gtk::manage(new PinNamesBox(comp, UUIDPath<2>(gate->uuid, it->uuid), sg_combo));
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

private:
    Glib::RefPtr<Gtk::SizeGroup> sg;
    Glib::RefPtr<Gtk::SizeGroup> sg_combo;
    std::vector<PinNamesBox *> boxes;
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

SymbolPinNamesWindow::SymbolPinNamesWindow(Gtk::Window *parent, ImpInterface *intf, SchematicSymbol *s)
    : ToolWindow(parent, intf), sym(s)
{
    set_title("Symbol " + s->component->refdes + s->gate->suffix + " pin names");
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
        emit_event(ToolDataWindow::Event::UPDATE);
    });

    auto box2 = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 4));
    box2->property_margin() = 8;
    box->pack_start(*box2, false, false, 0);
    box2->pack_start(*mode_combo, true, true, 0);

    auto import_button = Gtk::manage(new Gtk::Button("Import custom pin names"));
    import_button->signal_clicked().connect(sigc::mem_fun(*this, &SymbolPinNamesWindow::handle_import));
    box2->pack_start(*import_button, false, false, 0);

    auto sep = Gtk::manage(new Gtk::Separator(Gtk::ORIENTATION_HORIZONTAL));
    box->pack_start(*sep, false, false, 0);

    editor = Gtk::manage(new GatePinEditor(sym->component, sym->gate));
    editor->signal_changed().connect([this] { emit_event(ToolDataWindow::Event::UPDATE); });
    editor->signal_selected_rows_changed().connect([this] { emit_event(ToolDataWindow::Event::UPDATE); });
    auto sc = Gtk::manage(new Gtk::ScrolledWindow());
    sc->set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC);
    sc->add(*editor);
    box->pack_start(*sc, true, true, 0);


    add(*box);

    show_all();
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
