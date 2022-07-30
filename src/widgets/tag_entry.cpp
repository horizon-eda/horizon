#include "tag_entry.hpp"
#include "pool/ipool.hpp"
#include "util/sqlite.hpp"
#include "util/str_util.hpp"

namespace horizon {

class TagEntry::TagPopover : public Gtk::Popover {
public:
    TagPopover(TagEntry *p);

    TagEntry *parent;
    Gtk::SearchEntry *search_entry;
    class ListColumns : public Gtk::TreeModelColumnRecord {
    public:
        ListColumns()
        {
            Gtk::TreeModelColumnRecord::add(name);
            Gtk::TreeModelColumnRecord::add(count);
        }
        Gtk::TreeModelColumn<Glib::ustring> name;
        Gtk::TreeModelColumn<int> count;
    };
    ListColumns list_columns;

    Gtk::TreeView *view;
    Glib::RefPtr<Gtk::ListStore> store;

    void update_list();
    void update_list_edit();
    void activate();
};

TagEntry::TagPopover::TagPopover(TagEntry *p) : Gtk::Popover(), parent(p)
{
    auto box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL, 3));
    search_entry = Gtk::manage(new Gtk::SearchEntry());
    box->pack_start(*search_entry, false, false, 0);

    store = Gtk::ListStore::create(list_columns);


    search_entry->signal_changed().connect([this] {
        std::string search = search_entry->get_text();
        update_list();
        if (!parent->edit_mode) {
            if (store->children().size())
                view->get_selection()->select(store->children().begin());
            auto it = view->get_selection()->get_selected();
            if (it) {
                view->scroll_to_row(store->get_path(it));
            }
        }
    });
    if (parent->edit_mode) {
        search_entry->signal_activate().connect([this] {
            std::string tag = search_entry->get_text();
            trim(tag);
            if (tag.size()) {
                parent->add_tag(tag);
                search_entry->set_text("");
                search_entry->grab_focus();
                update_list();
            }
        });
    }
    else {
        search_entry->signal_activate().connect(sigc::mem_fun(*this, &TagPopover::activate));
    }

    view = Gtk::manage(new Gtk::TreeView(store));
    view->set_headers_visible(false);
    view->get_selection()->set_mode(Gtk::SELECTION_BROWSE);
    view->append_column("Tag", list_columns.name);
    view->get_column(0)->set_expand(true);
    if (!parent->edit_mode) {
        auto cr = Gtk::manage(new Gtk::CellRendererText());
        auto tvc = Gtk::manage(new Gtk::TreeViewColumn("N", *cr));
        tvc->add_attribute(cr->property_text(), list_columns.count);
        cr->property_xalign() = 1;
        cr->property_sensitive() = false;
        auto attributes_list = pango_attr_list_new();
        auto attribute_font_features = pango_attr_font_features_new("tnum 1");
        pango_attr_list_insert(attributes_list, attribute_font_features);
        g_object_set(G_OBJECT(cr->gobj()), "attributes", attributes_list, NULL);
        pango_attr_list_unref(attributes_list);
        view->append_column(*tvc);
    }
    view->set_enable_search(false);
    view->signal_key_press_event().connect([this](GdkEventKey *ev) -> bool {
        search_entry->grab_focus_without_selecting();
        return search_entry->handle_event(ev);
    });

    view->signal_row_activated().connect(
            [this](const Gtk::TreeModel::Path &path, Gtk::TreeViewColumn *col) { activate(); });

    property_visible().signal_changed().connect([this] {
        if (get_visible()) {
            search_entry->set_text("");
            view->get_selection()->unselect_all();
            update_list();
        }
    });

    auto sc = Gtk::manage(new Gtk::ScrolledWindow());
    sc->set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC);
    sc->set_shadow_type(Gtk::SHADOW_IN);
    sc->set_min_content_height(210);
    sc->add(*view);

    box->pack_start(*sc, true, true, 0);

    add(*box);
    box->show_all();

    update_list();
}

