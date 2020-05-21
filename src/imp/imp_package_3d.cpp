#include "imp_package.hpp"
#include "util/gtk_util.hpp"
#include "util/util.hpp"
#include "3d_view.hpp"
#include "widgets/spin_button_angle.hpp"
#include "board/board_layers.hpp"

namespace horizon {

class ModelEditor : public Gtk::Box, public Changeable {
public:
    ModelEditor(ImpPackage *iimp, const UUID &iuu);
    const UUID uu;
    void update_all();

private:
    ImpPackage *imp;
    Gtk::CheckButton *default_cb = nullptr;
    Gtk::Label *current_label = nullptr;

    SpinButtonDim *sp_x = nullptr;
    SpinButtonDim *sp_y = nullptr;
    SpinButtonDim *sp_z = nullptr;

    class SpinButtonAngle *sp_roll = nullptr;
    class SpinButtonAngle *sp_pitch = nullptr;
    class SpinButtonAngle *sp_yaw = nullptr;
};

static Gtk::Label *make_label(const std::string &text)
{
    auto la = Gtk::manage(new Gtk::Label(text));
    la->get_style_context()->add_class("dim-label");
    la->set_halign(Gtk::ALIGN_END);
    return la;
}


class PlaceAtPadDialog : public Gtk::Dialog {
public:
    PlaceAtPadDialog(Package *pkg);
    UUID selected_pad;

private:
    Package *pkg;
};

class MyLabel : public Gtk::Label {
public:
    MyLabel(const std::string &txt, const UUID &uu) : Gtk::Label(txt), uuid(uu)
    {
        set_xalign(0);
        property_margin() = 5;
    }

    const UUID uuid;
};

PlaceAtPadDialog::PlaceAtPadDialog(Package *p)
    : Gtk::Dialog("Place at pad", Gtk::DIALOG_MODAL | Gtk::DIALOG_USE_HEADER_BAR), pkg(p)
{
    set_default_size(200, 400);
    auto sc = Gtk::manage(new Gtk::ScrolledWindow);
    sc->set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC);

    auto lb = Gtk::manage(new Gtk::ListBox);
    lb->set_selection_mode(Gtk::SELECTION_NONE);
    lb->set_activate_on_single_click(true);
    lb->set_header_func(sigc::ptr_fun(header_func_separator));
    sc->add(*lb);

    std::vector<const Pad *> pads;
    for (const auto &it : pkg->pads) {
        pads.push_back(&it.second);
    }
    std::sort(pads.begin(), pads.end(), [](auto a, auto b) { return strcmp_natural(a->name, b->name) < 0; });

    for (const auto it : pads) {
        auto la = Gtk::manage(new MyLabel(it->name, it->uuid));
        lb->append(*la);
    }
    lb->signal_row_activated().connect([this](Gtk::ListBoxRow *row) {
        auto la = dynamic_cast<MyLabel *>(row->get_child());
        selected_pad = la->uuid;
        response(Gtk::RESPONSE_OK);
    });

    sc->show_all();
    get_content_area()->set_border_width(0);
    get_content_area()->pack_start(*sc, true, true, 0);
}

