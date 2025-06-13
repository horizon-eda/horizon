#include "part_editor.hpp"
#include "dialogs/pool_browser_dialog.hpp"
#include "widgets/pool_browser_package.hpp"
#include "widgets/pool_browser_part.hpp"
#include "widgets/tag_entry.hpp"
#include <iostream>
#include "pool/part.hpp"
#include "pool/pool_parametric.hpp"
#include "util/util.hpp"
#include "util/gtk_util.hpp"
#include <glibmm.h>
#include "util/pool_completion.hpp"
#include "parametric.hpp"
#include "pool/ipool.hpp"
#include <range/v3/view.hpp>
#include "util/range_util.hpp"
#include <nlohmann/json.hpp>
#include "checks/check_part.hpp"

namespace horizon {
class EntryWithInheritance : public Gtk::Box {
public:
    EntryWithInheritance()
        : Glib::ObjectBase(typeid(EntryWithInheritance)), Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 0),
          p_property_inherit(*this, "inherit"), p_property_can_inherit(*this, "can-inherit")
    {
        entry = Gtk::manage(new Gtk::Entry);
        entry_add_sanitizer(entry);
        button = Gtk::manage(new Gtk::ToggleButton("Inherit"));

        pack_start(*entry, true, true, 0);
        pack_start(*button, false, false, 0);
        get_style_context()->add_class("linked");
        entry->show();
        button->show();

        inh_binding = Glib::Binding::bind_property(button->property_active(), property_inherit(),
                                                   Glib::BINDING_BIDIRECTIONAL);
        property_inherit().signal_changed().connect([this] {
            entry->set_sensitive(!property_inherit());
            if (property_inherit()) {
                text_this = entry->get_text();
                entry->set_text(text_inherit);
            }
            else {
                entry->set_text(text_this);
            }
        });

        property_can_inherit().signal_changed().connect([this] {
            button->set_sensitive(property_can_inherit());
            if (!property_can_inherit()) {
                property_inherit() = false;
            }
        });
    }
    void set_text_inherit(const std::string &s)
    {
        text_inherit = s;
        if (property_inherit()) {
            entry->set_text(text_inherit);
        }
    }

    void set_text_this(const std::string &s)
    {
        text_this = s;
        if (!property_inherit()) {
            entry->set_text(text_this);
        }
    }

    std::pair<bool, std::string> get_as_pair()
    {
        if (property_inherit()) {
            return {true, text_this};
        }
        else {
            return {false, entry->get_text()};
        }
    }

    Gtk::Entry *entry = nullptr;
    Gtk::ToggleButton *button = nullptr;


    Glib::PropertyProxy<bool> property_inherit()
    {
        return p_property_inherit.get_proxy();
    }
    Glib::PropertyProxy<bool> property_can_inherit()
    {
        return p_property_can_inherit.get_proxy();
    }

private:
    Glib::Property<bool> p_property_inherit;
    Glib::Property<bool> p_property_can_inherit;
    Glib::RefPtr<Glib::Binding> inh_binding;
    std::string text_inherit;
    std::string text_this;
};

class OrderableMPNEditor : public Gtk::Box, public Changeable {
public:
    OrderableMPNEditor(Part &p, const UUID &uu) : Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 0), part(p), uuid(uu)
    {
        get_style_context()->add_class("linked");
        entry = Gtk::manage(new Gtk::Entry);
        entry->show();
        entry->set_text(part.orderable_MPNs.at(uuid));
        entry->signal_changed().connect([this] { s_signal_changed.emit(); });
        entry_add_sanitizer(entry);
        pack_start(*entry, true, true, 0);

        auto bu = Gtk::manage(new Gtk::Button());
        bu->set_image_from_icon_name("list-remove-symbolic");
        bu->show();
        pack_start(*bu, false, false, 0);
        bu->signal_clicked().connect([this] {
            part.orderable_MPNs.erase(uuid);
            s_signal_changed.emit();
            delete this;
        });
    }
    std::string get_MPN()
    {
        return entry->get_text();
    }
    const UUID &get_uuid() const
    {
        return uuid;
    }

    void focus()
    {
        entry->grab_focus();
    }

private:
    Part &part;
    UUID uuid;
    Gtk::Entry *entry = nullptr;
};

class FlagEditor : public Gtk::Box, public Changeable {
public:
    FlagEditor() : Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 0)
    {
        get_style_context()->add_class("linked");
        static const std::vector<std::pair<Part::FlagState, std::string>> states = {
                {Part::FlagState::SET, "Yes"},
                {Part::FlagState::CLEAR, "No"},
                {Part::FlagState::INHERIT, "Inherit"},
        };
        Gtk::RadioButton *group = nullptr;
        for (const auto &[st, name] : states) {
            auto bu = Gtk::manage(new Gtk::RadioButton(name));
            buttons.emplace(st, bu);
            bu->set_mode(false);
            if (group)
                bu->join_group(*group);
            else
                group = bu;
            bu->show();
            pack_start(*bu, false, false, 0);
            bu->signal_toggled().connect([this, bu] {
                if (bu->get_active()) {
                    s_signal_changed.emit();
                }
            });
        }
    }

    Part::FlagState get_state() const
    {
        for (auto [st, button] : buttons) {
            if (button->get_active())
                return st;
        }
        return Part::FlagState::CLEAR;
    }

    void set_state(Part::FlagState st)
    {
        buttons.at(st)->set_active(true);
    }

    void set_can_inherit(bool inh)
    {
        buttons.at(Part::FlagState::INHERIT)->set_sensitive(inh);
    }

private:
    std::map<Part::FlagState, Gtk::RadioButton *> buttons;
};

