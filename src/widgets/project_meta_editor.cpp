#include "project_meta_editor.hpp"
#include "util/gtk_util.hpp"
#include "util/str_util.hpp"

namespace horizon {

class Field {
public:
    Field(const std::string &k, const std::string &l, const std::string &d) : key(k), label(l), description(d)
    {
    }
    const std::string key;
    const std::string label;
    const std::string description;
};

static const std::vector<Field> well_known_fields = {
        {"project_title", "Project title", "For reference only, write what you want."},
        {"project_name", "Project name", "Will be used for file names, so keep it short and simple."},
        {"author", "Author", "Will show up in title blocks and PDF metadata."},
        {"rev", "Revision", "Will show up in title blocks."},
        {"date", "Date", "Will show up in title blocks."},
};

class CustomFieldEditor : public Gtk::Box, public Changeable {
public:
    CustomFieldEditor(std::map<std::string, std::string> &v, const std::string &k)
        : Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 5), values(v), key(k)
    {
        auto box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 0));
        box->get_style_context()->add_class("linked");
        pack_start(*box, true, true, 0);
        entry_key = Gtk::manage(new Gtk::Entry);
        entry_key->set_text(key);
        entry_key->set_width_chars(12);
        box->pack_start(*entry_key, false, false, 0);

        entry_value = Gtk::manage(new Gtk::Entry);
        if (values.count(key))
            entry_value->set_text(values.at(key));
        box->pack_start(*entry_value, true, true, 0);

        entry_key->signal_changed().connect(sigc::mem_fun(*this, &CustomFieldEditor::update_key));
        entry_value->signal_changed().connect(sigc::mem_fun(*this, &CustomFieldEditor::update_value));

        auto remove_button = Gtk::manage(new Gtk::Button);
        remove_button->set_image_from_icon_name("list-remove-symbolic", Gtk::ICON_SIZE_BUTTON);
        remove_button->signal_clicked().connect([this] {
            values.erase(key);
            delete this;
        });
        pack_start(*remove_button, false, false, 0);
        remove_button->show();

        box->show_all();

        if (!key.size()) {
            entry_set_warning(entry_key, "Empty key");
            entry_value->set_sensitive(false);
        }
    }

    void focus()
    {
        entry_key->grab_focus();
    }

    const std::string &get_key() const
    {
        return key;
    }

private:
    std::map<std::string, std::string> &values;
    std::string key;
    Gtk::Entry *entry_value = nullptr;
    Gtk::Entry *entry_key = nullptr;

    void update_key()
    {
        std::string k = entry_key->get_text();
        std::string v = entry_value->get_text();
        trim(k);
        trim(v);
        values.erase(key);

        entry_set_warning(entry_key, "");
        entry_value->set_sensitive(true);

        if (k.size()) {
            if (values.count(k)) {
                key.clear();
                entry_value->set_sensitive(false);
                entry_set_warning(entry_key, "Duplicate key");
            }
            else {
                key = k;
                values[key] = v;
                s_signal_changed.emit();
            }
        }
        else {
            entry_set_warning(entry_key, "Empty key");
            entry_value->set_sensitive(false);
        }
    }

    void update_value()
    {
        if (key.size()) {
            std::string v = entry_value->get_text();
            trim(v);
            values[key] = v;
            s_signal_changed.emit();
        }
    }
};

