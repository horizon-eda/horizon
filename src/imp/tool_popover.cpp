#include "tool_popover.hpp"
#include "action_catalog.hpp"
#include "util/str_util.hpp"

namespace horizon {
ToolPopover::ToolPopover(Gtk::Widget *parent) : Gtk::Popover(*parent)
{
    auto box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL, 4));
    search_entry = Gtk::manage(new Gtk::SearchEntry());
    box->pack_start(*search_entry, false, false, 0);

    store = Gtk::ListStore::create(list_columns);
    store->set_sort_column(list_columns.name, Gtk::SORT_ASCENDING);

    store_filtered = Gtk::TreeModelFilter::create(store);
    store_filtered->set_visible_func([this](const Gtk::TreeModel::const_iterator &it) -> bool {
        const std::string search = search_entry->get_text();
        const Gtk::TreeModel::Row row = *it;
        if (row[list_columns.can_begin] == false)
            return false;
        // std::string name = it;
        Glib::ustring tool_name = row[list_columns.name];
        auto r = std::search(tool_name.begin(), tool_name.end(), search.begin(), search.end(),
                             [](const auto &a, const auto &b) -> bool { return std::tolower(a) == std::tolower(b); });
        return r != tool_name.end();

    });
    search_entry->signal_search_changed().connect([this] {
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
    view->append_column("Tool", list_columns.name);
    view->append_column("Keys", list_columns.keys);
    view->set_enable_search(false);
    view->signal_key_press_event().connect([this](GdkEventKey *ev) -> bool {
        std::cout << "handle ev" << std::endl;
        search_entry->grab_focus_without_selecting();
        return search_entry->handle_event(ev);
    });

    search_entry->signal_activate().connect(sigc::mem_fun(this, &ToolPopover::emit_tool_activated));
    view->signal_row_activated().connect([this](auto a, auto b) { this->emit_tool_activated(); });

    auto sc = Gtk::manage(new Gtk::ScrolledWindow());
    sc->set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC);
    sc->set_min_content_height(200);
    sc->add(*view);

    box->pack_start(*sc, true, true, 0);

    Gtk::TreeModel::Row row;
    for (const auto &it : action_catalog) {
        if (it.first.first == ActionID::TOOL) {
            row = *(store->append());
            row[list_columns.name] = it.second.name;
            row[list_columns.tool_id] = it.first.second;
            row[list_columns.can_begin] = true;
            row[list_columns.keys] = "";
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
        s_signal_tool_activated.emit(row[list_columns.tool_id]);
    }
}

void ToolPopover::set_can_begin(const std::map<ToolID, bool> &can_begin)
{
    for (auto &it : store->children()) {
        if (can_begin.count(it[list_columns.tool_id])) {
            it[list_columns.can_begin] = can_begin.at(it[list_columns.tool_id]);
        }
        else {
            it[list_columns.can_begin] = true;
        }
    }
}

void ToolPopover::set_key_sequences(ToolID tool_id, const std::vector<KeySequence2> &seqs)
{
    std::stringstream s;
    std::transform(seqs.begin(), seqs.end(), std::ostream_iterator<std::string>(s, ","),
                   [](const auto &x) { return key_sequence_to_string(x); });
    auto str = s.str();
    if (str.size())
        str.pop_back();
    for (auto &it : store->children()) {
        if (it[list_columns.tool_id] == tool_id) {
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