PartEditor::PartEditor(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x, const std::string &fn, IPool &po,
                       PoolParametric &pp)
    : Gtk::Box(cobject), PoolEditorBase(fn, po),
      part(filename.size() ? Part::new_from_file(filename, pool) : Part(UUID::random())), pool_parametric(pp)
{
    history_append("init");
    auto add_entry = [x](const char *name) {
        Gtk::Box *t;
        x->get_widget(name, t);
        auto e = Gtk::manage(new EntryWithInheritance);
        e->show();
        t->pack_start(*e, true, true, 0);
        return e;
    };

    x->get_widget("entity_label", w_entity_label);
    x->get_widget("package_label", w_package_label);
    x->get_widget("change_package_button", w_change_package_button);
    x->get_widget("set_base_menu_item", w_set_base_menu_item);
    x->get_widget("clear_base_menu_item", w_clear_base_menu_item);
    x->get_widget("create_base_menu_item", w_create_base_menu_item);
    {
        Gtk::Box *model_box;
        x->get_widget("model_box", model_box);
        w_model_combo = Gtk::manage(new std::remove_pointer_t<decltype(w_model_combo)>);
        w_model_combo->get_cr_text().property_ellipsize() = Pango::ELLIPSIZE_END;
        w_model_combo->show();
        model_box->pack_start(*w_model_combo, true, true, 0);
    }
    x->get_widget("model_inherit", w_model_inherit);
    x->get_widget("base_label", w_base_label);
    {
        Gtk::Box *tag_box;
        x->get_widget("tags", tag_box);
        w_tags = Gtk::manage(new TagEntry(pool, ObjectType::PART, true));
        w_tags->show();
        tag_box->pack_start(*w_tags, true, true, 0);
    }
    x->get_widget("tags_inherit", w_tags_inherit);
    x->get_widget("tags_inherited", w_tags_inherited);
    x->get_widget("tv_pins", w_tv_pins);
    x->get_widget("tv_pads", w_tv_pads);
    x->get_widget("button_map", w_button_map);
    x->get_widget("button_unmap", w_button_unmap);
    x->get_widget("button_automap", w_button_automap);
    x->get_widget("button_select_pin", w_button_select_pin);
    x->get_widget("button_select_pads", w_button_select_pads);
    x->get_widget("button_copy_from_other", w_button_copy_from_other);
    x->get_widget("pin_stat", w_pin_stat);
    x->get_widget("pad_stat", w_pad_stat);
    x->get_widget("parametric_box", w_parametric_box);
    x->get_widget("parametric_table_combo", w_parametric_table_combo);
    x->get_widget("orderable_MPNs_label", w_orderable_MPNs_label);
    x->get_widget("orderable_MPNs_box", w_orderable_MPNs_box);
    x->get_widget("orderable_MPNs_add_button", w_orderable_MPNs_add_button);
    x->get_widget("flags_grid", w_flags_grid);
    x->get_widget("flags_label", w_flags_label);
    x->get_widget("override_prefix_inherit_button", w_override_prefix_inherit_button);
    x->get_widget("override_prefix_no_button", w_override_prefix_no_button);
    x->get_widget("override_prefix_yes_button", w_override_prefix_yes_button);
    x->get_widget("override_prefix_entry", w_override_prefix_entry);
    sg_parametric_label = decltype(sg_parametric_label)::cast_dynamic(x->get_object("sg_parametric_label"));

    override_prefix_radio_buttons = {{Part::OverridePrefix::NO, w_override_prefix_no_button},
                                     {Part::OverridePrefix::INHERIT, w_override_prefix_inherit_button},
                                     {Part::OverridePrefix::YES, w_override_prefix_yes_button}};

    label_make_item_link(*w_entity_label, ObjectType::ENTITY);
    label_make_item_link(*w_base_label, ObjectType::PART);
    label_make_item_link(*w_package_label, ObjectType::PACKAGE);

    w_mpn = add_entry("part_mpn_box");
    w_value = add_entry("part_value_box");
    w_manufacturer = add_entry("part_manufacturer_box");
    w_manufacturer->entry->set_completion(create_pool_manufacturer_completion(pool));
    w_datasheet = add_entry("part_datasheet_box");
    w_description = add_entry("part_description_box");

    attr_editors.emplace(horizon::Part::Attribute::MPN, w_mpn);
    attr_editors.emplace(horizon::Part::Attribute::VALUE, w_value);
    attr_editors.emplace(horizon::Part::Attribute::MANUFACTURER, w_manufacturer);
    attr_editors.emplace(horizon::Part::Attribute::DESCRIPTION, w_description);
    attr_editors.emplace(horizon::Part::Attribute::DATASHEET, w_datasheet);


    for (auto &it : attr_editors) {
        it.second->entry->signal_changed().connect([this, it] {
            if (is_loading())
                return;
            part.attributes[it.first] = it.second->get_as_pair();
            set_needs_save();
        });
        it.second->button->signal_toggled().connect([this, it] {
            if (is_loading())
                return;
            part.attributes[it.first] = it.second->get_as_pair();
            set_needs_save();
        });
    }

    w_change_package_button->signal_clicked().connect(sigc::mem_fun(*this, &PartEditor::change_package));
    w_set_base_menu_item->signal_activate().connect(sigc::mem_fun(*this, &PartEditor::change_base));
    w_clear_base_menu_item->signal_activate().connect(sigc::mem_fun(*this, &PartEditor::clear_base));
    w_create_base_menu_item->signal_activate().connect(sigc::mem_fun(*this, &PartEditor::create_base));


    w_tags_inherit->signal_toggled().connect([this] {
        if (is_loading())
            return;
        part.inherit_tags = w_tags_inherit->get_active();
        set_needs_save();
    });

    w_tags->signal_changed().connect([this] {
        if (is_loading())
            return;
        part.tags = w_tags->get_tags();
        set_needs_save();
    });

    pin_store = Gtk::ListStore::create(pin_list_columns);
    pin_store->set_sort_func(pin_list_columns.pin_name,
                             [this](const Gtk::TreeModel::iterator &ia, const Gtk::TreeModel::iterator &ib) {
                                 Gtk::TreeModel::Row ra = *ia;
                                 Gtk::TreeModel::Row rb = *ib;
                                 Glib::ustring a = ra[pin_list_columns.pin_name];
                                 Glib::ustring b = rb[pin_list_columns.pin_name];
                                 return strcmp_natural(a, b);
                             });
    w_tv_pins->set_model(pin_store);

    w_tv_pins->append_column("Gate", pin_list_columns.gate_name);
    w_tv_pins->append_column("Pin", pin_list_columns.pin_name);
    {
        auto cr = Gtk::manage(new Gtk::CellRendererPixbuf());
        cr->property_icon_name() = "object-select-symbolic";
        cr->property_xalign() = 0;
        auto tvc = Gtk::manage(new Gtk::TreeViewColumn("Mapped", *cr));
        tvc->add_attribute(cr->property_visible(), pin_list_columns.mapped);
        w_tv_pins->append_column(*tvc);
    }

    w_tv_pins->get_column(0)->set_sort_column(pin_list_columns.gate_name);
    w_tv_pins->get_column(1)->set_sort_column(pin_list_columns.pin_name);

    pin_store->set_sort_column(pin_list_columns.pin_name, Gtk::SORT_ASCENDING);
    Glib::signal_timeout().connect_once(
            sigc::track_obj([this] { pin_store->set_sort_column(pin_list_columns.gate_name, Gtk::SORT_ASCENDING); },
                            *this),
            10);

    pad_store = Gtk::ListStore::create(pad_list_columns);
    pad_store->set_sort_func(pad_list_columns.pad_name,
                             [this](const Gtk::TreeModel::iterator &ia, const Gtk::TreeModel::iterator &ib) {
                                 Gtk::TreeModel::Row ra = *ia;
                                 Gtk::TreeModel::Row rb = *ib;
                                 Glib::ustring a = ra[pad_list_columns.pad_name];
                                 Glib::ustring b = rb[pad_list_columns.pad_name];
                                 return strcmp_natural(a, b);
                             });
    w_tv_pads->set_model(pad_store);
    w_tv_pads->append_column("Pad", pad_list_columns.pad_name);

    w_tv_pads->append_column("Gate", pad_list_columns.gate_name);
    w_tv_pads->append_column("Pin", pad_list_columns.pin_name);
    w_tv_pads->get_column(0)->set_sort_column(pad_list_columns.pad_name);

    pad_store->set_sort_column(pad_list_columns.pad_name, Gtk::SORT_ASCENDING);

    w_tv_pads->get_selection()->signal_changed().connect(sigc::mem_fun(*this, &PartEditor::update_map_buttons));
    w_tv_pins->get_selection()->signal_changed().connect(sigc::mem_fun(*this, &PartEditor::update_map_buttons));

    w_button_unmap->signal_clicked().connect([this] {
        auto sel = w_tv_pads->get_selection();
        for (auto &path : sel->get_selected_rows()) {
            auto it = pad_store->get_iter(path);
            Gtk::TreeModel::Row row = *it;
            row[pad_list_columns.gate_name] = "";
            row[pad_list_columns.pin_name] = "";
            row[pad_list_columns.pin_uuid] = UUID();
            row[pad_list_columns.gate_uuid] = UUID();
        }
        update_mapped();
        update_pad_map();
        set_needs_save();
    });

    w_tv_pins->signal_row_activated().connect([this](const Gtk::TreeModel::Path &path, Gtk::TreeViewColumn *col) {
        auto it_pin = pin_store->get_iter(path);
        map_pin(it_pin);
    });

    w_button_map->signal_clicked().connect([this] {
        auto pin_sel = w_tv_pins->get_selection();
        auto it_pin = pin_sel->get_selected();
        map_pin(it_pin);
    });

    w_button_automap->signal_clicked().connect([this] {
        auto sel = w_tv_pads->get_selection();
        for (auto &path : sel->get_selected_rows()) {
            auto it = pad_store->get_iter(path);
            Gtk::TreeModel::Row row = *it;
            Glib::ustring pad_name = row[pad_list_columns.pad_name];
            for (const auto &it_pin : pin_store->children()) {
                Gtk::TreeModel::Row row_pin = *it_pin;
                Glib::ustring pin_name = row_pin[pin_list_columns.pin_name];
                if (pin_name == pad_name) {
                    row[pad_list_columns.gate_name] = row_pin.get_value(pin_list_columns.gate_name);
                    row[pad_list_columns.pin_name] = row_pin.get_value(pin_list_columns.pin_name);
                    row[pad_list_columns.pin_uuid] = row_pin.get_value(pin_list_columns.pin_uuid);
                    row[pad_list_columns.gate_uuid] = row_pin.get_value(pin_list_columns.gate_uuid);
                    break;
                }
            }
        }
        update_mapped();
        update_pad_map();
        set_needs_save();
    });

    w_button_select_pin->signal_clicked().connect([this] {
        auto sel = w_tv_pads->get_selection();
        auto rows = sel->get_selected_rows();
        if (rows.size() != 1)
            return;
        auto it_pad = pad_store->get_iter(rows.front());
        Gtk::TreeModel::Row row_pad = *it_pad;
        auto gate_uuid = row_pad[pad_list_columns.gate_uuid];
        auto pin_uuid = row_pad[pad_list_columns.pin_uuid];
        for (auto &it : pin_store->children()) {
            Gtk::TreeModel::Row row = *it;
            if (row[pin_list_columns.gate_uuid] == gate_uuid && row[pin_list_columns.pin_uuid] == pin_uuid) {
                w_tv_pins->get_selection()->select(it);
                tree_view_scroll_to_selection(w_tv_pins);
                break;
            }
        }
    });

    w_button_select_pads->signal_clicked().connect([this] {
        auto sel = w_tv_pins->get_selection();
        auto rows = sel->get_selected_rows();
        if (rows.size() != 1)
            return;
        auto it_pin = pin_store->get_iter(rows.front());
        Gtk::TreeModel::Row row_pin = *it_pin;
        auto gate_uuid = row_pin[pin_list_columns.gate_uuid];
        auto pin_uuid = row_pin[pin_list_columns.pin_uuid];

        w_tv_pads->get_selection()->unselect_all();
        for (auto &it : pad_store->children()) {
            Gtk::TreeModel::Row row = *it;
            if (row[pad_list_columns.gate_uuid] == gate_uuid && row[pad_list_columns.pin_uuid] == pin_uuid) {
                w_tv_pads->get_selection()->select(it);
            }
        }
        tree_view_scroll_to_selection(w_tv_pads);
    });
    w_button_copy_from_other->signal_clicked().connect(sigc::mem_fun(*this, &PartEditor::copy_from_other_part));
    w_model_inherit->signal_toggled().connect([this] {
        if (is_loading())
            return;
        part.inherit_model = w_model_inherit->get_active();
        set_needs_save();
        update_model_inherit();
    });

    w_model_combo->signal_changed().connect([this] {
        if (is_loading())
            return;
        part.model = w_model_combo->get_active_key();
        set_needs_save();
    });

    {
        namespace rv = ranges::views;
        const auto maxlen = ranges::max(ranges::views::concat(
                ranges::views::single(0),
                pool_parametric.get_tables() | rv::transform([](const auto &x) { return x.second.columns; }) | rv::join
                        | rv::transform([](const auto &x) { return x.display_name.size(); })));
        Gtk::Label *la;
        x->get_widget("label_table", la);
        la->set_width_chars(maxlen);
    }

    w_parametric_table_combo->append("", "None");
    for (const auto &it : pool_parametric.get_tables()) {
        w_parametric_table_combo->append(it.first, it.second.display_name);
    }
    w_parametric_table_combo->signal_changed().connect([this] {
        if (is_loading())
            return;
        w_parametric_table_combo->grab_focus();
        update_parametric_editor();
        set_needs_save();
    });

    w_orderable_MPNs_add_button->signal_clicked().connect([this] {
        auto uu = UUID::random();
        part.orderable_MPNs.emplace(uu, "");
        auto ed = create_orderable_MPN_editor(uu);
        ed->focus();
        set_needs_save();
        update_orderable_MPNs_label();
    });

    {
        static const std::map<Part::Flag, std::string> flag_names = {
                {Part::Flag::BASE_PART, "Base part"},
                {Part::Flag::EXCLUDE_BOM, "Exclude from BOM"},
                {Part::Flag::EXCLUDE_PNP, "Exclude from Pick&Place"},
        };
        int top = 0;
        for (const auto &[fl, name] : flag_names) {
            auto ed = Gtk::manage(new FlagEditor());
            flag_editors.emplace(fl, ed);
            const auto fl2 = fl;
            ed->signal_changed().connect([this, ed, fl2] {
                if (is_loading())
                    return;
                part.flags.at(fl2) = ed->get_state();
                set_needs_save();
                update_flags_label();
            });
            grid_attach_label_and_widget(w_flags_grid, name, ed, top);
        }
    }


    for (auto [ov, rb] : override_prefix_radio_buttons) {
        auto ov2 = ov;
        auto &rb2 = *rb;
        rb2.signal_toggled().connect([this, &rb2, ov2] {
            if (is_loading())
                return;
            if (rb2.get_active()) {
                part.override_prefix = ov2;
                update_prefix_entry();
                set_needs_save();
            }
        });
    }

    override_prefix_entry_connection = w_override_prefix_entry->signal_changed().connect([this] {
        if (part.override_prefix == Part::OverridePrefix::YES) {
            if (is_loading())
                return;
            part.prefix = w_override_prefix_entry->get_text();
            set_needs_save();
        }
    });

    load();
}

