#include "imp_package.hpp"
#include "3d_view.hpp"
#include "board/board_layers.hpp"
#include "canvas/canvas_gl.hpp"
#include "footprint_generator/footprint_generator_window.hpp"
#include "header_button.hpp"
#include "parameter_window.hpp"
#include "pool/part.hpp"
#include "util/gtk_util.hpp"
#include "util/util.hpp"
#include "util/pool_completion.hpp"
#include "util/changeable.hpp"
#include "util/str_util.hpp"
#include "widgets/pool_browser_button.hpp"
#include "widgets/pool_browser.hpp"
#include "widgets/spin_button_dim.hpp"
#include "widgets/parameter_set_editor.hpp"
#include "widgets/tag_entry.hpp"
#include "widgets/layer_box.hpp"
#include "widgets/layer_help_box.hpp"
#include "widgets/spin_button_angle.hpp"
#include "hud_util.hpp"
#include <iomanip>

namespace horizon {
ImpPackage::ImpPackage(const std::string &package_filename, const std::string &pool_path)
    : ImpLayer(pool_path), core_package(package_filename, *pool), fake_block(UUID::random()),
      fake_board(UUID::random(), fake_block)
{
    core = &core_package;
    core_package.signal_tool_changed().connect(sigc::mem_fun(*this, &ImpBase::handle_tool_change));
}

void ImpPackage::canvas_update()
{
    canvas->update(*core_package.get_canvas_data());
    warnings_box->update(core_package.get_package()->warnings);
    update_highlights();
}

void ImpPackage::apply_preferences()
{
    if (view_3d_window)
        view_3d_window->set_smooth_zoom(preferences.zoom.smooth_zoom_3d);
    ImpLayer::apply_preferences();
}

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
        sp->set_range(-100_mm, 100_mm);
        placement_grid->attach(*sp, 1, 0, 1, 1);
        bind_widget(sp, model.x);
        placement_spin_buttons.insert(sp);
        sp_x = sp;
    }
    {
        auto sp = Gtk::manage(new SpinButtonDim);
        sp->set_range(-100_mm, 100_mm);
        placement_grid->attach(*sp, 1, 1, 1, 1);
        bind_widget(sp, model.y);
        placement_spin_buttons.insert(sp);
        sp_y = sp;
    }
    {
        auto sp = Gtk::manage(new SpinButtonDim);
        sp->set_range(-100_mm, 100_mm);
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

std::string ImpPackage::get_hud_text(std::set<SelectableRef> &sel)
{
    std::string s;
    if (sel_count_type(sel, ObjectType::PAD) == 1) {
        const auto &pad = core_package.get_package()->pads.at(sel_find_one(sel, ObjectType::PAD));
        s += "<b>Pad " + pad.name + "</b>\n";
        for (const auto &it : pad.parameter_set) {
            s += parameter_id_to_name(it.first) + ": " + dim_to_string(it.second, true) + "\n";
        }
        sel_erase_type(sel, ObjectType::PAD);
    }
    else if (sel_count_type(sel, ObjectType::PAD) == 2) {
        const auto &pkg = core_package.get_package();
        std::vector<const Pad *> pads;
        for (const auto &it : sel) {
            if (it.type == ObjectType::PAD) {
                pads.push_back(&pkg->pads.at(it.uuid));
            }
        }
        assert(pads.size() == 2);
        s += "<b>2 Pads</b>\n";
        s += "ΔX:" + dim_to_string(std::abs(pads.at(0)->placement.shift.x - pads.at(1)->placement.shift.x)) + "\n";
        s += "ΔY:" + dim_to_string(std::abs(pads.at(0)->placement.shift.y - pads.at(1)->placement.shift.y));
        sel_erase_type(sel, ObjectType::PAD);
    }
    trim(s);
    return s;
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

void ImpPackage::update_action_sensitivity()
{
    auto sel = canvas->get_selection();
    set_action_sensitive(make_action(ActionID::EDIT_PADSTACK),
                         sockets_connected && std::any_of(sel.begin(), sel.end(), [](const auto &x) {
                             return x.type == ObjectType::PAD;
                         }));

    ImpBase::update_action_sensitivity();
}

void ImpPackage::update_monitor()
{
    std::set<std::pair<ObjectType, UUID>> items;
    const auto pkg = core_package.get_package();
    for (const auto &it : pkg->pads) {
        items.emplace(ObjectType::PADSTACK, it.second.pool_padstack->uuid);
    }
    set_monitor_items(items);
}

void ImpPackage::construct()
{
    ImpLayer::construct_layer_box();

    main_window->set_title("Package - Interactive Manipulator");
    state_store = std::make_unique<WindowStateStore>(main_window, "imp-package");

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

    connect_action(ActionID::VIEW_3D, [this](const auto &a) {
        view_3d_window->update();
        view_3d_window->present();
    });

    connect_action(ActionID::EDIT_PADSTACK, [this](const auto &a) {
        auto sel = canvas->get_selection();
        if (auto uu = sel_find_one(sel, ObjectType::PAD)) {
            this->edit_pool_item(ObjectType::PADSTACK, core_package.get_package()->pads.at(uu).pool_padstack->uuid);
        }
    });

    footprint_generator_window = FootprintGeneratorWindow::create(main_window, &core_package);
    footprint_generator_window->signal_generated().connect(sigc::mem_fun(*this, &ImpBase::canvas_update_from_pp));

    auto parameter_window =
            new ParameterWindow(main_window, &core_package.parameter_program_code, &core_package.parameter_set);
    parameter_window->signal_changed().connect([this] { core_package.set_needs_save(); });
    {
        auto button = Gtk::manage(new Gtk::Button("Parameters..."));
        main_window->header->pack_start(*button);
        button->show();
        button->signal_clicked().connect([parameter_window] { parameter_window->present(); });
    }
    parameter_window_add_polygon_expand(parameter_window);
    {
        auto button = Gtk::manage(new Gtk::Button("Insert courtyard program"));
        parameter_window->add_button(button);
        button->signal_clicked().connect([this, parameter_window] {
            const Polygon *poly = nullptr;
            for (const auto &it : core_package.get_package()->polygons) {
                if (it.second.vertices.size() == 4 && !it.second.has_arcs()
                    && it.second.parameter_class == "courtyard") {
                    poly = &it.second;
                    break;
                }
            }
            if (poly) {
                parameter_window->set_error_message("");
                Coordi a = poly->vertices.at(0).position;
                Coordi b = a;
                for (const auto &v : poly->vertices) {
                    a = Coordi::min(a, v.position);
                    b = Coordi::max(b, v.position);
                }
                auto c = (a + b) / 2;
                auto d = b - a;
                std::stringstream ss;
                ss.imbue(std::locale::classic());
                ss << std::fixed << std::setprecision(3);
                ss << d.x / 1e6 << "mm " << d.y / 1e6 << "mm\n";
                ss << "get-parameter [ courtyard_expansion ]\n2 * "
                      "+xy\nset-polygon [ courtyard rectangle ";
                ss << c.x / 1e6 << "mm " << c.y / 1e6 << "mm ]";
                parameter_window->insert_text(ss.str());
                parameter_window->get_parameter_set_editor()->add_or_set_parameter(ParameterID::COURTYARD_EXPANSION,
                                                                                   0.25_mm);
                parameter_window->signal_apply().emit();
            }
            else {
                parameter_window->set_error_message(
                        "no courtyard polygon found: needs to have 4 vertices "
                        "and the parameter class 'courtyard'");
            }
        });
    }
    parameter_window->signal_apply().connect([this, parameter_window] {
        if (core->tool_is_active())
            return;
        auto ps = core_package.get_package();
        auto r_compile = ps->parameter_program.set_code(core_package.parameter_program_code);
        if (r_compile.first) {
            parameter_window->set_error_message("<b>Compile error:</b>" + r_compile.second);
            return;
        }
        else {
            parameter_window->set_error_message("");
        }
        ps->parameter_set = core_package.parameter_set;
        auto r = ps->parameter_program.run(ps->parameter_set);
        if (r.first) {
            parameter_window->set_error_message("<b>Run error:</b>" + r.second);
            return;
        }
        else {
            parameter_window->set_error_message("");
        }
        core_package.rebuild();
        canvas_update();
    });

    layer_help_box = Gtk::manage(new LayerHelpBox(*pool.get()));
    layer_help_box->show();
    main_window->left_panel->pack_end(*layer_help_box, true, true, 0);
    layer_box->property_work_layer().signal_changed().connect(
            [this] { layer_help_box->set_layer(layer_box->property_work_layer()); });
    layer_help_box->set_layer(layer_box->property_work_layer());
    layer_help_box->signal_trigger_action().connect([this](auto a) { this->trigger_action(a); });


    auto header_button = Gtk::manage(new HeaderButton);
    header_button->set_label(core_package.get_package()->name);
    main_window->header->set_custom_title(*header_button);
    header_button->show();

    auto entry_name = header_button->add_entry("Name");
    auto entry_manufacturer = header_button->add_entry("Manufacturer");
    entry_manufacturer->set_completion(create_pool_manufacturer_completion(pool.get()));

    auto entry_tags = Gtk::manage(new TagEntry(*pool.get(), ObjectType::PACKAGE, true));
    entry_tags->show();
    header_button->add_widget("Tags", entry_tags);

    auto browser_alt_button = Gtk::manage(new PoolBrowserButton(ObjectType::PACKAGE, pool.get()));
    browser_alt_button->get_browser()->set_show_none(true);
    header_button->add_widget("Alternate for", browser_alt_button);

    {
        auto pkg = core_package.get_package();
        entry_name->set_text(pkg->name);
        entry_manufacturer->set_text(pkg->manufacturer);
        std::stringstream s;
        entry_tags->set_tags(pkg->tags);
        if (pkg->alternate_for)
            browser_alt_button->property_selected_uuid() = pkg->alternate_for->uuid;
    }

    entry_name->signal_changed().connect([this, entry_name, header_button] {
        header_button->set_label(entry_name->get_text());
        core_package.set_needs_save();
    });
    entry_manufacturer->signal_changed().connect([this] { core_package.set_needs_save(); });
    entry_tags->signal_changed().connect([this] { core_package.set_needs_save(); });

    browser_alt_button->property_selected_uuid().signal_changed().connect([this] { core_package.set_needs_save(); });

    if (core_package.get_package()->name.size() == 0) { // new package
        entry_name->set_text(Glib::path_get_basename(Glib::path_get_dirname(core_package.get_filename())));
    }

    hamburger_menu->append("Import DXF", "win.import_dxf");
    add_tool_action(ToolID::IMPORT_DXF, "import_dxf");

    hamburger_menu->append("Import KiCad package", "win.import_kicad");
    add_tool_action(ToolID::IMPORT_KICAD_PACKAGE, "import_kicad");

    hamburger_menu->append("Reload pool", "win.reload_pool");
    main_window->add_action("reload_pool", [this] { trigger_action(ActionID::RELOAD_POOL); });

    {
        auto button = Gtk::manage(new Gtk::Button("Footprint gen."));
        button->signal_clicked().connect([this] {
            footprint_generator_window->present();
            footprint_generator_window->show_all();
        });
        button->show();
        core->signal_tool_changed().connect([button](ToolID t) { button->set_sensitive(t == ToolID::NONE); });
        main_window->header->pack_end(*button);
    }

    update_monitor();

    core_package.signal_save().connect(
            [this, entry_name, entry_manufacturer, entry_tags, header_button, browser_alt_button] {
                auto pkg = core_package.get_package();
                pkg->tags = entry_tags->get_tags();
                pkg->name = entry_name->get_text();
                pkg->manufacturer = entry_manufacturer->get_text();
                UUID alt_uuid = browser_alt_button->property_selected_uuid();
                if (alt_uuid) {
                    pkg->alternate_for = pool->get_package(alt_uuid);
                }
                else {
                    pkg->alternate_for = nullptr;
                }
                header_button->set_label(pkg->name);
            });
}


void ImpPackage::update_highlights()
{
    canvas->set_flags_all(0, Triangle::FLAG_HIGHLIGHT);
    canvas->set_highlight_enabled(highlights.size());
    for (const auto &it : highlights) {
        if (it.type == ObjectType::PAD) {
            ObjectRef ref(ObjectType::PAD, it.uuid);
            canvas->set_flags(ref, Triangle::FLAG_HIGHLIGHT, 0);
        }

        else {
            canvas->set_flags(it, Triangle::FLAG_HIGHLIGHT, 0);
        }
    }
}


std::pair<ActionID, ToolID> ImpPackage::get_doubleclick_action(ObjectType type, const UUID &uu)
{
    auto a = ImpBase::get_doubleclick_action(type, uu);
    if (a.first != ActionID::NONE)
        return a;
    switch (type) {
    case ObjectType::PAD:
        return make_action(ToolID::EDIT_PAD_PARAMETER_SET);
        break;
    default:
        return {ActionID::NONE, ToolID::NONE};
    }
}

static void append_bottom_layers(std::vector<int> &layers)
{
    std::vector<int> bottoms;
    bottoms.reserve(layers.size());
    for (auto it = layers.rbegin(); it != layers.rend(); it++) {
        bottoms.push_back(-100 - *it);
    }
    layers.insert(layers.end(), bottoms.begin(), bottoms.end());
}

std::map<ObjectType, ImpBase::SelectionFilterInfo> ImpPackage::get_selection_filter_info() const
{
    std::vector<int> layers_line = {BoardLayers::TOP_ASSEMBLY, BoardLayers::TOP_PACKAGE, BoardLayers::TOP_SILKSCREEN};
    append_bottom_layers(layers_line);
    std::vector<int> layers_text = {BoardLayers::TOP_ASSEMBLY, BoardLayers::TOP_SILKSCREEN};
    append_bottom_layers(layers_text);
    std::vector<int> layers_polygon = {BoardLayers::TOP_COURTYARD, BoardLayers::TOP_ASSEMBLY, BoardLayers::TOP_PACKAGE,
                                       BoardLayers::TOP_SILKSCREEN};
    append_bottom_layers(layers_polygon);
    std::map<ObjectType, ImpBase::SelectionFilterInfo> r = {
            {ObjectType::PAD, {}},
            {ObjectType::LINE, {layers_line, true}},
            {ObjectType::TEXT, {layers_text, true}},
            {ObjectType::JUNCTION, {}},
            {ObjectType::POLYGON, {layers_polygon, true}},
            {ObjectType::DIMENSION, {}},
            {ObjectType::ARC, {layers_line, true}},
    };
    return r;
}


} // namespace horizon
