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
#include "widgets/pin_names_editor.hpp"
#include <nlohmann/json.hpp>
#include "checks/check_unit.hpp"

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
    PinNamesEditor *names_editor = nullptr;
    Gtk::ComboBoxText *dir_combo = nullptr;
};

PinEditor::PinEditor(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x, Pin *p, UnitEditor *pa)
    : Gtk::Box(cobject), pin(p), parent(pa)
{
    x->get_widget("pin_name", name_entry);
    x->get_widget("pin_direction", dir_combo);
    entry_add_sanitizer(name_entry);
    parent->sg_name->add_widget(*name_entry);
    parent->sg_direction->add_widget(*dir_combo);
    widget_remove_scroll_events(*dir_combo);

    {
        Gtk::Box *pin_names_box;
        x->get_widget("pin_names_box", pin_names_box);
        names_editor = Gtk::manage(new PinNamesEditor(pin->names));
        pin_names_box->pack_start(*names_editor, true, true, 0);
        names_editor->show();
    }
    parent->sg_names->add_widget(*names_editor);


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

    names_editor->signal_changed().connect([this] { parent->set_needs_save(); });

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
        if (parent->propagating)
            return;
        parent->propagating = true;
        propagate([this](PinEditor *ed) { ed->dir_combo->set_active_id(dir_combo->get_active_id()); });
        parent->set_needs_save();
        parent->propagating = false;
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
    if (sort_helper.get_column() == "pin") {
        pins_listbox->set_sort_func([this](Gtk::ListBoxRow *a, Gtk::ListBoxRow *b) {
            auto na = dynamic_cast<PinEditor *>(a->get_child())->pin->primary_name;
            auto nb = dynamic_cast<PinEditor *>(b->get_child())->pin->primary_name;
            return sort_helper.transform_order(strcmp_natural(na, nb));
        });
    }
    else if (sort_helper.get_column() == "direction") {
        pins_listbox->set_sort_func([this](Gtk::ListBoxRow *a, Gtk::ListBoxRow *b) {
            auto da = static_cast<int>(dynamic_cast<PinEditor *>(a->get_child())->pin->direction);
            auto db = static_cast<int>(dynamic_cast<PinEditor *>(b->get_child())->pin->direction);
            return sort_helper.transform_order(da - db);
        });
    }
    pins_listbox->invalidate_sort();
    pins_listbox->unset_sort_func();
}

UnitEditor::UnitEditor(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x, const std::string &fn,
                       class IPool &p)
    : Gtk::Box(cobject), PoolEditorBase(fn, p),
      unit(filename.size() ? Unit::new_from_file(filename) : Unit(UUID::random()))
{
    history_append("init");
    x->get_widget("unit_name", name_entry);
    x->get_widget("unit_manufacturer", manufacturer_entry);
    x->get_widget("unit_pins", pins_listbox);
    x->get_widget("pin_add", add_button);
    x->get_widget("pin_delete", delete_button);
    x->get_widget("cross_probing", cross_probing_cb);
    x->get_widget("pin_count", pin_count_label);
    entry_add_sanitizer(name_entry);
    entry_add_sanitizer(manufacturer_entry);

    sg_name = decltype(sg_name)::cast_dynamic(x->get_object("sg_name"));
    sg_direction = decltype(sg_direction)::cast_dynamic(x->get_object("sg_direction"));
    sg_names = decltype(sg_names)::cast_dynamic(x->get_object("sg_names"));

    HelpButton::pack_into(x, "box_names", HelpTexts::UNIT_PIN_ALT_NAMES);

    name_entry->signal_changed().connect([this] {
        if (is_loading())
            return;
        unit.name = name_entry->get_text();
        set_needs_save();
    });
    manufacturer_entry->set_completion(create_pool_manufacturer_completion(pool));
    manufacturer_entry->signal_changed().connect([this] {
        if (is_loading())
            return;
        unit.manufacturer = manufacturer_entry->get_text();
        set_needs_save();
    });

    delete_button->signal_clicked().connect(sigc::mem_fun(*this, &UnitEditor::handle_delete));
    add_button->signal_clicked().connect(sigc::mem_fun(*this, &UnitEditor::handle_add));

    pins_listbox->signal_key_press_event().connect([this](GdkEventKey *ev) {
        if (ev->keyval == GDK_KEY_Delete) {
            handle_delete();
            return true;
        }
        return false;
    });

    sort_helper.attach("pin", x);
    sort_helper.attach("direction", x);
    sort_helper.signal_changed().connect(sigc::mem_fun(*this, &UnitEditor::sort));

    label_set_tnum(pin_count_label);

    load();
}

void UnitEditor::load()
{
    auto loading_setter = set_loading();

    name_entry->set_text(unit.name);
    manufacturer_entry->set_text(unit.manufacturer);
    for (auto ch : pins_listbox->get_children()) {
        delete ch;
    }
    for (auto &it : unit.pins) {
        auto ed = PinEditor::create(&it.second, this);
        ed->show_all();
        pins_listbox->append(*ed);
        ed->unreference();
    }
    sort_helper.set_sort("pin", Gtk::SORT_ASCENDING);
    update_pin_count();
}

void UnitEditor::handle_delete()
{
    auto rows = pins_listbox->get_selected_rows();
    if (rows.size() == 0)
        return;
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
    update_pin_count();
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
            listbox_ensure_row_visible_new(pins_listbox, row);
            break;
        }
    }
    set_needs_save();
    update_pin_count();
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

void UnitEditor::update_pin_count()
{
    const uint32_t n = unit.pins.size();
    std::string s = std::to_string(n);
    if (n == 1)
        s += " pin";
    else
        s += " pins";
    pin_count_label->set_text(s);
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

void UnitEditor::save_as(const std::string &fn)
{
    unset_needs_save();
    filename = fn;
    save_json_to_file(fn, unit.serialize());
}

std::string UnitEditor::get_name() const
{
    return unit.name;
}

const UUID &UnitEditor::get_uuid() const
{
    return unit.uuid;
}

RulesCheckResult UnitEditor::run_checks() const
{
    return check_unit(unit);
}

const FileVersion &UnitEditor::get_version() const
{
    return unit.version;
}

unsigned int UnitEditor::get_required_version() const
{
    return unit.get_required_version();
}

ObjectType UnitEditor::get_type() const
{
    return ObjectType::UNIT;
}

class HistoryItemUnit : public HistoryManager::HistoryItem {
public:
    HistoryItemUnit(const Unit &u, const std::string &cm) : HistoryManager::HistoryItem(cm), unit(u)
    {
    }
    Unit unit;
};

std::unique_ptr<HistoryManager::HistoryItem> UnitEditor::make_history_item(const std::string &comment)
{
    return std::make_unique<HistoryItemUnit>(unit, comment);
}

void UnitEditor::history_load(const HistoryManager::HistoryItem &it)
{
    const auto &x = dynamic_cast<const HistoryItemUnit &>(it);
    unit = x.unit;
    load();
}

UnitEditor *UnitEditor::create(const std::string &fn, class IPool &p)
{
    UnitEditor *w;
    Glib::RefPtr<Gtk::Builder> x = Gtk::Builder::create();
    x->add_from_resource("/org/horizon-eda/horizon/pool-prj-mgr/pool-mgr/editors/unit_editor.ui");
    x->get_widget_derived("unit_editor", w, fn, p);
    w->reference();
    return w;
}

} // namespace horizon