void PartEditor::load()
{
    auto loading_setter = set_loading();
    for (auto &[attr, entry] : attr_editors) {
        entry->property_can_inherit() = part.base.get();
        entry->set_text_this(part.attributes.at(attr).second);
        if (part.base) {
            entry->set_text_inherit(part.base->attributes.at(attr).second);
            entry->property_inherit() = part.attributes.at(attr).first;
        }
        else {
            entry->property_inherit() = false;
        }
    }
    update_entries();

    w_tags_inherit->set_active(part.inherit_tags);
    w_tags->set_tags(part.tags);
    w_change_package_button->set_sensitive(!part.base);
    w_clear_base_menu_item->set_sensitive(part.base.get());


    if (part.base) {
        w_model_inherit->set_active(part.inherit_model);
        w_model_inherit->set_sensitive(true);
    }
    else {
        w_model_inherit->set_active(false);
        w_model_inherit->set_sensitive(false);
    }
    update_model_inherit();


    update_treeview();

    update_map_buttons();

    update_mapped();
    populate_models();
    w_model_combo->set_active_key(part.model);


    if (part.parametric.count("table")) {
        std::string tab = part.parametric.at("table");
        parametric_data[tab] = part.parametric;
        if (pool_parametric.get_tables().count(tab)) {
            w_parametric_table_combo->set_active_id(tab);
        }
        else {
            w_parametric_table_combo->set_active_id("");
        }
    }
    else {
        w_parametric_table_combo->set_active_id("");
    }
    update_parametric_editor();

    for (const auto &it : part.orderable_MPNs) {
        create_orderable_MPN_editor(it.first);
    }
    update_orderable_MPNs_label();

    override_prefix_radio_buttons.at(part.override_prefix)->set_active(true);

    w_override_prefix_inherit_button->set_sensitive(part.base.get());
    update_prefix_entry();

    for (auto [fl, ed] : flag_editors) {
        ed->set_can_inherit(part.base.get());
        ed->set_state(part.flags.at(fl));
    }
    update_flags_label();
}