void TagEntry::TagPopover::activate()
{
    auto it = view->get_selection()->get_selected();
    if (it) {
        Gtk::TreeModel::Row row = *it;
        Glib::ustring tag = row[list_columns.name];
        parent->add_tag(tag);
        search_entry->set_text("");
        search_entry->grab_focus();
        update_list();
    }
}

void TagEntry::TagPopover::update_list()
{
    store->clear();
    if (parent->edit_mode) {
        update_list_edit();
        return;
    }
    auto tags_existing = parent->get_tags();
    auto ntags = tags_existing.size();
    std::stringstream query;
    query << "SELECT tag, count(*) AS cnt, tag LIKE $pre AS prefix from tags "
             "WHERE type = $type "
             "AND tag LIKE $tag "
             "AND tag NOT in (";
    for (size_t i = 0; i < ntags; i++) {
        query << "$etag" << i << ",";
    }
    query << "'') ";
    if (ntags) {
        query << "AND uuid IN (SELECT uuid FROM tags WHERE (";

        for (size_t i = 0; i < ntags; i++) {
            query << "tag = $etag" << i << " OR ";
        }
        query << "0) AND type = $type "
                 "GROUP by tags.uuid HAVING count(*) >= $ntags) ";
    }
    query << "GROUP BY tag "
             "ORDER BY prefix DESC, cnt DESC";
    SQLite::Query q(parent->pool.get_db(), query.str());
    {
        size_t i = 0;
        for (const auto &it : tags_existing) {
            std::string b = "$etag" + std::to_string(i);
            q.bind(b.c_str(), it);
            i++;
        }
    }

    q.bind("$pre", search_entry->get_text() + "%");
    if (ntags) {
        q.bind("$ntags", ntags);
    }
    q.bind("$tag", "%" + search_entry->get_text() + "%");
    q.bind("$type", parent->type);
    while (q.step()) {
        Gtk::TreeModel::Row row = *store->append();
        row[list_columns.name] = q.get<std::string>(0);
        row[list_columns.count] = q.get<int>(1);
    }
}

void TagEntry::TagPopover::update_list_edit()
{
    auto tags_existing = parent->get_tags();
    auto ntags = tags_existing.size();
    std::stringstream query;
    query << "WITH ids_existing AS (SELECT uuid FROM tags WHERE type = $type "
             "AND tag in (";
    for (size_t i = 0; i < ntags; i++) {
        query << "$etag" << i << ",";
    }
    query << "'') GROUP BY uuid HAVING count(*) >= $ntags) ";
    query << "SELECT tag, count(*) AS cnt, tag LIKE $pre AS prefix, SUM(uuid IN ids_existing) AS n_ex FROM tags "
             "WHERE type = $type "
             "AND tag LIKE $tag "
             "AND tag NOT in (";
    for (size_t i = 0; i < ntags; i++) {
        query << "$etag" << i << ",";
    }
    query << "'') ";
    query << "GROUP BY tag "
             "ORDER BY prefix DESC, n_ex DESC, cnt DESC";
    SQLite::Query q(parent->pool.get_db(), query.str());
    {
        size_t i = 0;
        for (const auto &it : tags_existing) {
            std::string b = "$etag" + std::to_string(i);
            q.bind(b.c_str(), it);
            i++;
        }
    }

    q.bind("$pre", search_entry->get_text() + "%");
    if (ntags) {
        q.bind("$ntags", ntags);
    }
    q.bind("$tag", "%" + search_entry->get_text() + "%");
    q.bind("$type", parent->type);
    while (q.step()) {
        Gtk::TreeModel::Row row = *store->append();
        row[list_columns.name] = q.get<std::string>(0);
        row[list_columns.count] = q.get<int>(1);
    }
}

