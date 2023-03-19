#include "property_panel.hpp"
#include "common/object_descr.hpp"
#include "property_editor.hpp"
#include "property_panels.hpp"
#include "core/core.hpp"
#include "util/util.hpp"
#include "util/gtk_util.hpp"
#include <assert.h>

namespace horizon {

PropertyPanel::PropertyPanel(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x, ObjectType ty, Core *c)
    : Gtk::Expander(cobject), type(ty), core(c)
{
    x->get_widget("editors_box", editors_box);
    x->get_widget("button_sel", button_sel);
    x->get_widget("button_prev", button_prev);
    x->get_widget("button_next", button_next);

    sel_menu.set_halign(Gtk::ALIGN_CENTER);
    button_sel->set_menu(sel_menu);
    button_sel_label = Gtk::manage(new Gtk::Label);
    button_sel->add(*button_sel_label);
    label_set_tnum(button_sel_label);

    std::vector<std::pair<ObjectProperty::ID, const ObjectProperty *>> properties_sorted;
    properties_sorted.reserve(object_descriptions.at(type).properties.size());
    for (const auto &it : object_descriptions.at(type).properties) {
        properties_sorted.emplace_back(it.first, &it.second);
    }
    std::sort(properties_sorted.begin(), properties_sorted.end(),
              [](const auto a, const auto b) { return a.second->order < b.second->order; });

    for (const auto &it : properties_sorted) {
        PropertyEditor *e;
        ObjectProperty::ID property = it.first;
        switch (it.second->type) {
        case ObjectProperty::Type::BOOL:
            e = new PropertyEditorBool(type, property, this);
            break;

        case ObjectProperty::Type::STRING:
            e = new PropertyEditorString(type, property, this);
            break;

        case ObjectProperty::Type::LENGTH: {
            auto pe = new PropertyEditorDim(type, property, this);
            pe->set_range(0, 1e9);
            e = pe;
        } break;

        case ObjectProperty::Type::DIM:
            e = new PropertyEditorDim(type, property, this);
            break;

        case ObjectProperty::Type::ENUM:
            e = new PropertyEditorEnum(type, property, this);
            break;

        case ObjectProperty::Type::STRING_RO:
            e = new PropertyEditorStringRO(type, property, this);
            break;

        case ObjectProperty::Type::NET_CLASS:
            e = new PropertyEditorNetClass(type, property, this);
            break;

        case ObjectProperty::Type::LAYER:
            e = new PropertyEditorLayer(type, property, this);
            break;

        case ObjectProperty::Type::LAYER_COPPER: {
            auto pe = new PropertyEditorLayer(type, property, this);
            pe->copper_only = true;
            e = pe;
        } break;

        case ObjectProperty::Type::ANGLE:
            e = new PropertyEditorAngle(type, property, this);
            break;

        case ObjectProperty::Type::STRING_MULTILINE:
            e = new PropertyEditorStringMultiline(type, property, this);
            break;

        case ObjectProperty::Type::EXPAND:
            e = new PropertyEditorExpand(type, property, this);
            break;

        case ObjectProperty::Type::OPACITY:
            e = new PropertyEditorOpacity(type, property, this);
            break;

        case ObjectProperty::Type::PRIORITY:
            e = new PropertyEditorPriority(type, property, this);
            break;

        case ObjectProperty::Type::SCALE:
            e = new PropertyEditorScale(type, property, this);
            break;

        case ObjectProperty::Type::LAYER_RANGE:
            e = new PropertyEditorLayerRange(type, property, this);
            break;

        default:
            e = new PropertyEditor(type, property, this);
        }

        e->signal_changed().connect(
                [this, property, e] { handle_changed(property, e->get_value(), e->get_apply_all()); });
        e->signal_activate().connect([this] { parent->s_signal_activate.emit(); });
        e->signal_apply_all().connect([this, property, e] {
            if (e->get_apply_all()) {
                handle_apply_all(property, e->get_value());
            }
        });
        auto em = Gtk::manage(e);
        em->construct();
        editors_box->pack_start(*em, false, false, 0);
        em->show_all();
    }
    reload();
    button_next->signal_clicked().connect(sigc::bind<int>(sigc::mem_fun(*this, &PropertyPanel::go), 1));
    button_prev->signal_clicked().connect(sigc::bind<int>(sigc::mem_fun(*this, &PropertyPanel::go), -1));
}

void PropertyPanel::handle_changed(ObjectProperty::ID property, const PropertyValue &value, bool all)
{
    if (all) {
        parent->set_property(type, objects, property, value);
    }
    else {
        parent->set_property(type, {objects.at(object_current)}, property, value);
    }
}

void PropertyPanel::handle_apply_all(ObjectProperty::ID property, const PropertyValue &value)
{
    if (core->get_property_transaction()) {
        for (const auto &uu : objects) {
            core->set_property(type, uu, property, value);
        }
        parent->force_commit();
    }
    else {
        core->set_property_begin();
        for (const auto &uu : objects) {
            core->set_property(type, uu, property, value);
        }
        core->set_property_commit();
        parent->signal_update().emit();
        parent->reload();
    }
}

PropertyPanel *PropertyPanel::create(ObjectType t, Core *c, PropertyPanels *parent)
{
    PropertyPanel *w;
    Glib::RefPtr<Gtk::Builder> x = Gtk::Builder::create();
    x->add_from_resource("/org/horizon-eda/horizon/property_panels/property_panel.ui");
    x->get_widget_derived("PropertyPanel", w, t, c);
    w->reference();
    w->parent = parent;

    w->set_use_markup(true);
    w->set_label("<b>" + object_descriptions.at(w->type).name_pl + "</b>");
    return w;
}

class MyMenuItem : public Gtk::CheckMenuItem {
public:
    MyMenuItem(const std::string &la, const UUID &uu) : Gtk::CheckMenuItem(la), uuid(uu)
    {
        set_draw_as_radio(true);
    }
    UUID uuid;
};

void PropertyPanel::update_selector()
{
    button_sel_label->set_text(format_m_of_n(object_current + 1, objects.size()));
    auto chs = sel_menu.get_children();
    for (auto it : chs) {
        if (auto mit = dynamic_cast<MyMenuItem *>(it)) {
            mit->set_active(mit->uuid == objects.at(object_current));
        }
    }
}

void PropertyPanel::go(int dir)
{
    object_current += dir;
    if (object_current < 0) {
        object_current += objects.size();
    }
    object_current %= objects.size();
    update_selector();
    reload();
}

void PropertyPanel::update_objects(const std::set<SelectableRef> &selection)
{
    UUID uuid_current;
    if (objects.size())
        uuid_current = objects.at(object_current);
    std::set<UUID> uuids_from_sel;
    for (const auto &it : selection) {
        uuids_from_sel.insert(it.uuid);
    }
    // delete objects not in selection
    objects.erase(std::remove_if(objects.begin(), objects.end(),
                                 [&uuids_from_sel](auto &a) { return uuids_from_sel.count(a) == 0; }),
                  objects.end());

    // add new objects from selection
    for (const auto &it : selection) {
        if (std::find(objects.begin(), objects.end(), it.uuid) == objects.end()) {
            objects.push_back(it.uuid);
        }
    }

    std::sort(objects.begin(), objects.end(), [this](const auto &a, const auto &b) {
        return strcmp_natural(core->get_display_name(type, a), core->get_display_name(type, b)) < 0;
    });

    object_current = 0;
    for (size_t i = 0; i < objects.size(); i++) {
        if (objects[i] == uuid_current) {
            object_current = i;
            break;
        }
    }
    reload();

    for (const auto ch : editors_box->get_children()) {
        if (auto ed = dynamic_cast<PropertyEditor *>(ch)) {
            ed->set_can_apply_all(objects.size() > 1);
        }
    }


    {
        auto chs = sel_menu.get_children();
        for (auto it : chs) {
            delete it;
        }
    }
    int i = 0;
    for (const auto &it : objects) {
        auto name = core->get_display_name(type, it);
        if (name.size()) {
            auto la = Gtk::manage(new MyMenuItem(name, it));
            la->show();
            la->signal_toggled().connect([this, i, la] {
                if (la->get_active()) {
                    object_current = i;
                    update_selector();
                    reload();
                }
            });
            sel_menu.append(*la);
        }
        i++;
    }
    update_selector();
}

ObjectType PropertyPanel::get_type()
{
    return type;
}

void PropertyPanel::reload()
{
    if (!objects.size())
        return;
    for (const auto ch : editors_box->get_children()) {
        if (auto ed = dynamic_cast<PropertyEditor *>(ch)) {
            auto uu = objects.at(object_current);
            PropertyValue &value = ed->get_value();
            PropertyMeta &meta = ed->get_meta();
            if (value.get_type() != PropertyValue::Type::INVALID) {
                assert(core->get_property(type, uu, ed->property_id, value));
                core->get_property_meta(type, uu, ed->property_id, meta);
                ed->reload();
                ed->set_sensitive(meta.is_settable);
                ed->set_visible(meta.is_visible);
            }
        }
    }
}
} // namespace horizon
