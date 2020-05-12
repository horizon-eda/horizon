#include "tool_popover.hpp"
#include "action_catalog.hpp"
#include "util/str_util.hpp"
#include <iostream>

namespace horizon {
ToolPopover::ToolPopover(Gtk::Widget *parent, ActionCatalogItem::Availability availability) : Gtk::Popover(*parent)
{
    get_style_context()->add_class("imp-tool-popover");
    auto box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL));
    search_entry = Gtk::manage(new Gtk::SearchEntry());
    search_entry->set_margin_bottom(5);
    box->pack_start(*search_entry, false, false, 0);

    store = Gtk::ListStore::create(list_columns);
    store->set_sort_column(list_columns.name, Gtk::SORT_ASCENDING);

    store_filtered = Gtk::TreeModelFilter::create(store);
    store_filtered->set_visible_func([this](const Gtk::TreeModel::const_iterator &it) -> bool {
        const Gtk::TreeModel::Row row = *it;
        if (row[list_columns.can_begin] == false)
            return false;
        if (!pattern) {
            if (selected_group == ActionGroup::ALL)
                return true;
            else {
                auto key = std::make_pair(row[list_columns.action_id], row[list_columns.tool_id]);
                return action_catalog.at(key).group == selected_group;
            }
        }
        Glib::ustring tool_name_u = row[list_columns.name];
        std::string tool_name(tool_name_u);
        std::transform(tool_name.begin(), tool_name.end(), tool_name.begin(), tolower);
        return pattern->match(tool_name);
    });
    search_entry->signal_changed().connect([this] {
        std::string search = search_entry->get_text();
        revealer->set_reveal_child(search.size() == 0);

        if (search.size()) {
            std::transform(search.begin(), search.end(), search.begin(), tolower);
            search = "*" + search + "*";
            std::replace(search.begin(), search.end(), ' ', '*');

            pattern.reset(new Glib::PatternSpec(search));
        }
        else {
            pattern.reset();
        }
        store_filtered->refilter();
        if (store_filtered->children().size())
            view->get_selection()->select(store_filtered->children().begin());
        auto it = view->get_selection()->get_selected();
        if (it) {
            view->scroll_to_row(store_filtered->get_path(it));
        }
    });

    view = Gtk::manage(new Gtk::TreeView(store_filtered));
    view->get_selection()->set_mode(Gtk::SELECTION_BROWSE);
    view->append_column("Action", list_columns.name);
    view->append_column("Keys", list_columns.keys);
    view->set_enable_search(false);
    view->set_activate_on_single_click(true);
    view->signal_key_press_event().connect([this](GdkEventKey *ev) -> bool {
        search_entry->grab_focus_without_selecting();
        return search_entry->handle_event(ev);
    });

    search_entry->signal_activate().connect(sigc::mem_fun(*this, &ToolPopover::emit_tool_activated));
    view->signal_row_activated().connect([this](auto a, auto b) { this->emit_tool_activated(); });

    sc = Gtk::manage(new Gtk::ScrolledWindow());
    sc->set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC);
    sc->set_shadow_type(Gtk::SHADOW_IN);
    sc->set_min_content_height(210);
    sc->add(*view);


    store_group = Gtk::ListStore::create(list_columns_group);

    view_group = Gtk::manage(new Gtk::TreeView(store_group));
    view_group->get_selection()->set_mode(Gtk::SELECTION_BROWSE);
    view_group->append_column("Group", list_columns_group.name);
    view_group->set_row_separator_func(
            [this](const Glib::RefPtr<Gtk::TreeModel> &model, const Gtk::TreeModel::iterator &iter) {
                Gtk::TreeModel::Row row = *iter;
                return row[list_columns_group.name] == "SEPARATOR";
            });
    view_group->get_selection()->signal_changed().connect([this] {
        auto iter = view_group->get_selection()->get_selected();
        if (iter) {
            selected_group = (*iter)[list_columns_group.group];
        }
        else {
            selected_group = ActionGroup::ALL;
        }
        store_filtered->refilter();
    });


    auto box2 = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 0));


    auto sc2 = Gtk::manage(new Gtk::ScrolledWindow());
    sc2->set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC);
    sc2->set_shadow_type(Gtk::SHADOW_IN);
    sc2->add(*view_group);
    sc2->set_margin_end(5);

    revealer = Gtk::manage(new Gtk::Revealer);
    revealer->add(*sc2);
    revealer->set_transition_type(Gtk::REVEALER_TRANSITION_TYPE_SLIDE_RIGHT);
    revealer->set_reveal_child(true);

    box2->pack_start(*revealer, false, false, 0);

    box2->pack_start(*sc, true, true, 0);
    box->pack_start(*box2, true, true, 0);

    auto evb = Gtk::manage(new Gtk::EventBox);
    evb->set_size_request(-1, 7);
    evb->signal_realize().connect([evb] {
        auto dpy = evb->get_display();
        evb->get_window()->set_cursor(Gdk::Cursor::create(dpy, "ns-resize"));
    });
    evb->signal_button_press_event().connect([this](GdkEventButton *ev) {
        if (ev->button == 1) {
            y_start = ev->y_root;
            sc_height = sc->get_min_content_height();
            return true;
        }
        return false;
    });
    evb->signal_button_release_event().connect([this](GdkEventButton *ev) {
        if (ev->button == 1) {
            y_start = NAN;
            return true;
        }
        return false;
    });
    evb->signal_motion_notify_event().connect([this](GdkEventMotion *ev) {
        if (!std::isnan(y_start)) {
            sc->set_min_content_height(sc_height + (ev->y_root - y_start));
            return true;
        }
        return false;
    });
    box->set_margin_start(5);
    box->set_margin_end(5);
    box->set_margin_top(5);
    box->pack_start(*evb, false, false, 0);

    for (const auto &it : action_catalog) {
        if ((it.second.availability & availability) && !(it.second.flags & ActionCatalogItem::FLAGS_NO_POPOVER)) {
            Gtk::TreeModel::Row row = *(store->append());
            row[list_columns.name] = it.second.name;
            row[list_columns.tool_id] = it.first.second;
            row[list_columns.action_id] = it.first.first;
            row[list_columns.can_begin] = true;
            row[list_columns.keys] = "";
        }
    }
    {
        Gtk::TreeModel::Row row = *(store_group->append());
        row[list_columns_group.name] = "All";
        row[list_columns_group.group] = ActionGroup::ALL;
        view_group->get_selection()->select(row);
    }

    {
        Gtk::TreeModel::Row row = *(store_group->append());
        row[list_columns_group.name] = "SEPARATOR";
        row[list_columns_group.group] = ActionGroup::ALL;
    }

    for (const auto &it : action_group_catalog) {
        bool show = std::any_of(action_catalog.begin(), action_catalog.end(), [availability, it](const auto &x) {
            return (x.second.group == it.first) && (x.second.availability & availability);
        });

        if (show) {
            Gtk::TreeModel::Row row = *(store_group->append());
            row[list_columns_group.name] = it.second;
            row[list_columns_group.group] = it.first;
        }
    }

    add(*box);
    box->show_all();
}