ModelEditor::ModelEditor(ImpPackage *iimp, const UUID &iuu) : Gtk::Box(Gtk::ORIENTATION_VERTICAL, 5), uu(iuu), imp(iimp)
{
    auto &model = imp->core_package.models.at(uu);
    property_margin() = 10;
    auto entry = Gtk::manage(new Gtk::Entry);
    pack_start(*entry, false, false, 0);
    entry->show();
    entry->set_width_chars(45);
    entry->signal_focus_in_event().connect([this](GdkEventFocus *ev) {
        imp->current_model = uu;
        imp->view_3d_window->update();
        update_all();
        return false;
    });
    bind_widget(entry, model.filename);
    entry->signal_changed().connect([this] { s_signal_changed.emit(); });

    {
        auto box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 5));
        auto delete_button = Gtk::manage(new Gtk::Button);
        delete_button->set_image_from_icon_name("list-remove-symbolic", Gtk::ICON_SIZE_BUTTON);
        delete_button->signal_clicked().connect([this] {
            imp->core_package.models.erase(uu);
            if (imp->core_package.default_model == uu) {
                if (imp->core_package.models.size()) {
                    imp->core_package.default_model = imp->core_package.models.begin()->first;
                }
                else {
                    imp->core_package.default_model = UUID();
                }
            }
            s_signal_changed.emit();
            update_all();
            delete this->get_parent();
        });
        box->pack_end(*delete_button, false, false, 0);

        auto browse_button = Gtk::manage(new Gtk::Button("Browse..."));
        browse_button->signal_clicked().connect([this, entry] {
            Package::Model *model2 = nullptr;
            if (imp->core_package.models.count(uu)) {
                model2 = &imp->core_package.models.at(uu);
            }
            auto mfn = imp->ask_3d_model_filename(model2 ? model2->filename : "");
            if (mfn.size()) {
                entry->set_text(mfn);
                imp->view_3d_window->update(true);
            }
        });
        box->pack_end(*browse_button, false, false, 0);

        default_cb = Gtk::manage(new Gtk::CheckButton("Default"));
        if (imp->core_package.default_model == uu) {
            default_cb->set_active(true);
        }
        default_cb->signal_toggled().connect([this] {
            if (default_cb->get_active()) {
                imp->core_package.default_model = uu;
                imp->current_model = uu;
            }
            imp->view_3d_window->update();
            s_signal_changed.emit();
            update_all();
        });
        box->pack_start(*default_cb, false, false, 0);

        current_label = Gtk::manage(new Gtk::Label("Current"));
        current_label->get_style_context()->add_class("dim-label");
        current_label->set_no_show_all(true);
        box->pack_start(*current_label, false, false, 0);

        box->show_all();
        pack_start(*box, false, false, 0);
    }

    {
        auto box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 5));
        auto reset_button = Gtk::manage(new Gtk::Button("Reset placement"));
        reset_button->signal_clicked().connect([this] {
            sp_x->set_value(0);
            sp_y->set_value(0);
            sp_z->set_value(0);
            sp_roll->set_value(0);
            sp_pitch->set_value(0);
            sp_yaw->set_value(0);
        });
        box->pack_start(*reset_button, false, false, 0);

        auto place_at_pad_button = Gtk::manage(new Gtk::Button("Place at pad"));
        place_at_pad_button->signal_clicked().connect([this] {
            auto pkg = imp->core_package.get_package();
            PlaceAtPadDialog dia(pkg);
            dia.set_transient_for(*imp->view_3d_window);
            if (dia.run() == Gtk::RESPONSE_OK) {
                auto &pad = pkg->pads.at(dia.selected_pad);
                sp_x->set_value(pad.placement.shift.x);
                sp_y->set_value(pad.placement.shift.y);
            }
        });
        box->pack_start(*place_at_pad_button, false, false, 0);

        box->show_all();
        pack_start(*box, false, false, 0);
    }

    auto placement_grid = Gtk::manage(new Gtk::Grid);
    placement_grid->set_hexpand_set(true);
    placement_grid->set_row_spacing(5);
    placement_grid->set_column_spacing(5);
    placement_grid->attach(*make_label("X"), 0, 0, 1, 1);
    placement_grid->attach(*make_label("Y"), 0, 1, 1, 1);
    placement_grid->attach(*make_label("Z"), 0, 2, 1, 1);
    std::set<Gtk::SpinButton *> placement_spin_buttons;
    {
        auto sp = Gtk::manage(new SpinButtonDim);
        sp->set_range(-1000_mm, 1000_mm);
        placement_grid->attach(*sp, 1, 0, 1, 1);
        bind_widget(sp, model.x);
        placement_spin_buttons.insert(sp);
        sp_x = sp;
    }
    {
        auto sp = Gtk::manage(new SpinButtonDim);
        sp->set_range(-1000_mm, 1000_mm);
        placement_grid->attach(*sp, 1, 1, 1, 1);
        bind_widget(sp, model.y);
        placement_spin_buttons.insert(sp);
        sp_y = sp;
    }
    {
        auto sp = Gtk::manage(new SpinButtonDim);
        sp->set_range(-1000_mm, 1000_mm);
        placement_grid->attach(*sp, 1, 2, 1, 1);
        bind_widget(sp, model.z);
        placement_spin_buttons.insert(sp);
        sp_z = sp;
    }

    {
        auto la = make_label("Roll");
        la->set_hexpand(true);
        placement_grid->attach(*la, 2, 0, 1, 1);
    }
    placement_grid->attach(*make_label("Pitch"), 2, 1, 1, 1);
    placement_grid->attach(*make_label("Yaw"), 2, 2, 1, 1);
    {
        auto sp = Gtk::manage(new SpinButtonAngle);
        placement_grid->attach(*sp, 3, 0, 1, 1);
        bind_widget(sp, model.roll);
        placement_spin_buttons.insert(sp);
        sp_roll = sp;
    }
    {
        auto sp = Gtk::manage(new SpinButtonAngle);
        placement_grid->attach(*sp, 3, 1, 1, 1);
        bind_widget(sp, model.pitch);
        placement_spin_buttons.insert(sp);
        sp_pitch = sp;
    }
    {
        auto sp = Gtk::manage(new SpinButtonAngle);
        placement_grid->attach(*sp, 3, 2, 1, 1);
        bind_widget(sp, model.yaw);
        placement_spin_buttons.insert(sp);
        sp_yaw = sp;
    }
    for (auto sp : placement_spin_buttons) {
        sp->signal_value_changed().connect([this] {
            imp->view_3d_window->update();
            s_signal_changed.emit();
        });
    }

    placement_grid->show_all();
    pack_start(*placement_grid, false, false, 0);

    update_all();
}


