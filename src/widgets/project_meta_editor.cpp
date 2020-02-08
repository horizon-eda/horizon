#include "project_meta_editor.hpp"
#include "util/gtk_util.hpp"
#include "util/str_util.hpp"

namespace horizon {

static const std::vector<std::pair<std::string, std::string>> well_known_fields = {
        {"Project title", "project_title"},
        {"Project name", "project_name"},
        {"Author", "author"},
        {"Revision", "rev"},
        {"Date", "date"},
};

class CustomFieldEditor : public Gtk::Box {
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
        }
    }
};

ProjectMetaEditor::ProjectMetaEditor(std::map<std::string, std::string> &v) : values(v)
{
    set_column_spacing(10);
    set_row_spacing(10);
    property_margin() = 20;
    Gtk::Entry *en = nullptr;
    for (const auto &it : well_known_fields) {
        en = add_editor(it.first, it.second);
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
        auto ed = Gtk::manage(new CustomFieldEditor(values, ""));
        custom_box->pack_start(*ed, true, true, 0);
        ed->show();
        ed->focus();
    });
    custom_box->pack_end(*custom_add_button, false, false, 0);

    for (auto &it_v : values) {
        if (!std::count_if(well_known_fields.begin(), well_known_fields.end(),
                           [&it_v](const auto &x) { return x.second == it_v.first; })) {
            auto ed = Gtk::manage(new CustomFieldEditor(values, it_v.first));
            custom_box->pack_start(*ed, true, true, 0);
            ed->show();
        }
    }
}

Gtk::Entry *ProjectMetaEditor::add_editor(const std::string &title, const std::string &key)
{
    auto entry = Gtk::manage(new Gtk::Entry);
    entry->set_hexpand(true);
    bind_widget(entry, values[key]);
    grid_attach_label_and_widget(this, title, entry, top);
    return entry;
}


} // namespace horizon
