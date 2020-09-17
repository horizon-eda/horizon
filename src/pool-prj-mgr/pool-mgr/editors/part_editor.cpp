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

PartEditor::PartEditor(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x, Part &p, IPool &po,
                       PoolParametric &pp)
    : Gtk::Box(cobject), part(p), pool(po), pool_parametric(pp)
{

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
    x->get_widget("model_combo", w_model_combo);
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
    x->get_widget("copy_parametric_from_base", w_parametric_from_base);
    x->get_widget("orderable_MPNs_label", w_orderable_MPNs_label);
    x->get_widget("orderable_MPNs_box", w_orderable_MPNs_box);
    x->get_widget("orderable_MPNs_add_button", w_orderable_MPNs_add_button);
    w_parametric_from_base->hide();

    w_entity_label->set_track_visited_links(false);
    w_entity_label->signal_activate_link().connect(
            [this](const std::string &url) {
                s_signal_goto.emit(ObjectType::ENTITY, UUID(url));
                return true;
            },
            false);
    w_base_label->set_track_visited_links(false);
    w_base_label->signal_activate_link().connect(
            [this](const std::string &url) {
                s_signal_goto.emit(ObjectType::PART, UUID(url));
                return true;
            },
            false);
    w_package_label->set_track_visited_links(false);
    w_package_label->signal_activate_link().connect(
            [this](const std::string &url) {
                s_signal_goto.emit(ObjectType::PACKAGE, UUID(url));
                return true;
            },
            false);

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
        it.second->property_can_inherit() = part.base;
        it.second->set_text_this(part.attributes.at(it.first).second);
        if (part.base) {
            it.second->set_text_inherit(part.base->attributes.at(it.first).second);
            it.second->property_inherit() = part.attributes.at(it.first).first;
        }
        it.second->entry->signal_changed().connect([this, it] {
            part.attributes[it.first] = it.second->get_as_pair();
            set_needs_save();
        });
        it.second->button->signal_toggled().connect([this, it] {
            part.attributes[it.first] = it.second->get_as_pair();
            set_needs_save();
        });
    }

    update_entries();

    w_change_package_button->signal_clicked().connect(sigc::mem_fun(*this, &PartEditor::change_package));

    w_tags_inherit->set_active(part.inherit_tags);
    w_tags_inherit->signal_toggled().connect([this] { set_needs_save(); });

    w_tags->set_tags(part.tags);
    w_tags->signal_changed().connect([this] {
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

    update_treeview();

    update_map_buttons();
    w_tv_pads->get_selection()->signal_changed().connect(sigc::mem_fun(*this, &PartEditor::update_map_buttons));
    w_tv_pins->get_selection()->signal_changed().connect(sigc::mem_fun(*this, &PartEditor::update_map_buttons));
    w_change_package_button->set_sensitive(!part.base);

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

    update_mapped();
    populate_models();
    w_model_combo->set_active_id((std::string)part.model);

    if (part.base) {
        w_model_inherit->set_active(part.inherit_model);
        w_model_inherit->signal_toggled().connect([this] {
            set_needs_save();
            update_model_inherit();
        });
        update_model_inherit();
    }
    else {
        w_model_inherit->set_sensitive(false);
    }

    w_model_combo->signal_changed().connect([this] { set_needs_save(); });

    w_parametric_table_combo->append("", "None");
    for (const auto &it : pool_parametric.get_tables()) {
        w_parametric_table_combo->append(it.first, it.second.display_name);
    }
    w_parametric_table_combo->set_active_id("");
    if (part.parametric.count("table")) {
        std::string tab = part.parametric.at("table");
        if (pool_parametric.get_tables().count(tab)) {
            w_parametric_table_combo->set_active_id(tab);
        }
    }
    update_parametric_editor();
    w_parametric_table_combo->signal_changed().connect([this] {
        set_needs_save();
        update_parametric_editor();
    });
    w_parametric_from_base->set_sensitive(part.base);
    w_parametric_from_base->signal_clicked().connect([this] {
        if (part.base) {
            set_needs_save();
        }
    });

    w_orderable_MPNs_add_button->signal_clicked().connect([this] {
        auto uu = UUID::random();
        part.orderable_MPNs.emplace(uu, "");
        auto ed = create_orderable_MPN_editor(uu);
        ed->focus();
        set_needs_save();
        update_orderable_MPNs_label();
    });

    for (const auto &it : part.orderable_MPNs) {
        create_orderable_MPN_editor(it.first);
    }
    update_orderable_MPNs_label();
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
        s += Glib::Markup::escape_text(mpn) + ", ";
    }
    if (s.size()) {
        s.pop_back();
        s.pop_back();
    }
    else {
        s = "<i>no orderable MPNs defined</i>";
    }
    w_orderable_MPNs_label->set_markup(s);
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
        set_needs_save();
    }
}