class OrderableMPNEditor *PartEditor::create_orderable_MPN_editor(const UUID &uu)
{
    auto ed = Gtk::manage(new OrderableMPNEditor(part, uu));
    w_orderable_MPNs_box->pack_start(*ed, false, false, 0);
    ed->signal_changed().connect([this, ed] {
        if (part.orderable_MPNs.count(ed->get_uuid()))
            part.orderable_MPNs.at(ed->get_uuid()) = ed->get_MPN();
        set_needs_save();
        update_orderable_MPNs_label();
    });
    ed->show();
    return ed;
}

void PartEditor::update_map_buttons()
{
    if (part.base) {
        w_button_map->set_sensitive(false);
        w_button_unmap->set_sensitive(false);
        w_button_automap->set_sensitive(false);
        w_button_copy_from_other->set_sensitive(false);
    }
    else {
        bool can_map =
                w_tv_pads->get_selection()->count_selected_rows() && w_tv_pins->get_selection()->count_selected_rows();
        w_button_map->set_sensitive(can_map);
        w_button_unmap->set_sensitive(w_tv_pads->get_selection()->count_selected_rows());
        w_button_automap->set_sensitive(w_tv_pads->get_selection()->count_selected_rows());
        w_button_copy_from_other->set_sensitive(true);
    }
    w_button_select_pads->set_sensitive(w_tv_pins->get_selection()->count_selected_rows());
    w_button_select_pin->set_sensitive(w_tv_pads->get_selection()->count_selected_rows() == 1);
}