void ModelEditor::update_all()
{
    if (imp->core_package.models.count(imp->current_model) == 0) {
        imp->current_model = imp->core_package.default_model;
    }
    auto children = imp->models_listbox->get_children();
    for (auto ch : children) {
        auto ed = dynamic_cast<ModelEditor *>(dynamic_cast<Gtk::ListBoxRow *>(ch)->get_child());
        ed->default_cb->set_active(imp->core_package.default_model == ed->uu);
        ed->current_label->set_visible(imp->current_model == ed->uu);
    }
}

std::string ImpPackage::ask_3d_model_filename(const std::string &current_filename)
{
    GtkFileChooserNative *native = gtk_file_chooser_native_new("Open", GTK_WINDOW(view_3d_window->gobj()),
                                                               GTK_FILE_CHOOSER_ACTION_OPEN, "_Open", "_Cancel");
    auto chooser = Glib::wrap(GTK_FILE_CHOOSER(native));
    auto filter = Gtk::FileFilter::create();
    filter->set_name("STEP models");
    filter->add_pattern("*.step");
    filter->add_pattern("*.STEP");
    filter->add_pattern("*.stp");
    filter->add_pattern("*.STP");
    chooser->add_filter(filter);
    if (current_filename.size()) {
        chooser->set_filename(Glib::build_filename(pool->get_base_path(), current_filename));
    }
    else {
        chooser->set_current_folder(pool->get_base_path());
    }

    while (1) {
        if (gtk_native_dialog_run(GTK_NATIVE_DIALOG(native)) == GTK_RESPONSE_ACCEPT) {
            auto base_path = Gio::File::create_for_path(pool->get_base_path());
            std::string rel = base_path->get_relative_path(chooser->get_file());
            if (rel.size()) {
                replace_backslash(rel);
                return rel;
            }
            else {
                Gtk::MessageDialog md(*view_3d_window, "Model has to be in the pool directory", false /* use_markup */,
                                      Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK);
                md.run();
            }
        }
        else {
            return "";
        }
    }
    return "";
}

