#include "pool_merge_box.hpp"
#include "widgets/cell_renderer_color_box.hpp"
#include "common/object_descr.hpp"
#include "util/gtk_util.hpp"
#include <iomanip>

namespace horizon {

PoolMergeBox2::PoolMergeBox2(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x,
                             PoolStatusProviderPoolManager &aprv)
    : Gtk::Box(cobject), prv(aprv)
{
    GET_WIDGET(pool_item_view);
    GET_WIDGET(stack);
    GET_WIDGET(delta_text_view);
    GET_WIDGET(cb_update_layer_help);
    GET_WIDGET(cb_update_tables);
    GET_WIDGET(button_update);

    item_store = Gtk::ListStore::create(list_columns);
    pool_item_view->set_model(item_store);

    pool_item_view->get_selection()->signal_changed().connect(sigc::mem_fun(*this, &PoolMergeBox2::selection_changed));
    using ItemState = PoolStatusPoolManager::ItemInfo::ItemState;

    {
        auto cr_text = Gtk::manage(new Gtk::CellRendererText());
        auto cr_toggle = Gtk::manage(new Gtk::CellRendererToggle());
        auto tvc = Gtk::manage(new Gtk::TreeViewColumn("Action"));
        tvc->pack_start(*cr_toggle, false);
        tvc->pack_start(*cr_text, true);
        tvc->set_cell_data_func(*cr_text, [this](Gtk::CellRenderer *tcr, const Gtk::TreeModel::iterator &it) {
            Gtk::TreeModel::Row row = *it;
            auto mcr = dynamic_cast<Gtk::CellRendererText *>(tcr);
            switch (row[list_columns.state]) {
            case ItemState::CURRENT:
                mcr->property_text() = "Keep";
                break;

            case ItemState::CHANGED:
                mcr->property_text() = "Overwrite local";
                break;

            case ItemState::MOVED:
                mcr->property_text() = "Move";
                break;

            case ItemState::MOVED_CHANGED:
                mcr->property_text() = "Overwrite and move local";
                break;

            case ItemState::LOCAL_ONLY:
                mcr->property_text() = "Keep";
                break;

            case ItemState::REMOTE_ONLY:
                mcr->property_text() = "Add to local";
                break;

            default:
                mcr->property_text() = "fixme";
            }
        });
        tvc->set_cell_data_func(*cr_toggle, [this](Gtk::CellRenderer *tcr, const Gtk::TreeModel::iterator &it) {
            Gtk::TreeModel::Row row = *it;
            auto mcr = dynamic_cast<Gtk::CellRendererToggle *>(tcr);
            if (row[list_columns.state] == ItemState::CURRENT) {
                mcr->property_sensitive() = false;
                mcr->property_active() = true;
            }
            else if (row[list_columns.state] == ItemState::LOCAL_ONLY) {
                mcr->property_sensitive() = false;
                mcr->property_active() = true;
            }
            else {
                mcr->property_active() = row[list_columns.merge];
                mcr->property_sensitive() = true;
            }
        });
        cr_toggle->signal_toggled().connect(sigc::mem_fun(*this, &PoolMergeBox2::action_toggled));
        pool_item_view->append_column(*tvc);
    }

    {
        auto cr = Gtk::manage(new CellRendererColorBox());
        auto tvc = Gtk::manage(new Gtk::TreeViewColumn("State", *cr));
        auto cr2 = Gtk::manage(new Gtk::CellRendererText());
        cr2->property_text() = "hallo";

        tvc->set_cell_data_func(*cr2, [this](Gtk::CellRenderer *tcr, const Gtk::TreeModel::iterator &it) {
            Gtk::TreeModel::Row row = *it;
            auto mcr = dynamic_cast<Gtk::CellRendererText *>(tcr);
            switch (row[list_columns.state]) {
            case ItemState::CURRENT:
                mcr->property_text() = "Current";
                break;

            case ItemState::CHANGED:
                mcr->property_text() = "Changed";
                break;

            case ItemState::MOVED:
                mcr->property_text() = "Just moved";
                break;

            case ItemState::MOVED_CHANGED:
                mcr->property_text() = "Moved and changed";
                break;

            case ItemState::LOCAL_ONLY:
                mcr->property_text() = "Local only";
                break;

            case ItemState::REMOTE_ONLY:
                mcr->property_text() = "Remote only";
                break;

            default:
                mcr->property_text() = "fixme";
            }
        });
        tvc->set_cell_data_func(*cr, [this](Gtk::CellRenderer *tcr, const Gtk::TreeModel::iterator &it) {
            Gtk::TreeModel::Row row = *it;
            auto mcr = dynamic_cast<CellRendererColorBox *>(tcr);
            Color co(1, 0, 1);
            switch (row[list_columns.state]) {
            case ItemState::CURRENT:
                co = Color::new_from_int(138, 226, 52);
                break;

            case ItemState::MOVED:
            case ItemState::LOCAL_ONLY:
            case ItemState::REMOTE_ONLY:
                co = Color::new_from_int(252, 175, 62);
                break;

            default:
                co = Color::new_from_int(239, 41, 41);
            }
            Gdk::RGBA va;
            va.set_alpha(1);
            va.set_red(co.r);
            va.set_green(co.g);
            va.set_blue(co.b);
            mcr->property_color() = va;
        });
        tvc->pack_start(*cr2, false);
        tvc->set_sort_column(list_columns.state);
        pool_item_view->append_column(*tvc);
    }

    {
        auto cr = Gtk::manage(new Gtk::CellRendererText());
        auto tvc = Gtk::manage(new Gtk::TreeViewColumn("Type", *cr));
        tvc->set_cell_data_func(*cr, [this](Gtk::CellRenderer *tcr, const Gtk::TreeModel::iterator &it) {
            Gtk::TreeModel::Row row = *it;
            auto mcr = dynamic_cast<Gtk::CellRendererText *>(tcr);
            mcr->property_text() = object_descriptions.at(row[list_columns.type]).name;
        });
        pool_item_view->append_column(*tvc);
    }

    pool_item_view->append_column("Name", list_columns.name);

    {
        auto cr = Gtk::manage(new Gtk::CellRendererText());
        auto tvc = Gtk::manage(new Gtk::TreeViewColumn("Local filename", *cr));
        tvc->add_attribute(cr->property_text(), list_columns.filename_local);
        cr->property_ellipsize() = Pango::ELLIPSIZE_START;
        tvc->set_resizable(true);
        pool_item_view->append_column(*tvc);
    }
    {
        auto cr = Gtk::manage(new Gtk::CellRendererText());
        auto tvc = Gtk::manage(new Gtk::TreeViewColumn("Remote filename", *cr));
        tvc->add_attribute(cr->property_text(), list_columns.filename_remote);
        cr->property_ellipsize() = Pango::ELLIPSIZE_START;
        tvc->set_resizable(true);
        pool_item_view->append_column(*tvc);
    }


    item_store->set_sort_func(list_columns.state,
                              [this](const Gtk::TreeModel::iterator &a, const Gtk::TreeModel::iterator &b) {
                                  Gtk::TreeModel::Row ra = *a;
                                  Gtk::TreeModel::Row rb = *b;
                                  ItemState sa = ra[list_columns.state];
                                  ItemState sb = rb[list_columns.state];
                                  return static_cast<int>(sb) - static_cast<int>(sa);
                              });
    item_store->set_sort_column(list_columns.state, Gtk::SORT_ASCENDING);

    append_context_menu_item("Check", MenuOP::CHECK);
    append_context_menu_item("Uncheck", MenuOP::UNCHECK);
    append_context_menu_item("Toggle", MenuOP::TOGGLE);

    pool_item_view->signal_button_press_event().connect(
            [this](GdkEventButton *ev) {
                Gtk::TreeModel::Path path;
                if (gdk_event_triggers_context_menu((GdkEvent *)ev)
                    && pool_item_view->get_selection()->count_selected_rows()) {
#if GTK_CHECK_VERSION(3, 22, 0)
                    context_menu.popup_at_pointer((GdkEvent *)ev);
#else
                    context_menu.popup(ev->button, gtk_get_current_event_time());
#endif
                    return true;
                }
                return false;
            },
            false);

    prv.signal_changed().connect(sigc::track_obj([this] { update_from_prv(); }, *this));
    update_from_prv();

    button_update->signal_clicked().connect([this] {
        PoolStatusPoolManager st;
        for (const auto &iter : item_store->children()) {
            Gtk::TreeModel::Row row = *iter;
            st.items.emplace_back();
            auto &it = st.items.back();
            if (row.get_value(list_columns.merge)) {
                it.filename_local = row[list_columns.filename_local];
                it.filename_remote = row[list_columns.filename_remote];
                it.state = row[list_columns.state];
            }
        }
        st.tables_needs_update = cb_update_tables->get_active() ? PoolStatusPoolManager::UpdateState::REQUIRED
                                                                : PoolStatusPoolManager::UpdateState::NO;
        st.layer_help_needs_update = cb_update_layer_help->get_active() ? PoolStatusPoolManager::UpdateState::REQUIRED
                                                                        : PoolStatusPoolManager::UpdateState::NO;
        prv.apply_update(st);
    });
}

void PoolMergeBox2::action_toggled(const Glib::ustring &path)
{
    auto it = item_store->get_iter(path);
    if (it) {
        Gtk::TreeModel::Row row = *it;
        row[list_columns.merge] = !row[list_columns.merge];
    }
}

void PoolMergeBox2::append_context_menu_item(const std::string &name, MenuOP op)
{
    auto it = Gtk::manage(new Gtk::MenuItem(name));
    it->signal_activate().connect([this, op] {
        auto rows = pool_item_view->get_selection()->get_selected_rows();
        for (const auto &path : rows) {
            Gtk::TreeModel::Row row = *item_store->get_iter(path);
            switch (op) {
            case MenuOP::CHECK:
                row[list_columns.merge] = true;
                break;
            case MenuOP::UNCHECK:
                row[list_columns.merge] = false;
                break;
            case MenuOP::TOGGLE:
                row[list_columns.merge] = !row[list_columns.merge];
                break;
            }
        }
    });
    context_menu.append(*it);
    it->show();
}

PoolMergeBox2 *PoolMergeBox2::create(PoolStatusProviderPoolManager &prv)
{
    PoolMergeBox2 *w;
    Glib::RefPtr<Gtk::Builder> x = Gtk::Builder::create();
    x->add_from_resource("/org/horizon-eda/horizon/pool-prj-mgr/pools_window/pool_merge.ui");
    x->get_widget_derived("box", w, prv);
    w->reference();
    return w;
}

static void apply_update_state(Gtk::CheckButton &b, PoolStatusPoolManager::UpdateState st)
{
    switch (st) {
    case PoolStatusPoolManager::UpdateState::NO:
        b.set_sensitive(false);
        b.set_active(false);
        break;
    case PoolStatusPoolManager::UpdateState::OPTIONAL:
        b.set_sensitive(true);
        b.set_active(true);
        break;
    case PoolStatusPoolManager::UpdateState::REQUIRED:
        b.set_sensitive(false);
        b.set_active(true);
        break;
    }
}

void PoolMergeBox2::update_from_prv()
{
    if (const auto st = dynamic_cast<const PoolStatusPoolManager *>(&prv.get())) {
        item_store->set_sort_column(Gtk::TreeSortable::DEFAULT_UNSORTED_COLUMN_ID, Gtk::SORT_ASCENDING);
        pool_item_view->unset_model();
        item_store->clear();
        item_store->freeze_notify();

        for (const auto &el : st->items) {
            if (el.state != PoolStatusPoolManager::ItemInfo::ItemState::CURRENT) {
                auto it = item_store->append();
                Gtk::TreeModel::Row row = *it;
                row[list_columns.uuid] = el.uuid;
                row[list_columns.name] = el.name;
                row[list_columns.type] = el.type;
                row[list_columns.filename_remote] = el.filename_remote;
                row[list_columns.filename_local] = el.filename_local;
                row[list_columns.delta] = el.delta;
                row[list_columns.state] = el.state;
                row[list_columns.merge] = el.merge;
            }
        }

        item_store->thaw_notify();
        pool_item_view->set_model(item_store);
        item_store->set_sort_column(list_columns.state, Gtk::SORT_ASCENDING);
        set_sensitive(true);

        apply_update_state(*cb_update_layer_help, st->layer_help_needs_update);
        apply_update_state(*cb_update_tables, st->tables_needs_update);

        button_update->set_sensitive(st->can_update());
    }
    else {
        set_sensitive(false);
    }
}

void PoolMergeBox2::selection_changed()
{
    auto rows = pool_item_view->get_selection()->get_selected_rows();
    using ItemState = PoolStatusPoolManager::ItemInfo::ItemState;

    if (rows.size() == 1) {
        Gtk::TreeModel::Row row = *item_store->get_iter(rows.at(0));
        ItemState state = row[list_columns.state];
        ObjectType type = row[list_columns.type];
        if (state == ItemState::CURRENT) {
            stack->set_visible_child("up_to_date");
        }
        else if (state == ItemState::LOCAL_ONLY) {
            stack->set_visible_child("local_only");
        }
        else if (state == ItemState::REMOTE_ONLY) {
            stack->set_visible_child("remote_only");
        }
        else if (state == ItemState::MOVED) {
            stack->set_visible_child("moved");
        }
        else if (state == ItemState::CHANGED && type == ObjectType::MODEL_3D) {
            stack->set_visible_child("3d_changed");
        }
        else if (state == ItemState::CHANGED || state == ItemState::MOVED_CHANGED) {
            stack->set_visible_child("delta");
            const json &j = row.get_value(list_columns.delta);
            std::stringstream ss;
            ss << std::setw(4) << j;
            delta_text_view->get_buffer()->set_text(ss.str());
        }
    }
}

} // namespace horizon