ProjectMetaEditor::ProjectMetaEditor(std::map<std::string, std::string> &v) : values(v)
{
    set_column_spacing(10);
    set_row_spacing(10);
    Gtk::Entry *en = nullptr;
    for (const auto &it : well_known_fields) {
        en = add_editor(it.label, it.description, it.key);
    }
    custom_box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL, 10));
    auto la_custom = grid_attach_label_and_widget(this, "Custom", custom_box, top);
    la_custom->set_valign(Gtk::ALIGN_START);
    custom_box->show();

    if (en) {
        auto sg = Gtk::SizeGroup::create(Gtk::SIZE_GROUP_VERTICAL);
        sg->add_widget(*la_custom);
        sg->add_widget(*en);
    }

    auto custom_add_button = Gtk::manage(new Gtk::Button);
    custom_add_button->set_image_from_icon_name("list-add-symbolic", Gtk::ICON_SIZE_BUTTON);
    custom_add_button->show();
    custom_add_button->set_halign(Gtk::ALIGN_START);
    custom_add_button->signal_clicked().connect([this] {
        auto ed = add_custom_editor("");
        ed->focus();
    });
    custom_box->pack_end(*custom_add_button, false, false, 0);

    for (auto &it_v : values) {
        if (!std::count_if(well_known_fields.begin(), well_known_fields.end(),
                           [&it_v](const auto &x) { return x.key == it_v.first; })) {
            add_custom_editor(it_v.first);
        }
    }
}

static Gtk::Label *grid_attach_label_and_widget_with_description(Gtk::Grid *gr, const std::string &label,
                                                                 Gtk::Widget *w, const std::string &descr, int &top)
{
    auto sg = Gtk::SizeGroup::create(Gtk::SIZE_GROUP_VERTICAL);
    auto la = Gtk::manage(new Gtk::Label(label));
    la->get_style_context()->add_class("dim-label");
    la->set_halign(Gtk::ALIGN_END);
    la->set_valign(Gtk::ALIGN_START);
    la->show();
    gr->attach(*la, 0, top, 1, 1);
    sg->add_widget(*la);
    sg->add_widget(*w);

    auto box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL, 5));
    box->pack_start(*w, true, true, 0);
    {
        auto la_descr = Gtk::manage(new Gtk::Label(descr));
        la_descr->get_style_context()->add_class("dim-label");
        la_descr->set_halign(Gtk::ALIGN_START);
        la_descr->show();
        auto attributes_list = pango_attr_list_new();
        auto attribute_scale = pango_attr_scale_new(.833);
        pango_attr_list_insert(attributes_list, attribute_scale);
        gtk_label_set_attributes(la_descr->gobj(), attributes_list);
        pango_attr_list_unref(attributes_list);
        box->pack_start(*la_descr, false, false, 0);
    }

    box->show_all();
    gr->attach(*box, 1, top, 1, 1);
    top++;
    return la;
}

Gtk::Entry *ProjectMetaEditor::add_editor(const std::string &title, const std::string &descr, const std::string &key)
{
    auto entry = Gtk::manage(new Gtk::Entry);
    entry->set_hexpand(true);
    bind_widget(entry, values[key], [this](auto &x) { s_signal_changed.emit(); });
    grid_attach_label_and_widget_with_description(this, title, entry, descr, top);
    entries.emplace(key, entry);
    return entry;
}

CustomFieldEditor *ProjectMetaEditor::add_custom_editor(const std::string &key)
{
    auto ed = Gtk::manage(new CustomFieldEditor(values, key));
    custom_box->pack_start(*ed, true, true, 0);
    ed->show();
    ed->signal_changed().connect([this] { s_signal_changed.emit(); });
    return ed;
}

void ProjectMetaEditor::clear()
{
    for (auto &w : entries) {
        w.second->set_text("");
    }
    auto children = custom_box->get_children();
    for (auto ch : children) {
        if (auto w = dynamic_cast<CustomFieldEditor *>(ch)) {
            values.erase(w->get_key());
            delete w;
        }
    }
}

void ProjectMetaEditor::preset()
{
    if (entries.count("author")) {
        auto name = Glib::get_real_name();
        if (name == "Unknown")
            name = Glib::get_user_name();
        entries.at("author")->set_text(name);
    }
    if (entries.count("rev"))
        entries.at("rev")->set_text("1");
    if (entries.count("date"))
        entries.at("date")->set_text(Glib::DateTime::create_now_local().format("%Y-%m-%d"));
}


} // namespace horizon
