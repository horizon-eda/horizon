#include "editor_window.hpp"
#include "unit_editor.hpp"
#include "entity_editor.hpp"
#include "part_editor.hpp"
#include "pool/unit.hpp"
#include "pool/entity.hpp"
#include "pool/part.hpp"
#include "util/util.hpp"
#include "util/str_util.hpp"
#include "util/gtk_util.hpp"
#include "pool/ipool.hpp"
#include "nlohmann/json.hpp"
#include "widgets/color_box.hpp"
#include "checks/check_entity.hpp"
#include "checks/check_unit.hpp"
#include "checks/check_part.hpp"
#include "common/object_descr.hpp"
#include <iostream>

namespace horizon {

EditorWindowStore::EditorWindowStore(const std::string &fn) : filename(fn)
{
}

void EditorWindowStore::save()
{
    save_as(filename);
}

unsigned int EditorWindowStore::get_required_version() const
{
    return get_version().get_app();
}

class UnitStore : public EditorWindowStore {
public:
    UnitStore(const std::string &fn)
        : EditorWindowStore(fn), unit(filename.size() ? Unit::new_from_file(filename) : Unit(UUID::random()))
    {
    }
    void save_as(const std::string &fn) override
    {
        save_json_to_file(fn, unit.serialize());
        filename = fn;
    }
    std::string get_name() const override
    {
        return unit.name;
    }
    const UUID &get_uuid() const override
    {
        return unit.uuid;
    }
    RulesCheckResult run_checks() const override
    {
        return check_unit(unit);
    }
    const FileVersion &get_version() const override
    {
        return unit.version;
    }
    ObjectType get_type() const override
    {
        return ObjectType::UNIT;
    }
    Unit unit;
};

class EntityStore : public EditorWindowStore {
public:
    EntityStore(const std::string &fn, class IPool &pool)
        : EditorWindowStore(fn),
          entity(filename.size() ? Entity::new_from_file(filename, pool) : Entity(UUID::random()))
    {
    }
    void save_as(const std::string &fn) override
    {
        save_json_to_file(fn, entity.serialize());
        filename = fn;
    }
    std::string get_name() const override
    {
        return entity.name;
    }
    const UUID &get_uuid() const override
    {
        return entity.uuid;
    }
    RulesCheckResult run_checks() const override
    {
        return check_entity(entity);
    }
    const FileVersion &get_version() const override
    {
        return entity.version;
    }
    ObjectType get_type() const override
    {
        return ObjectType::ENTITY;
    }
    Entity entity;
};

class PartStore : public EditorWindowStore {
public:
    PartStore(const std::string &fn, class IPool &pool)
        : EditorWindowStore(fn), part(filename.size() ? Part::new_from_file(filename, pool) : Part(UUID::random()))
    {
    }
    void save_as(const std::string &fn) override
    {
        save_json_to_file(fn, part.serialize());
        filename = fn;
    }
    std::string get_name() const override
    {
        return part.get_MPN();
    }
    const UUID &get_uuid() const override
    {
        return part.uuid;
    }
    RulesCheckResult run_checks() const override
    {
        return check_part(part);
    }
    const FileVersion &get_version() const override
    {
        return part.version;
    }
    unsigned int get_required_version() const override
    {
        return part.get_required_version();
    }
    ObjectType get_type() const override
    {
        return ObjectType::PART;
    }
    Part part;
};

EditorWindow::EditorWindow(ObjectType ty, const std::string &filename, IPool *p, class PoolParametric *pp,
                           bool a_read_only, bool is_temp)
    : Gtk::Window(), type(ty), pool(*p), pool_parametric(pp),
      state_store(this, "pool-editor-win-" + std::to_string(static_cast<int>(type))), read_only(a_read_only)
{
    set_type_hint(Gdk::WINDOW_TYPE_HINT_DIALOG);
    auto hb = Gtk::manage(new Gtk::HeaderBar());
    set_titlebar(*hb);

    save_button = Gtk::manage(new Gtk::Button());
    hb->pack_start(*save_button);

    check_button = Gtk::manage(new Gtk::MenuButton());
    {
        auto box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 5));
        check_color_box = Gtk::manage(new ColorBox);
        check_color_box->set_valign(Gtk::ALIGN_CENTER);
        check_color_box->set_size_request(16, 16);
        box->pack_start(*check_color_box, false, false, 0);
        auto la = Gtk::manage(new Gtk::Label("Checks"));
        box->pack_start(*la, false, false, 0);
        check_button->add(*box);
    }
    check_popover = Gtk::manage(new Gtk::Popover);
    check_button->set_popover(*check_popover);
    check_label = Gtk::manage(new Gtk::Label("foo"));
    check_label->property_margin() = 10;
    check_popover->add(*check_label);
    check_label->show();
    hb->pack_end(*check_button);


    hb->show_all();
    hb->set_show_close_button(true);


    Gtk::Widget *editor = nullptr;
    switch (type) {
    case ObjectType::UNIT: {
        auto st = new UnitStore(filename);
        store.reset(st);
        auto ed = UnitEditor::create(st->unit, pool);
        editor = ed;
        iface = ed;
        hb->set_title("Unit Editor");
    } break;
    case ObjectType::ENTITY: {
        auto st = new EntityStore(filename, pool);
        store.reset(st);
        auto ed = EntityEditor::create(st->entity, pool);
        editor = ed;
        iface = ed;
        hb->set_title("Entity Editor");
    } break;
    case ObjectType::PART: {
        auto st = new PartStore(filename, pool);
        store.reset(st);
        auto ed = PartEditor::create(st->part, pool, *pool_parametric);
        editor = ed;
        iface = ed;
        hb->set_title("Part Editor");
    } break;
    default:;
    }

    auto box = Gtk::manage(new Gtk::Box(Gtk::Orientation::ORIENTATION_VERTICAL, 0));
    add(*box);
    box->show();

    {
        info_bar = Gtk::manage(new Gtk::InfoBar());
        info_bar_label = Gtk::manage(new Gtk::Label);
        info_bar_label->set_line_wrap(true);
        dynamic_cast<Gtk::Container &>(*info_bar->get_content_area()).add(*info_bar_label);
        info_bar_label->show();
        info_bar->show();
        box->pack_start(*info_bar, false, false, 0);
        info_bar_hide(info_bar);
    }

    {
        const auto &version = store->get_version();
        if (version.get_app() < version.get_file()) {
            info_bar_label->set_markup(version.get_message(store->get_type()));
            read_only = true;
            info_bar_show(info_bar);
        }
    }
    saved_version = store->get_version().get_file();

    editor->show();
    box->pack_start(*editor, true, true, 0);
    editor->unreference();

    if (read_only)
        save_button->set_sensitive(false);

    if (is_temp) {
        Gio::File::create_for_path(filename)->remove();
        store->filename.clear();
    }

    if (store->filename.size()) {
        save_button->set_label("Save");
    }
    else {
        save_button->set_label("Save as..");
    }

    std::cout << __LINE__ << " " << __FUNCTION__ << " " << iface << std::endl;
    assert(iface);
    iface->signal_goto().connect([this](ObjectType t, UUID uu) { s_signal_goto.emit(t, uu); });
    if (!read_only) {
        save_button->set_sensitive(iface->get_needs_save());
        iface->signal_needs_save().connect([this] { save_button->set_sensitive(iface->get_needs_save()); });
    }
    iface->signal_needs_save().connect([this] {
        if (iface->get_needs_save()) {
            run_checks();
        }
        update_version_warning();
    });
    run_checks();

    if (!state_store.get_default_set())
        set_default_size(-1, 600);

    signal_delete_event().connect([this](GdkEventAny *ev) {
        std::cout << __LINE__ << " " << __FUNCTION__ << " " << iface << std::endl;
        if (iface && iface->get_needs_save()) {
            if (!read_only) { // not read only
                Gtk::MessageDialog md(*this, "Save changes before closing?", false /* use_markup */,
                                      Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_NONE);
                md.set_secondary_text(
                        "If you don't save, all your changes will be permanently "
                        "lost.");
                md.add_button("Close without Saving", 1);
                md.add_button("Cancel", Gtk::RESPONSE_CANCEL);
                md.add_button("Save", 2);
                switch (md.run()) {
                case 1:
                    return false; // close

                case 2:
                    save();
                    return false; // close

                default:
                    return true; // keep window open
                }
                return false;
            }
            else {
                Gtk::MessageDialog md(*this, "Document is read only", false /* use_markup */, Gtk::MESSAGE_QUESTION,
                                      Gtk::BUTTONS_NONE);
                md.set_secondary_text("You won't be able to save your changes");
                md.add_button("Close without Saving", 1);
                md.add_button("Cancel", Gtk::RESPONSE_CANCEL);
                switch (md.run()) {
                case 1:
                    return false; // close

                default:
                    return true; // keep window open
                }
                return false;
            }
        }
        return false;
    });

    save_button->signal_clicked().connect(sigc::mem_fun(*this, &EditorWindow::save));
}

