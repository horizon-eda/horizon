#include "editor_base.hpp"
#include "util/file_version.hpp"
#include "preferences/preferences.hpp"
#include "preferences/preferences_provider.hpp"

namespace horizon {

PoolEditorBase::PoolEditorBase(const std::string &fn, class IPool &apool) : filename(fn), pool(apool)
{
    PreferencesProvider::get().signal_changed().connect(sigc::mem_fun(*this, &PoolEditorBase::apply_preferences));
}

void PoolEditorBase::apply_preferences()
{
    history_manager.set_never_forgets(PreferencesProvider::get_prefs().undo_redo.never_forgets);
}

unsigned int PoolEditorBase::get_required_version() const
{
    return get_version().get_app();
}

void PoolEditorBase::label_make_item_link(Gtk::Label &la, ObjectType type)
{
    la.set_track_visited_links(false);
    auto menu = Gtk::manage(new Gtk::Menu);
    menu->attach_to_widget(la);
    auto it = Gtk::manage(new Gtk::MenuItem("Edit"));
    menu->append(*it);
    it->show();
    it->signal_activate().connect([&la, type, this] {
        auto uri = la.get_current_uri();
        if (!uri.size())
            return;
        s_signal_open_item.emit(type, UUID(uri));
    });
    la.signal_button_press_event().connect(
            [&la, menu](GdkEventButton *ev) {
                if (!gdk_event_triggers_context_menu((GdkEvent *)ev))
                    return false;
                int start, end;
                if (la.get_selection_bounds(start, end))
                    return false;
                if (!la.get_current_uri().size())
                    return false;
                menu->popup_at_pointer((GdkEvent *)ev);
                return true;
            },
            false);
    la.signal_activate_link().connect(
            [this, type](const std::string &url) {
                s_signal_goto.emit(type, UUID(url));
                return true;
            },
            false);
}

bool PoolEditorBase::can_redo() const
{
    return history_manager.can_redo();
}

bool PoolEditorBase::can_undo() const
{
    return history_manager.can_undo();
}

void PoolEditorBase::history_append(const std::string &comment)
{
    history_manager.push(make_history_item(comment));
}

void PoolEditorBase::undo()
{
    if (!history_manager.can_undo())
        return;
    history_load(history_manager.undo());
    set_needs_save(true);
}

void PoolEditorBase::redo()
{
    if (!history_manager.can_redo())
        return;
    history_load(history_manager.redo());
    set_needs_save(true);
}


} // namespace horizon