void PartEditor::update_orderable_MPNs_label()
{
    std::string s;
    for (const auto &[uu, mpn] : part.orderable_MPNs) {
        s += mpn + ", ";
    }
    if (s.size()) {
        s.pop_back();
        s.pop_back();
    }
    else {
        s = "(None)";
    }
    w_orderable_MPNs_label->set_text(s);
}

void PartEditor::update_flags_label()
{
    std::string s;
    static const std::map<Part::Flag, std::string> flag_names = {
            {Part::Flag::BASE_PART, "Base part"},
            {Part::Flag::EXCLUDE_BOM, "No BOM"},
            {Part::Flag::EXCLUDE_PNP, "No Pick&Place"},
    };
    for (const auto &[fl, name] : flag_names) {
        if (part.get_flag(fl)) {
            s += name;
            s += ", ";
        }
    }
    if (s.size()) {
        s.pop_back();
        s.pop_back();
    }
    else {
        s = "(None)";
    }
    w_flags_label->set_text(s);
}

void PartEditor::update_prefix_entry()
{
    override_prefix_entry_connection.block();
    switch (part.override_prefix) {
    case Part::OverridePrefix::NO:
        w_override_prefix_entry->set_sensitive(false);
        w_override_prefix_entry->set_text(part.entity->prefix);
        break;

    case Part::OverridePrefix::INHERIT:
        w_override_prefix_entry->set_sensitive(false);
        w_override_prefix_entry->set_text(part.base->get_prefix());
        break;

    case Part::OverridePrefix::YES:
        w_override_prefix_entry->set_sensitive(true);
        w_override_prefix_entry->set_text(part.prefix);
        break;
    }
    override_prefix_entry_connection.unblock();
}

void PartEditor::map_pin(Gtk::TreeModel::iterator it_pin)
{
    auto pin_sel = w_tv_pins->get_selection();
    if (it_pin) {
        Gtk::TreeModel::Row row_pin = *it_pin;
        auto sel = w_tv_pads->get_selection();
        for (auto &path : sel->get_selected_rows()) {
            auto it = pad_store->get_iter(path);
            Gtk::TreeModel::Row row = *it;
            row[pad_list_columns.gate_name] = row_pin.get_value(pin_list_columns.gate_name);
            row[pad_list_columns.pin_name] = row_pin.get_value(pin_list_columns.pin_name);
            row[pad_list_columns.pin_uuid] = row_pin.get_value(pin_list_columns.pin_uuid);
            row[pad_list_columns.gate_uuid] = row_pin.get_value(pin_list_columns.gate_uuid);
        }
        if (++it_pin) {
            pin_sel->select(it_pin);
        }
        if (sel->count_selected_rows() == 1) {
            auto it_pad = pad_store->get_iter(sel->get_selected_rows().at(0));
            sel->unselect(it_pad);
            if (++it_pad) {
                sel->select(it_pad);
            }
        }
        update_mapped();
        update_pad_map();
        set_needs_save();
    }
}

void PartEditor::update_model_inherit()
{
    if (!part.base)
        return;
    auto active = w_model_inherit->get_active();
    if (active) {
        w_model_combo->set_active_key(part.base->model);
    }
    w_model_combo->set_sensitive(!active);
}

