#include "imp_package.hpp"
#include "util/gtk_util.hpp"
#include "util/util.hpp"
#include "3d_view.hpp"
#include "widgets/spin_button_angle.hpp"
#include "board/board_layers.hpp"
#include "canvas3d/canvas3d.hpp"
#include "model_editor.hpp"
#include "place_model_box.hpp"

namespace horizon {

void ImpPackage::update_model_editors()
{
    if (core_package.models.count(current_model) == 0) {
        current_model = core_package.default_model;
    }
    auto children = models_listbox->get_children();
    for (auto ch : children) {
        auto ed = dynamic_cast<ModelEditor *>(dynamic_cast<Gtk::ListBoxRow *>(ch)->get_child());
        ed->set_is_default(core_package.default_model);
        ed->set_is_current(current_model);
    }
}

void ImpPackage::update_points()
{
    if (core_package.models.count(current_model)) {
        auto &model = core_package.models.at(current_model);
        auto &ca = view_3d_window->get_canvas();
        ca.set_point_model(model.filename);
        auto mat = mat_from_model(model, 1e-6);
        view_3d_window->get_canvas().set_point_transform(mat);
        view_3d_window->get_canvas().request_push();
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

void ImpPackage::update_fake_board()
{
    fake_board.polygons.clear();
    {
        auto uu = UUID::random();
        auto &poly = fake_board.polygons.emplace(uu, uu).first->second;
        poly.layer = BoardLayers::L_OUTLINE;

        auto bb = package.get_bbox();
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
        fake_package.package = package;
        fake_package.package.models = core_package.models;
        fake_package.pool_package = &package;
        fake_package.model = current_model;
    }
}

void ImpPackage::reload_model_editor()
{
    auto children = models_listbox->get_children();
    for (auto ch : children) {
        auto ed = dynamic_cast<ModelEditor *>(dynamic_cast<Gtk::ListBoxRow *>(ch)->get_child());
        if (ed->uu == current_model) {
            ed->reload();
        }
    }
}

void ImpPackage::construct_3d()
{
    auto view_3d_button = Gtk::manage(new Gtk::Button("3D"));
    main_window->header->pack_start(*view_3d_button);
    view_3d_button->show();
    view_3d_button->signal_clicked().connect([this] { trigger_action(ActionID::VIEW_3D); });

    fake_board.set_n_inner_layers(0);
    fake_board.stackup.at(0).substrate_thickness = 1.6_mm;

    view_3d_window = View3DWindow::create(fake_board, *pool.get(), View3DWindow::Mode::PACKAGE);
    view_3d_window->signal_request_update().connect(sigc::mem_fun(*this, &ImpPackage::update_fake_board));
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
        la->get_style_context()->add_class("dim-label");
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
                auto ed = Gtk::manage(new ModelEditor(*this, uu));
                ed->signal_changed().connect([this] { core_package.set_needs_save(); });
                models_listbox->append(*ed);
                ed->show();
                current_model = uu;
                view_3d_window->update();
                update_model_editors();
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
            ed->make_current();
        });
        sc->add(*models_listbox);

        current_model = core_package.default_model;
        for (auto &it : core_package.models) {
            auto ed = Gtk::manage(new ModelEditor(*this, it.first));
            ed->signal_changed().connect([this] { core_package.set_needs_save(); });
            models_listbox->append(*ed);
            ed->show();
        }

        models_box->pack_start(*sc, true, true, 0);
    }
    update_model_editors();

    models_box->show_all();

    view_3d_stack = Gtk::manage(new Gtk::Stack);
    view_3d_stack->add(*models_box, "models");
    view_3d_stack->show();

    place_model_box = Gtk::manage(new PlaceModelBox(*this));
    view_3d_window->get_canvas().signal_point_select().connect(
            [this](glm::dvec3 p) { place_model_box->handle_pick(p); });
    view_3d_stack->add(*place_model_box, "place");
    place_model_box->show();

    view_3d_window->add_widget(view_3d_stack);
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
