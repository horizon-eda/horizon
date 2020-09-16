#include "unit_editor.hpp"
#include <iostream>
#include "pool/unit.hpp"
#include "util/util.hpp"
#include <glibmm.h>
#include <iomanip>
#include "util/pool_completion.hpp"
#include "util/gtk_util.hpp"
#include "widgets/help_button.hpp"
#include "help_texts.hpp"

namespace horizon {

class PinEditor : public Gtk::Box {
public:
    PinEditor(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x, class Pin *p, UnitEditor *pa);
    static PinEditor *create(class Pin *p, UnitEditor *pa);
    class Pin *pin;
    UnitEditor *parent;
    void focus();

private:
    Gtk::Entry *name_entry = nullptr;
    Gtk::Entry *names_entry = nullptr;
    Gtk::ComboBoxText *dir_combo = nullptr;
};

PinEditor::PinEditor(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x, Pin *p, UnitEditor *pa)
    : Gtk::Box(cobject), pin(p), parent(pa)
{
    x->get_widget("pin_name", name_entry);
    x->get_widget("pin_names", names_entry);
    x->get_widget("pin_direction", dir_combo);
    entry_add_sanitizer(name_entry);
    entry_add_sanitizer(names_entry);
    parent->sg_name->add_widget(*name_entry);
    parent->sg_direction->add_widget(*dir_combo);
    parent->sg_names->add_widget(*names_entry);
    widget_remove_scroll_events(*dir_combo);

    for (const auto &it : Pin::direction_names) {
        dir_combo->append(std::to_string(static_cast<int>(it.first)), it.second);
    }

    name_entry->set_text(pin->primary_name);
    name_entry->signal_changed().connect([this] {
        pin->primary_name = name_entry->get_text();
        parent->set_needs_save();
    });
    name_entry->signal_activate().connect([this] { parent->handle_activate(this); });
    name_entry->signal_focus_in_event().connect([this](GdkEventFocus *ev) {
        auto this_row = dynamic_cast<Gtk::ListBoxRow *>(get_ancestor(GTK_TYPE_LIST_BOX_ROW));
        if (!this_row->is_selected()) {
            auto lb = dynamic_cast<Gtk::ListBox *>(get_ancestor(GTK_TYPE_LIST_BOX));
            lb->unselect_all();
            lb->select_row(*this_row);
        }
        return false;
    });

    {
        std::stringstream ss;
        std::copy(pin->names.begin(), pin->names.end(), std::ostream_iterator<std::string>(ss, " "));
        std::string s(ss.str());
        if (s.size())
            s.pop_back();
        names_entry->set_text(s);
    }
    names_entry->signal_changed().connect([this] {
        std::stringstream ss(names_entry->get_text());
        std::istream_iterator<std::string> begin(ss);
        std::istream_iterator<std::string> end;
        std::vector<std::string> names(begin, end);
        pin->names = names;
        parent->set_needs_save();
    });

    auto propagate = [this](std::function<void(PinEditor *)> fn) {
        auto lb = dynamic_cast<Gtk::ListBox *>(get_ancestor(GTK_TYPE_LIST_BOX));
        auto this_row = dynamic_cast<Gtk::ListBoxRow *>(get_ancestor(GTK_TYPE_LIST_BOX_ROW));
        auto rows = lb->get_selected_rows();
        if (rows.size() > 1 && this_row->is_selected()) {
            for (auto &row : rows) {
                if (auto ed = dynamic_cast<PinEditor *>(row->get_child())) {
                    fn(ed);
                }
            }
        }
    };

    dir_combo->set_active_id(std::to_string(static_cast<int>(pin->direction)));
    dir_combo->signal_changed().connect([this, propagate] {
        pin->direction = static_cast<Pin::Direction>(std::stoi(dir_combo->get_active_id()));
        propagate([this](PinEditor *ed) { ed->dir_combo->set_active_id(dir_combo->get_active_id()); });
        parent->set_needs_save();
    });
}

PinEditor *PinEditor::create(Pin *p, UnitEditor *pa)
{
    PinEditor *w;
    Glib::RefPtr<Gtk::Builder> x = Gtk::Builder::create();
    static const std::vector<Glib::ustring> widgets = {"pin_editor"};
    x->add_from_resource("/org/horizon-eda/horizon/pool-prj-mgr/pool-mgr/editors/unit_editor.ui", widgets);
    x->get_widget_derived("pin_editor", w, p, pa);
    w->reference();
    return w;
}

void PinEditor::focus()
{
    name_entry->grab_focus();
}

void UnitEditor::sort()
{
    pins_listbox->set_sort_func([](Gtk::ListBoxRow *a, Gtk::ListBoxRow *b) {
        auto na = dynamic_cast<PinEditor *>(a->get_child())->pin->primary_name;
        auto nb = dynamic_cast<PinEditor *>(b->get_child())->pin->primary_name;
        return strcmp_natural(na, nb);
    });
    pins_listbox->invalidate_sort();
    pins_listbox->unset_sort_func();
}

UnitEditor::UnitEditor(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x, Unit &u, class IPool &p)
    : Gtk::Box(cobject), unit(u), pool(p)
{
    x->get_widget("unit_name", name_entry);
    x->get_widget("unit_manufacturer", manufacturer_entry);
    x->get_widget("unit_pins", pins_listbox);
    x->get_widget("unit_pins_refresh", refresh_button);
    x->get_widget("pin_add", add_button);
    x->get_widget("pin_delete", delete_button);
    x->get_widget("cross_probing", cross_probing_cb);
    entry_add_sanitizer(name_entry);
    entry_add_sanitizer(manufacturer_entry);

    sg_name = decltype(sg_name)::cast_dynamic(x->get_object("sg_name"));
    sg_direction = decltype(sg_direction)::cast_dynamic(x->get_object("sg_direction"));
    sg_names = decltype(sg_names)::cast_dynamic(x->get_object("sg_names"));

    HelpButton::pack_into(x, "box_names", HelpTexts::UNIT_PIN_ALT_NAMES);

    name_entry->set_text(unit.name);
    name_entry->signal_changed().connect([this] {
        unit.name = name_entry->get_text();
        set_needs_save();
    });
    manufacturer_entry->set_text(unit.manufacturer);
    manufacturer_entry->set_completion(create_pool_manufacturer_completion(pool));
    manufacturer_entry->signal_changed().connect([this] {
        unit.manufacturer = manufacturer_entry->get_text();
        set_needs_save();
    });

    for (auto &it : unit.pins) {
        auto ed = PinEditor::create(&it.second, this);
        ed->show_all();
        pins_listbox->append(*ed);
        ed->unreference();
    }

    refresh_button->signal_clicked().connect([this] { sort(); });

    delete_button->signal_clicked().connect(sigc::mem_fun(*this, &UnitEditor::handle_delete));
    add_button->signal_clicked().connect(sigc::mem_fun(*this, &UnitEditor::handle_add));

    pins_listbox->signal_key_press_event().connect([this](GdkEventKey *ev) {
        if (ev->keyval == GDK_KEY_Delete) {
            handle_delete();
            return true;
        }
        return false;
    });

    sort();
}

void UnitEditor::handle_delete()
{
    auto rows = pins_listbox->get_selected_rows();
    std::set<int> indices;
    std::set<UUID> uuids;
    for (auto &row : rows) {
        uuids.insert(dynamic_cast<PinEditor *>(row->get_child())->pin->uuid);
        indices.insert(row->get_index());
    }
    for (auto &row : rows) {
        delete row;
    }
    for (auto it = unit.pins.begin(); it != unit.pins.end();) {
        if (uuids.find(it->first) != uuids.end()) {
            unit.pins.erase(it++);
        }
        else {
            it++;
        }
    }
    for (auto index : indices) {
        auto row = pins_listbox->get_row_at_index(index);
        if (row)
            pins_listbox->select_row(*row);
    }
    set_needs_save();
}

static std::string inc_pin_name(const std::string &s, int inc = 1)
{
    Glib::ustring u(s);
    Glib::MatchInfo ma;
    const auto regex = Glib::Regex::create("^(\\D*)(\\d+)(\\D*)\\d*$");
    if (regex->match(u, ma)) {
        auto number_str = ma.fetch(2);
        auto number = std::stoi(number_str) + inc;
        std::stringstream ss;
        ss << ma.fetch(1);
        ss << std::setfill('0') << std::setw(number_str.size()) << number;
        ss << ma.fetch(3);
        return ss.str();
    }
    else {
        return s;
    }
}

// adapted from gtklistbox.c
static void listbox_ensure_row_visible(Gtk::ListBox *box, Gtk::ListBoxRow *row)
{
    Gtk::Widget *header = nullptr;
    gint y, height;

    auto adjustment = box->get_adjustment();
    if (!adjustment)
        return;

    auto allocation = row->get_allocation();
    y = allocation.get_y();
    height = allocation.get_height();

    /* If the row has a header, we want to ensure that it is visible as well. */
    header = row->get_header();
    if (GTK_IS_WIDGET(header) && gtk_widget_is_drawable(header->gobj())) {
        auto header_allocation = header->get_allocation();
        y = header_allocation.get_y();
        height += header_allocation.get_height();
    }

    adjustment->clamp_page(y, y + height);
}

void UnitEditor::handle_add()
{
    const Pin *pin_selected = nullptr;
    auto rows = pins_listbox->get_selected_rows();
    for (auto &row : rows) {
        pin_selected = dynamic_cast<PinEditor *>(row->get_child())->pin;
    }

    auto uu = UUID::random();
    auto pin = &unit.pins.emplace(uu, uu).first->second;
    if (pin_selected) {
        pin->swap_group = pin_selected->swap_group;
        pin->direction = pin_selected->direction;
        pin->primary_name = inc_pin_name(pin_selected->primary_name);
        pin->names = pin_selected->names;
    }

    int index = 0;
    {
        auto children = pins_listbox->get_children();
        index = children.size();
        int i = 0;
        for (auto &ch : children) {
            auto row = dynamic_cast<Gtk::ListBoxRow *>(ch);
            auto ed_row = dynamic_cast<PinEditor *>(row->get_child());
            if (ed_row->pin == pin_selected) {
                index = i;
                break;
            }
            i++;
        }
    }


    auto ed = PinEditor::create(pin, this);
    ed->show_all();
    pins_listbox->insert(*ed, index + 1);
    ed->unreference();

    auto children = pins_listbox->get_children();
    for (auto &ch : children) {
        auto row = dynamic_cast<Gtk::ListBoxRow *>(ch);
        auto ed_row = dynamic_cast<PinEditor *>(row->get_child());
        if (ed_row->pin->uuid == uu) {
            pins_listbox->unselect_all();
            pins_listbox->select_row(*row);
            ed_row->focus();
            Glib::signal_idle().connect_once([this, row] { listbox_ensure_row_visible(pins_listbox, row); });
            break;
        }
    }
    set_needs_save();
}

void UnitEditor::handle_activate(PinEditor *ed)
{
    auto children = pins_listbox->get_children();
    size_t i = 0;
    for (auto &ch : children) {
        auto row = dynamic_cast<Gtk::ListBoxRow *>(ch);
        auto ed_row = dynamic_cast<PinEditor *>(row->get_child());
        if (ed_row == ed) {
            if (i == children.size() - 1) {
                pins_listbox->unselect_all();
                pins_listbox->select_row(*row);
                handle_add();
            }
            else {
                auto row_next = dynamic_cast<Gtk::ListBoxRow *>(children.at(i + 1));
                auto ed_next = dynamic_cast<PinEditor *>(row_next->get_child());
                pins_listbox->unselect_all();
                pins_listbox->select_row(*row_next);
                listbox_ensure_row_visible(pins_listbox, row_next);
                ed_next->focus();
            }
            return;
        }
        i++;
    }
}

void UnitEditor::select(const ItemSet &items)
{
    if (!cross_probing_cb->get_active())
        return;

    pins_listbox->unselect_all();
    for (auto &ch : pins_listbox->get_children()) {
        auto row = dynamic_cast<Gtk::ListBoxRow *>(ch);
        auto ed_row = dynamic_cast<PinEditor *>(row->get_child());
        if (items.count({ObjectType::SYMBOL_PIN, ed_row->pin->uuid})) {
            pins_listbox->select_row(*row);
        }
    }
}

UnitEditor *UnitEditor::create(Unit &u, class IPool &p)
{
    UnitEditor *w;
    Glib::RefPtr<Gtk::Builder> x = Gtk::Builder::create();
    x->add_from_resource("/org/horizon-eda/horizon/pool-prj-mgr/pool-mgr/editors/unit_editor.ui");
    x->get_widget_derived("unit_editor", w, u, p);
    w->reference();
    return w;
}
} // namespace horizon