static std::string append_with_slash(const std::string &s)
{
    if (s.size())
        return " / " + s;
    else
        return "";
}

void PartEditor::update_entries()
{
    if (part.base) {
        w_base_label->set_markup(
                "<a href=\"" + (std::string)part.base->uuid + "\">"
                + Glib::Markup::escape_text(part.base->get_MPN() + append_with_slash(part.base->get_manufacturer()))
                + "</a>");
        w_entity_label->set_markup("<a href=\"" + (std::string)part.base->entity->uuid + "\">"
                                   + Glib::Markup::escape_text(part.base->entity->name
                                                               + append_with_slash(part.base->entity->manufacturer))
                                   + "</a>");
        w_package_label->set_markup("<a href=\"" + (std::string)part.base->package->uuid + "\">"
                                    + Glib::Markup::escape_text(part.base->package->name
                                                                + append_with_slash(part.base->package->manufacturer))
                                    + "</a>");
    }
    else {
        w_base_label->set_text("none");
        w_entity_label->set_markup(
                "<a href=\"" + (std::string)part.entity->uuid + "\">"
                + Glib::Markup::escape_text(part.entity->name + append_with_slash(part.entity->manufacturer)) + "</a>");
        w_package_label->set_markup(
                "<a href=\"" + (std::string)part.package->uuid + "\">"
                + Glib::Markup::escape_text(part.package->name + append_with_slash(part.package->manufacturer))
                + "</a>");
    }

    if (part.base) {
        std::stringstream s;
        auto tags_from_base = part.base->get_tags();
        std::copy(tags_from_base.begin(), tags_from_base.end(), std::ostream_iterator<std::string>(s, " "));
        w_tags_inherited->set_text(s.str());
    }
    else {
        w_tags_inherited->set_text("");
    }

    w_tags_inherit->set_sensitive(part.base.get());
}

void PartEditor::set_package(std::shared_ptr<const Package> package)
{
    const auto old_package = part.package;
    part.package = package;
    part.model = part.package->default_model;
    const auto old_pad_map = part.pad_map;
    part.pad_map.clear();
    for (const auto &[uu, pad] : part.package->pads) {
        const auto &pad_name = pad.name;
        if (auto it = find_if_ptr(old_pad_map, [&pad_name, &old_package](const auto &x) {
                return old_package->pads.at(x.first).name == pad_name;
            })) {
            part.pad_map.emplace(uu, it->second);
        }
    }
}

void PartEditor::change_package()
{
    auto top = dynamic_cast<Gtk::Window *>(get_ancestor(GTK_TYPE_WINDOW));
    PoolBrowserDialog dia(top, ObjectType::PACKAGE, pool);
    if (dia.run() == Gtk::RESPONSE_OK) {
        set_package(pool.get_package(dia.get_browser().get_selected()));
        load();
        set_needs_save();
    }
}

bool PartEditor::check_base(const UUID &new_base)
{
    bool r = true;
    auto &db = pool.get_db();
    db.execute("DROP VIEW IF EXISTS part_deps");
    {
        db.execute(
                        "CREATE TEMPORARY view part_deps AS SELECT uuid, dep_uuid FROM dependencies WHERE "
                        "type = 'part' AND dep_type = 'part' AND uuid != '" + (std::string)part.uuid + "' "
                        "UNION SELECT '" + (std::string)part.uuid + "', '" + (std::string) new_base+ "'");
    }
    {
        SQLite::Query q(db,
                        "WITH FindRoot AS ( "
                        "SELECT uuid, dep_uuid, uuid as path, 0 AS Distance "
                        "FROM part_deps WHERE uuid = $part "
                        "UNION ALL "
                        "SELECT C.uuid, P.dep_uuid, C.path || ' > ' || P.uuid, C.Distance + 1 "
                        "FROM part_deps AS P "
                        " JOIN FindRoot AS C "
                        " ON C.dep_uuid = P.uuid AND P.dep_uuid != P.uuid AND C.dep_uuid != C.uuid "
                        ") "
                        "SELECT * "
                        "FROM FindRoot AS R "
                        "WHERE R.uuid = R.dep_uuid AND R.Distance > 0");
        q.bind("$part", part.uuid);
        while (q.step()) {
            r = false;
            break;
        }
    }

    db.execute("DROP VIEW IF EXISTS part_deps");
    return r;
}

void PartEditor::change_base()
{
    auto top = dynamic_cast<Gtk::Window *>(get_ancestor(GTK_TYPE_WINDOW));
    PoolBrowserDialog dia(top, ObjectType::PART, pool);
    {
        auto &br = dynamic_cast<PoolBrowserPart &>(dia.get_browser());
        br.set_entity_uuid(part.entity->uuid);
        br.search();
    }
    while (dia.run() == Gtk::RESPONSE_OK) {
        const auto selected_part_uuid = dia.get_browser().get_selected();
        if (part.base && part.base->uuid == selected_part_uuid)
            return;
        if (selected_part_uuid == part.uuid || !check_base(selected_part_uuid)) {
            Gtk::MessageDialog md(dia, "Can't set as base part", false /* use_markup */, Gtk::MESSAGE_ERROR,
                                  Gtk::BUTTONS_OK);
            md.set_secondary_text("Cyclic dependency");
            md.run();
            continue;
        }
        auto new_base = pool.get_part(selected_part_uuid);
        set_base(new_base);
        load();
        set_needs_save();
        return;
    }
}