TagEntry::TagEntry(IPool &p, ObjectType t, bool e)
    : Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 4), pool(p), type(t), edit_mode(e)
{
    box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 4));
    box->show();
    pack_start(*box, false, false, 0);

    add_button = Gtk::manage(new Gtk::MenuButton);
    add_button->set_image_from_icon_name("list-add-symbolic", Gtk::ICON_SIZE_BUTTON);
    add_button->show();
    add_button->set_tooltip_text("No more tags available");
    add_button->set_has_tooltip(false);
    pack_start(*add_button, false, false, 0);

    auto popover = Gtk::manage(new TagPopover(this));
    add_button->set_popover(*popover);
}

class TagEntry::TagLabel : public Gtk::Box {
public:
    TagLabel(TagEntry *p, const std::string &t) : Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 2), parent(p), tag(t)
    {
        la = Gtk::manage(new Gtk::Label(t));
        pack_start(*la, false, false, 0);
        la->show();

        bu = Gtk::manage(new Gtk::Button);
        bu->set_image_from_icon_name("window-close-symbolic", Gtk::ICON_SIZE_BUTTON);
        bu->set_relief(Gtk::RELIEF_NONE);
        bu->get_style_context()->add_class("tag-entry-tiny-button");
        bu->get_style_context()->add_class("dim-label");
        bu->show();
        bu->signal_clicked().connect([this] { parent->remove_tag(tag); });
        pack_start(*bu, false, false, 0);
    }

private:
    TagEntry *parent;
    const std::string tag;
    Gtk::Label *la = nullptr;
    Gtk::Button *bu = nullptr;
};

void TagEntry::add_tag(const std::string &tag)
{
    auto w = Gtk::manage(new TagLabel(this, tag));
    w->show();
    box->pack_start(*w, false, false, 0);
    label_widgets.emplace(tag, w);
    s_signal_changed.emit();
    update_add_button_sensitivity();
}

void TagEntry::remove_tag(const std::string &tag)
{
    auto w = label_widgets.at(tag);
    label_widgets.erase(tag);
    delete w;
    s_signal_changed.emit();
    update_add_button_sensitivity();
}

std::set<std::string> TagEntry::get_tags() const
{
    std::set<std::string> r;
    for (const auto &it : label_widgets) {
        r.emplace(it.first);
    }
    return r;
}

void TagEntry::clear()
{
    for (auto w : label_widgets) {
        delete w.second;
    }
    label_widgets.clear();
    s_signal_changed.emit();
    update_add_button_sensitivity();
}

void TagEntry::set_tags(const std::set<std::string> &tags)
{
    for (auto w : label_widgets) {
        delete w.second;
    }
    label_widgets.clear();
    for (const auto &tag : tags) {
        auto w = Gtk::manage(new TagLabel(this, tag));
        w->show();
        box->pack_start(*w, false, false, 0);
        label_widgets.emplace(tag, w);
    }

    s_signal_changed.emit();
    update_add_button_sensitivity();
}

void TagEntry::update_add_button_sensitivity()
{
    if (edit_mode)
        return;
    auto tags = get_tags();
    size_t ntags = tags.size();
    bool tags_available = true;
    if (ntags) {
        std::stringstream query;
        query << "SELECT tag FROM tags "
                 "WHERE type = $type "
                 "AND tag NOT in (";
        for (size_t i = 0; i < ntags; i++) {
            query << "$etag" << i << ",";
        }
        query << "'') ";
        query << "AND uuid IN (SELECT uuid FROM tags WHERE (";

        for (size_t i = 0; i < ntags; i++) {
            query << "tag = $etag" << i << " OR ";
        }
        query << "0) AND type = $type "
                 "GROUP by tags.uuid HAVING count(*) >= $ntags) ";

        SQLite::Query q(pool.get_db(), query.str());
        {
            size_t i = 0;
            for (const auto &it : tags) {
                std::string b = "$etag" + std::to_string(i);
                q.bind(b.c_str(), it);
                i++;
            }
        }
        q.bind("$ntags", ntags);
        q.bind("$type", type);
        tags_available = q.step();
    }
    add_button->set_sensitive(tags_available);
    add_button->set_has_tooltip(!tags_available);
    if (!tags_available) {
        add_button->get_popover()->hide();
    }
}

} // namespace horizon