void PartEditor::update_model_inherit()
{
    auto active = w_model_inherit->get_active();
    if (active) {
        w_model_combo->set_active_id((std::string)part.base->model);
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

    w_tags_inherit->set_sensitive(part.base);
}

void PartEditor::change_package()
{
    auto top = dynamic_cast<Gtk::Window *>(get_ancestor(GTK_TYPE_WINDOW));
    PoolBrowserDialog dia(top, ObjectType::PACKAGE, pool);
    if (dia.run() == Gtk::RESPONSE_OK) {
        set_needs_save();
        part.package = pool.get_package(dia.get_browser().get_selected());
        auto ch = pad_store->children();
        std::set<UUID> pads_exisiting;
        for (auto it = ch.begin(); it != ch.end();) {
            Gtk::TreeModel::Row row = *it;
            auto pad_name = row[pad_list_columns.pad_name];
            UUID pad_uuid;
            for (const auto &it_pad : part.package->pads) {
                if (it_pad.second.name == pad_name) {
                    pad_uuid = it_pad.second.uuid;
                    break;
                }
            }
            if (pad_uuid) {
                row[pad_list_columns.pad_uuid] = pad_uuid;
                pads_exisiting.insert(pad_uuid);
                it++;
            }
            else {
                pad_store->erase(it++);
            }
        }
        for (const auto &it : part.package->pads) {
            if (pads_exisiting.count(it.first) == 0) {
                Gtk::TreeModel::Row row = *(pad_store->append());
                row[pad_list_columns.pad_uuid] = it.first;
                row[pad_list_columns.pad_name] = it.second.name;
            }
        }

        update_entries();
        update_mapped();
        populate_models();
        w_model_combo->set_active_id((std::string)part.package->default_model);
        set_needs_save();
    }
}

void PartEditor::reload()
{
    part.update_refs(pool);
    update_entries();
}

void PartEditor::save()
{
    part.inherit_model = w_model_inherit->get_active();

    if (w_model_combo->get_active_row_number() != -1)
        part.model = UUID(w_model_combo->get_active_id());
    else
        part.model = UUID();

    part.inherit_tags = w_tags_inherit->get_active();

    if (parametric_editor) {
        part.parametric = parametric_editor->get_values();
    }
    else {
        part.parametric.clear();
    }

    PoolEditorInterface::save();
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
                row[pad_list_columns.gate_uuid] = x.gate->uuid;
                row[pad_list_columns.gate_name] = x.gate->name;
                row[pad_list_columns.pin_uuid] = x.pin->uuid;
                row[pad_list_columns.pin_name] = x.pin->primary_name;
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
    auto chs = w_parametric_box->get_children();
    if (chs.size()) {
        delete chs.back();
    }
    parametric_editor = nullptr;
    auto tab = w_parametric_table_combo->get_active_id();
    if (pool_parametric.get_tables().count(tab)) {
        auto ed = Gtk::manage(new ParametricEditor(pool_parametric, tab));
        ed->show();
        w_parametric_box->pack_start(*ed, true, true, 0);
        if (part.parametric.count("table") && part.parametric.at("table") == tab) {
            ed->update(part.parametric);
        }
        parametric_editor = ed;
        parametric_editor->signal_changed().connect([this] { set_needs_save(); });
    }
}

void PartEditor::copy_from_other_part()
{
    auto top = dynamic_cast<Gtk::Window *>(get_ancestor(GTK_TYPE_WINDOW));
    PoolBrowserDialog dia(top, ObjectType::PART, pool);
    auto &br = dynamic_cast<PoolBrowserPart &>(dia.get_browser());
    br.set_entity_uuid(part.entity->uuid);
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

PartEditor *PartEditor::create(Part &p, IPool &po, PoolParametric &pp)
{
    PartEditor *w;
    Glib::RefPtr<Gtk::Builder> x = Gtk::Builder::create();
    x->add_from_resource("/org/horizon-eda/horizon/pool-prj-mgr/pool-mgr/editors/part_editor.ui");
    x->get_widget_derived("part_editor", w, p, po, pp);
    w->reference();
    return w;
}
} // namespace horizon
