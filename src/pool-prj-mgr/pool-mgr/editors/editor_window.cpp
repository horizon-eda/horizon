#include "editor_window.hpp"
#include "unit_editor.hpp"
#include "entity_editor.hpp"
#include "part_editor.hpp"
#include "pool/unit.hpp"
#include "pool/entity.hpp"
#include "pool/part.hpp"
#include "util/util.hpp"
#include "util/str_util.hpp"
#include "pool/ipool.hpp"
#include "nlohmann/json.hpp"
#include "widgets/color_box.hpp"
#include "checks/check_entity.hpp"
#include "checks/check_unit.hpp"
#include "checks/check_part.hpp"

namespace horizon {

EditorWindowStore::EditorWindowStore(const std::string &fn) : filename(fn)
{
}

void EditorWindowStore::save()
{
    save_as(filename);
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
    Part part;
};

EditorWindow::EditorWindow(ObjectType ty, const std::string &filename, IPool *p, class PoolParametric *pp,
                           bool read_only, bool is_temp)
    : Gtk::Window(), type(ty), pool(*p), pool_parametric(pp),
      state_store(this, "pool-editor-win-" + std::to_string(static_cast<int>(type)))
{
    set_type_hint(Gdk::WINDOW_TYPE_HINT_DIALOG);
    auto hb = Gtk::manage(new Gtk::HeaderBar());
    set_titlebar(*hb);

    save_button = Gtk::manage(new Gtk::Button());
    if (read_only)
        save_button->set_sensitive(false);
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
    editor->show();
    add(*editor);
    editor->unreference();

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
    });
    run_checks();

    if (!state_store.get_default_set())
        set_default_size(-1, 600);

    signal_delete_event().connect([this, read_only](GdkEventAny *ev) {
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
    if (store->filename.size()) {
        if (iface)
            iface->save();
        store->save();
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

        if (gtk_native_dialog_run(GTK_NATIVE_DIALOG(native)) == GTK_RESPONSE_ACCEPT) {
            if (iface)
                iface->save();
            std::string fn = fix_filename(chooser->get_filename());
            s_signal_filename_changed.emit(fn);
            store->save_as(fn);
            s_signal_saved.emit(store->filename);
            save_button->set_label("Save");
            need_update = true;
        }
    }
}

std::string EditorWindow::fix_filename(std::string s)
{
    trim(s);
    if (!endswith(s, ".json")) {
        s += ".json";
    }
    return s;
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

} // namespace horizon