void EditorWindow::update_version_warning()
{
    if (read_only)
        return;
    if (store->get_required_version() > saved_version) {
        const auto &t = object_descriptions.at(store->get_type()).name;
        info_bar_label->set_markup("Saving this " + t + " will update it from version " + std::to_string(saved_version)
                                   + " to " + std::to_string(store->get_required_version()) + " . "
                                   + FileVersion::learn_more_markup);
        info_bar_show(info_bar);
    }
    else {
        info_bar_hide(info_bar);
    }
}

void EditorWindow::force_close()
{
    hide();
}

bool EditorWindow::get_needs_save() const
{
    return iface->get_needs_save();
}

void EditorWindow::set_original_filename(const std::string &s)
{
    original_filename = s;
}

void EditorWindow::save()
{
    std::cout << __LINE__ << " " << __FUNCTION__ << " " << iface << std::endl;
    if (read_only)
        return;
    if (store->filename.size()) {
        if (iface)
            iface->save();
        store->save();
        saved_version = store->get_required_version();
        need_update = true;
        s_signal_saved.emit(store->filename);
    }
    else {
        GtkFileChooserNative *native = gtk_file_chooser_native_new("Save", GTK_WINDOW(gobj()),
                                                                   GTK_FILE_CHOOSER_ACTION_SAVE, "_Save", "_Cancel");
        auto chooser = Glib::wrap(GTK_FILE_CHOOSER(native));
        chooser->set_do_overwrite_confirmation(true);
        chooser->set_current_name(store->get_name() + ".json");
        switch (type) {
        case ObjectType::UNIT:
            chooser->set_current_folder(Glib::build_filename(pool.get_base_path(), "units"));
            break;
        case ObjectType::ENTITY:
            chooser->set_current_folder(Glib::build_filename(pool.get_base_path(), "entities"));
            break;
        case ObjectType::PART:
            chooser->set_current_folder(Glib::build_filename(pool.get_base_path(), "parts"));
            break;
        default:;
        }
        if (original_filename.size())
            chooser->set_current_folder(Glib::path_get_dirname(original_filename));

        std::cout << __LINE__ << " " << __FUNCTION__ << std::endl;
#ifdef G_OS_WIN32
        set_sensitive(false);
#endif
        const auto rc = gtk_native_dialog_run(GTK_NATIVE_DIALOG(native));
#ifdef G_OS_WIN32
        set_sensitive(true);
#endif
        std::cout << __LINE__ << " " << __FUNCTION__ << " " << rc << std::endl;
        if (rc == GTK_RESPONSE_ACCEPT) {
            std::cout << __LINE__ << std::endl;
            if (iface)
                iface->save();
            std::cout << __LINE__ << " " << __FUNCTION__ << std::endl;
            std::string fn = append_dot_json(chooser->get_filename());
            std::cout << __LINE__ << " " << __FUNCTION__ << std::endl;
            s_signal_filename_changed.emit(fn);
            std::cout << __LINE__ << " " << __FUNCTION__ << std::endl;
            store->save_as(fn);
            std::cout << __LINE__ << " " << __FUNCTION__ << std::endl;
            saved_version = store->get_required_version();
            std::cout << __LINE__ << " " << __FUNCTION__ << std::endl;
            s_signal_saved.emit(store->filename);
            std::cout << __LINE__ << " " << __FUNCTION__ << std::endl;
            save_button->set_label("Save");
            std::cout << __LINE__ << " " << __FUNCTION__ << std::endl;
            need_update = true;
            std::cout << __LINE__ << " " << __FUNCTION__ << std::endl;
        }
    }
    if (info_bar)
        info_bar_hide(info_bar);
}

std::string EditorWindow::get_filename() const
{
    return store->filename;
}

void EditorWindow::reload()
{
    if (iface) {
        iface->reload();
    }
}

bool EditorWindow::get_need_update() const
{
    return need_update;
}

ObjectType EditorWindow::get_object_type() const
{
    return type;
}

void EditorWindow::select(const ItemSet &items)
{
    if (iface)
        iface->select(items);
}

const UUID &EditorWindow::get_uuid() const
{
    return store->get_uuid();
}

void EditorWindow::run_checks()
{
    auto r = store->run_checks();
    check_color_box->set_color(rules_check_error_level_to_color(r.level));
    std::string s;
    if (r.level == RulesCheckErrorLevel::PASS) {
        s += "Checks passed";
    }
    else {
        s += "Checks didn't pass";
    }
    s += "\n";
    for (const auto &it : r.errors) {
        s += " • " + it.comment + "\n";
    }
    trim(s);
    check_label->set_text(s);
}

EditorWindow::~EditorWindow()
{
    std::cout << __FUNCTION__ << std::endl;
}

} // namespace horizon