void ImpPackage::construct_3d()
{
    auto view_3d_button = Gtk::manage(new Gtk::Button("3D"));
    main_window->header->pack_start(*view_3d_button);
    view_3d_button->show();
    view_3d_button->signal_clicked().connect([this] { trigger_action(ActionID::VIEW_3D); });

    fake_board.set_n_inner_layers(0);
    fake_board.stackup.at(0).substrate_thickness = 1.6_mm;

    view_3d_window = View3DWindow::create(&fake_board, pool.get());
    view_3d_window->signal_request_update().connect([this] {
        fake_board.polygons.clear();
        {
            auto uu = UUID::random();
            auto &poly = fake_board.polygons.emplace(uu, uu).first->second;
            poly.layer = BoardLayers::L_OUTLINE;

            auto bb = core_package.get_package()->get_bbox();
            bb.first -= Coordi(5_mm, 5_mm);
            bb.second += Coordi(5_mm, 5_mm);

            poly.vertices.emplace_back(bb.first);
            poly.vertices.emplace_back(Coordi(bb.first.x, bb.second.y));
            poly.vertices.emplace_back(bb.second);
            poly.vertices.emplace_back(Coordi(bb.second.x, bb.first.y));
        }

        fake_board.packages.clear();
        {
            auto uu = UUID::random();
            auto &fake_package = fake_board.packages.emplace(uu, uu).first->second;
            fake_package.package = *core_package.get_package();
            fake_package.package.models = core_package.models;
            fake_package.pool_package = core_package.get_package();
            fake_package.model = current_model;
        }
    });
    view_3d_window->signal_key_press_event().connect([this](GdkEventKey *ev) {
        if (ev->keyval == GDK_KEY_Escape) {
            main_window->present();
            return true;
        }
        return false;
    });

    auto models_box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL));
    {
        auto box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 5));
        box->property_margin() = 10;
        auto la = Gtk::manage(new Gtk::Label);
        la->set_markup("<b>3D Models</b>");
        la->set_xalign(0);
        box->pack_start(*la, false, false, 0);

        auto button_reload = Gtk::manage(new Gtk::Button("Reload models"));
        button_reload->signal_clicked().connect([this] { view_3d_window->update(true); });
        box->pack_end(*button_reload, false, false, 0);

        auto button_add = Gtk::manage(new Gtk::Button("Add model"));
        button_add->signal_clicked().connect([this] {
            auto mfn = ask_3d_model_filename();
            if (mfn.size()) {
                auto uu = UUID::random();
                if (core_package.models.size() == 0) { // first
                    core_package.default_model = uu;
                }
                core_package.models.emplace(std::piecewise_construct, std::forward_as_tuple(uu),
                                            std::forward_as_tuple(uu, mfn));
                auto ed = Gtk::manage(new ModelEditor(this, uu));
                ed->signal_changed().connect([this] { core_package.set_needs_save(); });
                models_listbox->append(*ed);
                ed->show();
                current_model = uu;
                view_3d_window->update();
                ed->update_all();
                core_package.set_needs_save();
            }
        });
        box->pack_end(*button_add, false, false, 0);

        models_box->pack_start(*box, false, false, 0);
    }
    {
        auto sep = Gtk::manage(new Gtk::Separator(Gtk::ORIENTATION_HORIZONTAL));
        models_box->pack_start(*sep, false, false, 0);
    }
    {
        auto sc = Gtk::manage(new Gtk::ScrolledWindow);
        sc->set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC);

        models_listbox = Gtk::manage(new Gtk::ListBox);
        models_listbox->set_selection_mode(Gtk::SELECTION_NONE);
        models_listbox->set_header_func(sigc::ptr_fun(header_func_separator));
        models_listbox->set_activate_on_single_click(true);
        models_listbox->signal_row_activated().connect([this](Gtk::ListBoxRow *row) {
            auto ed = dynamic_cast<ModelEditor *>(row->get_child());
            current_model = ed->uu;
            view_3d_window->update();
            ed->update_all();
        });
        sc->add(*models_listbox);

        current_model = core_package.default_model;
        for (auto &it : core_package.models) {
            auto ed = Gtk::manage(new ModelEditor(this, it.first));
            ed->signal_changed().connect([this] { core_package.set_needs_save(); });
            models_listbox->append(*ed);
            ed->show();
        }

        models_box->pack_start(*sc, true, true, 0);
    }

    models_box->show_all();
    view_3d_window->add_widget(models_box);
    {
        auto sep = Gtk::manage(new Gtk::Separator(Gtk::ORIENTATION_HORIZONTAL));
        sep->show();
        view_3d_window->add_widget(sep);
    }

    connect_action(ActionID::VIEW_3D, [this](const auto &a) {
        view_3d_window->update();
        view_3d_window->present();
    });
}
} // namespace horizon
