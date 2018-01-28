#include "prj-mgr-prefs.hpp"
#include "prj-mgr-app.hpp"

namespace horizon {

class ProjectManagerPrefsPoolItem : public Gtk::Box {
public:
    ProjectManagerPrefsPoolItem(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x) : Gtk::Box(cobject)
    {
        x->get_widget("pool_item_name", label_name);
        x->get_widget("pool_item_path", label_path);
        x->get_widget("pool_item_uuid", label_uuid);
        x->get_widget("pool_item_delete", button_delete);
        show_all();
    }
    static ProjectManagerPrefsPoolItem *create()
    {
        ProjectManagerPrefsPoolItem *w;
        Glib::RefPtr<Gtk::Builder> x = Gtk::Builder::create();
        std::vector<Glib::ustring> objs = {"pool_item_box", "image1"};
        x->add_from_resource("/net/carrotIndustries/horizon/src/prj-mgr/prefs.ui", objs);
        x->get_widget_derived("pool_item_box", w);

        w->reference();
        return w;
    }
    void load(const ProjectManagerPool &pool)
    {
        label_name->set_text(pool.name);
        label_path->set_text(pool.path);
        label_uuid->set_text((std::string)pool.uuid);
    }
    Gtk::Button *button_delete;

private:
    Gtk::Label *label_name;
    Gtk::Label *label_path;
    Gtk::Label *label_uuid;
};

class ProjectManagerPrefsBox : public Gtk::Notebook {
public:
    ProjectManagerPrefsBox(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x);
    static ProjectManagerPrefsBox *create();
    Gtk::ListBox *listbox = nullptr;
    Gtk::Button *button_add_pool = nullptr;

private:
};

ProjectManagerPrefsBox::ProjectManagerPrefsBox(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x)
    : Gtk::Notebook(cobject)
{
    x->get_widget("listbox", listbox);
    x->get_widget("button_add_pool", button_add_pool);
    show_all();
}

ProjectManagerPrefsBox *ProjectManagerPrefsBox::create()
{
    ProjectManagerPrefsBox *w;
    Glib::RefPtr<Gtk::Builder> x = Gtk::Builder::create();
    x->add_from_resource("/net/carrotIndustries/horizon/src/prj-mgr/prefs.ui", "box");
    x->get_widget_derived("box", w);

    w->reference();
    return w;
}

void ProjectManagerPrefs::update()
{
    auto mapp = Glib::RefPtr<ProjectManagerApplication>::cast_dynamic(app);
    auto children = box->listbox->get_children();
    for (auto it : children) {
        delete it;
    }
    for (const auto &it : mapp->pools) {
        auto x = ProjectManagerPrefsPoolItem::create();
        x->load(it.second);
        box->listbox->add(*x);
        x->unreference();
        auto uu = it.first;
        x->button_delete->signal_clicked().connect([uu, this, mapp] {
            mapp->pools.erase(uu);
            update();
        });
    }
}

ProjectManagerPrefs::ProjectManagerPrefs(Gtk::ApplicationWindow *parent)
    : Gtk::Dialog("Preferences", *parent, Gtk::DialogFlags::DIALOG_MODAL | Gtk::DialogFlags::DIALOG_USE_HEADER_BAR),
      app(parent->get_application())
{

    /*Gtk::Button *button_ok = add_button("OK", Gtk::ResponseType::RESPONSE_OK);
    add_button("Cancel", Gtk::ResponseType::RESPONSE_CANCEL);
    set_default_response(Gtk::ResponseType::RESPONSE_OK);*/
    set_default_size(300, 300);

    // button_ok->signal_clicked().connect(sigc::mem_fun(this,
    // &PoolBrowserDialogPart::ok_clicked));

    box = ProjectManagerPrefsBox::create();
    get_content_area()->pack_start(*box, true, true, 0);
    get_content_area()->set_border_width(0);
    box->unreference();
    update();

    box->button_add_pool->signal_clicked().connect([this] {
        GtkFileChooserNative *native = gtk_file_chooser_native_new("Add Pool", GTK_WINDOW(gobj()),
                                                                   GTK_FILE_CHOOSER_ACTION_OPEN, "_Open", "_Cancel");
        auto chooser = Glib::wrap(GTK_FILE_CHOOSER(native));
        auto filter = Gtk::FileFilter::create();
        filter->set_name("Horizon pool (pool.json)");
        filter->add_pattern("pool.json");
        chooser->add_filter(filter);

        if (gtk_native_dialog_run(GTK_NATIVE_DIALOG(native)) == GTK_RESPONSE_ACCEPT) {
            auto path = chooser->get_filename();
            auto mapp = Glib::RefPtr<ProjectManagerApplication>::cast_dynamic(app);
            mapp->add_pool(path);
            update();
        }
    });

    get_content_area()->show_all();
}
} // namespace horizon