void ToolPopover::emit_tool_activated()
{
    auto it = view->get_selection()->get_selected();
    if (it) {
#if GTK_CHECK_VERSION(3, 22, 0)
        popdown();
#else
        hide();
#endif
        Gtk::TreeModel::Row row = *it;
        s_signal_action_activated.emit(row[list_columns.action_id], row[list_columns.tool_id]);
    }
}

void ToolPopover::set_can_begin(const std::map<ActionToolID, bool> &can_begin)
{
    for (auto &it : store->children()) {
        auto k = std::make_pair(it[list_columns.action_id], it[list_columns.tool_id]);
        if (can_begin.count(k)) {
            it[list_columns.can_begin] = can_begin.at(k);
        }
        else {
            it[list_columns.can_begin] = true;
        }
    }
}

void ToolPopover::set_key_sequences(ActionToolID action_id, const std::vector<KeySequence> &seqs)
{
    auto str = key_sequences_to_string(seqs);
    for (auto &it : store->children()) {
        if (it[list_columns.tool_id] == action_id.second && it[list_columns.action_id] == action_id.first) {
            it[list_columns.keys] = str;
        }
    }
}

void ToolPopover::on_show()
{
    Gtk::Popover::on_show();
    search_entry->select_region(0, -1);
}
} // namespace horizon