void PartEditor::set_base(std::shared_ptr<const Part> new_base)
{
    auto old_base = part.base;
    part.base = new_base;
    if (new_base) {
        set_package(part.base->package);
        part.pad_map = part.base->pad_map;
    }
    else {
        for (auto &[attr, v] : part.attributes) {
            v.first = false;
        }
        for (auto &[flag, st] : part.flags) {
            if (st == Part::FlagState::INHERIT && old_base)
                st = old_base->get_flag(flag) ? Part::FlagState::SET : Part::FlagState::CLEAR;
        }
        if (part.inherit_tags) {
            auto inherited_tags = old_base->get_tags();
            part.tags.insert(inherited_tags.begin(), inherited_tags.end());
        }
        if (part.override_prefix == Part::OverridePrefix::INHERIT)
            part.override_prefix = old_base->get_override_prefix();
        if (part.override_prefix == Part::OverridePrefix::YES)
            part.prefix = old_base->get_prefix();
        part.inherit_model = false;
        part.inherit_tags = false;
    }
}

void PartEditor::clear_base()
{
    set_base(nullptr);
    load();
    set_needs_save();
}

void PartEditor::create_base()
{
    auto top = dynamic_cast<Gtk::Window *>(get_ancestor(GTK_TYPE_WINDOW));

    GtkFileChooserNative *native =
            gtk_file_chooser_native_new("Save", top->gobj(), GTK_FILE_CHOOSER_ACTION_SAVE, "_Save", "_Cancel");
    auto chooser = Glib::wrap(GTK_FILE_CHOOSER(native));
    chooser->set_do_overwrite_confirmation(true);


    chooser->set_current_folder(Glib::path_get_dirname(filename));

    run_native_filechooser_with_retry(chooser, "Error saving Part", [this, chooser] {
        std::string fn = append_dot_json(chooser->get_filename());
        pool.check_filename_throw(ObjectType::PART, fn);
        Part new_part = part;
        new_part.flags.at(Part::Flag::BASE_PART) = Part::FlagState::SET;
        auto &mpn = new_part.attributes.at(Part::Attribute::MPN);
        if (!mpn.first)
            mpn.second = mpn.second + " (Base part)";
        new_part.uuid = UUID::random();
        save_json_to_file(fn, new_part.serialize());
        s_signal_extra_file_saved.emit(fn);
        pending_base_part = new_part.uuid;
    });
}

static void part_update_refs_or_0(Part &part, IPool &pool)
{
    part.entity = pool.get_entity(part.entity->uuid);
    part.package = pool.get_package(part.package->uuid);
    if (part.base)
        part.base = pool.get_part(part.base->uuid);
    for (auto &[pad, it] : part.pad_map) {
        it.gate.update(part.entity->gates);
        if (it.gate)
            it.pin.update(it.gate->unit->pins);
        else
            it.pin.ptr = nullptr;
    }
}

void PartEditor::reload()
{
    part_update_refs_or_0(part, pool);
    update_entries();
    update_treeview();
    update_mapped();
    if (pending_base_part) {
        std::shared_ptr<const Part> new_base;
        try {
            new_base = pool.get_part(pending_base_part);
        }
        catch (...) {
            // doesn't seem to exist yet...
        }
        if (new_base) {
            set_base(new_base);
            s_signal_open_item.emit(ObjectType::PART, new_base->uuid);
            for (auto &[attr, value] : part.attributes) {
                value.first = true;
            }
            part.tags.clear();
            part.inherit_tags = true;
            part.inherit_model = true;
            if (new_base->override_prefix != Part::OverridePrefix::NO)
                part.override_prefix = Part::OverridePrefix::INHERIT;
            pending_base_part = UUID();
            load();
            set_needs_save();
        }
    }
}

void PartEditor::update_treeview()
{
    pin_store->freeze_notify();
    pad_store->freeze_notify();

    pin_store->clear();
    pad_store->clear();

    for (const auto &it_gate : part.entity->gates) {
        for (const auto &it_pin : it_gate.second.unit->pins) {
            Gtk::TreeModel::Row row = *(pin_store->append());
            row[pin_list_columns.gate_uuid] = it_gate.first;
            row[pin_list_columns.gate_name] = it_gate.second.name;
            row[pin_list_columns.pin_uuid] = it_pin.first;
            row[pin_list_columns.pin_name] = it_pin.second.primary_name;
        }
    }

    for (const auto &it : part.package->pads) {
        if (it.second.pool_padstack->type != Padstack::Type::MECHANICAL) {
            Gtk::TreeModel::Row row = *(pad_store->append());
            row[pad_list_columns.pad_uuid] = it.first;
            row[pad_list_columns.pad_name] = it.second.name;
            if (part.pad_map.count(it.first)) {
                const auto &x = part.pad_map.at(it.first);
                if (x.gate && x.pin) {
                    row[pad_list_columns.gate_uuid] = x.gate->uuid;
                    row[pad_list_columns.gate_name] = x.gate->name;
                    row[pad_list_columns.pin_uuid] = x.pin->uuid;
                    row[pad_list_columns.pin_name] = x.pin->primary_name;
                }
            }
        }
    }

    pad_store->thaw_notify();
    pin_store->thaw_notify();
}

void PartEditor::update_mapped()
{
    std::set<std::pair<UUID, UUID>> pins_mapped;
    int n_pads_not_mapped = 0;
    for (const auto &it : pad_store->children()) {
        if (it[pad_list_columns.gate_uuid] != UUID()) {
            pins_mapped.emplace(it[pad_list_columns.gate_uuid], it[pad_list_columns.pin_uuid]);
        }
        else {
            n_pads_not_mapped++;
        }
    }
    for (auto &it : pin_store->children()) {
        if (pins_mapped.count({it[pin_list_columns.gate_uuid], it[pin_list_columns.pin_uuid]})) {
            it[pin_list_columns.mapped] = true;
        }
        else {
            it[pin_list_columns.mapped] = false;
        }
    }
    w_pin_stat->set_text(std::to_string(pin_store->children().size() - pins_mapped.size()) + " pins not mapped");
    w_pad_stat->set_text(std::to_string(n_pads_not_mapped) + " pads not mapped");
}

