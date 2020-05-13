#include "preferences_window_pool.hpp"
#include "pool/pool_manager.hpp"
#include "util/gtk_util.hpp"

namespace horizon {
class PoolItemEditor : public Gtk::Box {
public:
    PoolItemEditor(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x, const PoolManagerPool &pool)
        : Gtk::Box(cobject)
    {
        x->get_widget("pool_item_name", label_name);
        x->get_widget("pool_item_path", label_path);
        x->get_widget("pool_item_uuid", label_uuid);
        x->get_widget("pool_item_delete", button_delete);
        x->get_widget("pool_item_enabled", switch_enabled);
        x->get_widget("pool_item_box_box", box);
        label_name->set_text(pool.name);
        label_path->set_text(pool.base_path);
        label_uuid->set_text((std::string)pool.uuid);
        switch_enabled->set_active(pool.enabled);
        show_all();
    }
    static PoolItemEditor *create(const PoolManagerPool &pool)
    {
        PoolItemEditor *w;
        Glib::RefPtr<Gtk::Builder> x = Gtk::Builder::create();
        std::vector<Glib::ustring> objs = {"pool_item_box", "image1"};
        x->add_from_resource("/org/horizon-eda/horizon/pool-prj-mgr/prj-mgr/prefs.ui", objs);
        x->get_widget_derived("pool_item_box", w, pool);

        w->reference();
        return w;
    }
    Gtk::Button *button_delete;
    Gtk::Switch *switch_enabled;
    Gtk::Box *box = nullptr;

private:
    Gtk::Label *label_name;
    Gtk::Label *label_path;
    Gtk::Label *label_uuid;
};

PoolPreferencesEditor::PoolPreferencesEditor(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x)
    : Gtk::ScrolledWindow(cobject), mgr(PoolManager::get())
{
    x->get_widget("listbox", listbox);
    listbox->set_header_func(&header_func_separator);
    size_group = Gtk::SizeGroup::create(Gtk::SIZE_GROUP_HORIZONTAL);
    show_all();

    listbox->signal_row_activated().connect([this](Gtk::ListBoxRow *row) { add_pool(""); });
    update();
}

void PoolPreferencesEditor::add_pool(const std::string &path)
{
    auto top = dynamic_cast<Gtk::Window *>(get_ancestor(GTK_TYPE_WINDOW));
    GtkFileChooserNative *native =
            gtk_file_chooser_native_new("Add Pool", top->gobj(), GTK_FILE_CHOOSER_ACTION_OPEN, "_Open", "_Cancel");
    auto chooser = Glib::wrap(GTK_FILE_CHOOSER(native));
    auto filter = Gtk::FileFilter::create();
    filter->set_name("Horizon pool (pool.json)");
    filter->add_pattern("pool.json");
    chooser->add_filter(filter);
    if (path.size())
        chooser->set_filename(path);

    if (gtk_native_dialog_run(GTK_NATIVE_DIALOG(native)) == GTK_RESPONSE_ACCEPT) {
        auto cpath = chooser->get_file()->get_parent()->get_path();
        mgr.add_pool(cpath);
        update();
    }
}

PoolPreferencesEditor *PoolPreferencesEditor::create()
{
    PoolPreferencesEditor *w;
    Glib::RefPtr<Gtk::Builder> x = Gtk::Builder::create();
    x->add_from_resource("/org/horizon-eda/horizon/pool-prj-mgr/prj-mgr/prefs.ui", "box");
    x->get_widget_derived("box", w);

    w->reference();
    return w;
}

void PoolPreferencesEditor::update()
{
    auto children = listbox->get_children();
    for (auto it : children) {
        delete it;
    }
    auto pools = mgr.get_pools();
    for (const auto &it : pools) {
        auto x = PoolItemEditor::create(it.second);
        auto row = Gtk::manage(new Gtk::ListBoxRow);
        row->add(*x);
        row->show_all();
        listbox->add(*row);
        row->set_activatable(false);
        size_group->add_widget(*x->box);
        x->unreference();
        std::string bp = it.first;
        x->button_delete->signal_clicked().connect([bp, this] {
            mgr.remove_pool(bp);
            update();
        });
        x->switch_enabled->property_active().signal_changed().connect([bp, this, x] {
            mgr.set_pool_enabled(bp, x->switch_enabled->get_active());
            update();
        });
    }
    {
        auto la = Gtk::manage(new Gtk::Label("Add pool…"));
        la->property_margin() = 10;
        la->show();
        listbox->add(*la);
    }
}
} // namespace horizon