void PartEditor::update_pad_map()
{
    part.pad_map.clear();
    for (const auto &it : pad_store->children()) {
        if (it[pad_list_columns.gate_uuid] != UUID() && part.package->pads.count(it[pad_list_columns.pad_uuid])) {
            const horizon::Gate *gate = &part.entity->gates.at(it[pad_list_columns.gate_uuid]);
            const horizon::Pin *pin = &gate->unit->pins.at(it[pad_list_columns.pin_uuid]);
            part.pad_map.emplace(it[pad_list_columns.pad_uuid], Part::PadMapItem(gate, pin));
        }
    }
}

void PartEditor::populate_models()
{
    w_model_combo->remove_all();
    for (const auto &it : part.package->models) {
        w_model_combo->append((std::string)it.first, Glib::path_get_basename(it.second.filename));
    }
}

void PartEditor::update_parametric_editor()
{
    if (parametric_editor) {
        delete parametric_editor;
        parametric_editor = nullptr;
    }
    auto tab = w_parametric_table_combo->get_active_id();
    if (pool_parametric.get_tables().count(tab)) {
        auto ed = Gtk::manage(new ParametricEditor(pool_parametric, tab, sg_parametric_label));
        ed->show();
        w_parametric_box->pack_start(*ed, true, true, 0);
        if (parametric_data.count(tab)) {
            const auto &values = parametric_data.at(tab);
            ed->update(values);
            part.parametric = values;
        }
        else {
            part.parametric.clear();
            part.parametric.emplace("table", tab);
        }
        parametric_editor = ed;
        parametric_editor->signal_changed().connect([this] {
            auto values = parametric_editor->get_values();
            parametric_data[values.at("table")] = values;
            part.parametric = values;
            set_needs_save();
        });
    }
    else {
        part.parametric.clear();
    }
}

void PartEditor::copy_from_other_part()
{
    auto top = dynamic_cast<Gtk::Window *>(get_ancestor(GTK_TYPE_WINDOW));
    PoolBrowserDialog dia(top, ObjectType::PART, pool);
    auto &br = dynamic_cast<PoolBrowserPart &>(dia.get_browser());
    br.set_entity_uuid(part.entity->uuid);
    br.search();
    if (dia.run() == Gtk::RESPONSE_OK) {
        auto uu = br.get_selected();
        auto other_part = pool.get_part(uu);
        for (auto &it_pad : pad_store->children()) {
            Gtk::TreeModel::Row row_pad = *it_pad;
            if (row_pad.get_value(pad_list_columns.gate_uuid) == UUID()) { // only update unmapped pads
                std::string pad_name = row_pad.get_value(pad_list_columns.pad_name);
                // find other part mapping
                for (const auto &it_map_other : other_part->pad_map) {
                    auto pad_uu_other = it_map_other.first;
                    const auto &pad_name_other = other_part->package->pads.at(pad_uu_other).name;
                    if (pad_name_other == pad_name) { // found it
                        row_pad[pad_list_columns.gate_name] = it_map_other.second.gate->name;
                        row_pad[pad_list_columns.pin_name] = it_map_other.second.pin->primary_name;
                        row_pad[pad_list_columns.pin_uuid] = it_map_other.second.pin->uuid;
                        row_pad[pad_list_columns.gate_uuid] = it_map_other.second.gate->uuid;
                        break;
                    }
                }
            }
        }
        update_mapped();
        set_needs_save();
    }
}

void PartEditor::save_as(const std::string &fn)
{
    unset_needs_save();
    filename = fn;
    save_json_to_file(fn, part.serialize());
}

std::string PartEditor::get_name() const
{
    return part.get_MPN();
}

const UUID &PartEditor::get_uuid() const
{
    return part.uuid;
}

RulesCheckResult PartEditor::run_checks() const
{
    return check_part(part);
}

const FileVersion &PartEditor::get_version() const
{
    return part.version;
}

unsigned int PartEditor::get_required_version() const
{
    return part.get_required_version();
}

ObjectType PartEditor::get_type() const
{
    return ObjectType::PART;
}

class HistoryItemPart : public HistoryManager::HistoryItem {
public:
    HistoryItemPart(const Part &p, const std::string &cm) : HistoryManager::HistoryItem(cm), part(p)
    {
    }
    Part part;
};

std::unique_ptr<HistoryManager::HistoryItem> PartEditor::make_history_item(const std::string &comment)
{
    return std::make_unique<HistoryItemPart>(part, comment);
}

void PartEditor::history_load(const HistoryManager::HistoryItem &it)
{
    const auto &x = dynamic_cast<const HistoryItemPart &>(it);
    part = x.part;
    part_update_refs_or_0(part, pool);
    load();
}


PartEditor *PartEditor::create(const std::string &fn, IPool &po, PoolParametric &pp)
{
    PartEditor *w;
    Glib::RefPtr<Gtk::Builder> x = Gtk::Builder::create();
    x->add_from_resource("/org/horizon-eda/horizon/pool-prj-mgr/pool-mgr/editors/part_editor.ui");
    x->get_widget_derived("part_editor", w, fn, po, pp);
    w->reference();
    return w;
}
} // namespace horizon
