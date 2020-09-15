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
#include "util/selection_util.hpp"
#include "widgets/action_button.hpp"
#include <iomanip>
#include "core/tool_id.hpp"
#include "pool/pool_cached.hpp"

namespace horizon {
ImpPackage::ImpPackage(const std::string &package_filename, const std::string &pool_path)
    : ImpLayer(pool_path), core_package(package_filename, *pool), package(core_package.get_package()),
      searcher(core_package), fake_block(UUID::random()), fake_board(UUID::random(), fake_block)
{
    core = &core_package;
    core_package.signal_tool_changed().connect(sigc::mem_fun(*this, &ImpBase::handle_tool_change));
    load_meta();
}

void ImpPackage::canvas_update()
{
    canvas->update(*core_package.get_canvas_data());
    warnings_box->update(package.warnings);
    update_highlights();
}

void ImpPackage::apply_preferences()
{
    if (view_3d_window)
        view_3d_window->set_smooth_zoom(preferences.zoom.smooth_zoom_3d);
    ImpLayer::apply_preferences();
}

std::string ImpPackage::get_hud_text(std::set<SelectableRef> &sel)
{
    std::string s;
    if (sel_count_type(sel, ObjectType::PAD) == 1) {
        const auto &pad = package.pads.at(sel_find_one(sel, ObjectType::PAD).uuid);
        s += "<b>Pad " + pad.name + "</b>\n";
        for (const auto &it : pad.parameter_set) {
            s += parameter_id_to_name(it.first) + ": " + dim_to_string(it.second, true) + "\n";
        }
        sel_erase_type(sel, ObjectType::PAD);
    }
    else if (sel_count_type(sel, ObjectType::PAD) == 2) {
        std::vector<const Pad *> pads;
        for (const auto &it : sel) {
            if (it.type == ObjectType::PAD) {
                pads.push_back(&package.pads.at(it.uuid));
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
    ItemSet items;
    for (const auto &it : package.pads) {
        items.emplace(ObjectType::PADSTACK, it.second.pool_padstack->uuid);
    }
    set_monitor_items(items);
}

void ImpPackage::check_alt_pkg()
{
    const UUID alt_uuid = browser_alt_button->property_selected_uuid();
    if (!alt_uuid)
        return;
    const auto pkg_uuid = package.uuid;
    if (pkg_uuid == alt_uuid) {
        browser_alt_button->property_selected_uuid() = UUID();
        return;
    }
    pool->db.execute("DROP VIEW IF EXISTS pkg_deps");
    {
        pool->db.execute(
                        "CREATE TEMPORARY view pkg_deps AS SELECT uuid, dep_uuid FROM dependencies WHERE "
                        "type = 'package' AND dep_type = 'package' AND uuid != '" + (std::string)pkg_uuid + "' "
                        "UNION SELECT '" + (std::string)pkg_uuid + "', '" + (std::string) alt_uuid+ "'");
    }
    {
        SQLite::Query q(pool->db,
                        "WITH FindRoot AS ( "
                        "SELECT uuid, dep_uuid, uuid as path, 0 AS Distance "
                        "FROM pkg_deps WHERE uuid = $pkg "
                        "UNION ALL "
                        "SELECT C.uuid, P.dep_uuid, C.path || ' > ' || P.uuid, C.Distance + 1 "
                        "FROM pkg_deps AS P "
                        " JOIN FindRoot AS C "
                        " ON C.dep_uuid = P.uuid AND P.dep_uuid != P.uuid AND C.dep_uuid != C.uuid "
                        ") "
                        "SELECT * "
                        "FROM FindRoot AS R "
                        "WHERE R.uuid = R.dep_uuid AND R.Distance > 0");
        q.bind("$pkg", pkg_uuid);
        while (q.step()) {
            Gtk::MessageDialog md(*main_window, "Can't set as alt. package", false /* use_markup */, Gtk::MESSAGE_ERROR,
                                  Gtk::BUTTONS_OK);
            md.set_secondary_text("Cyclic dependency");
            md.run();
            browser_alt_button->property_selected_uuid() = UUID();
            break;
        }
    }

    pool->db.execute("DROP VIEW IF EXISTS pkg_deps");
}

void ImpPackage::construct()
{
    ImpLayer::construct_layer_box();

    state_store = std::make_unique<WindowStateStore>(main_window, "imp-package");

    construct_3d();

    connect_action(ActionID::EDIT_PADSTACK, [this](const auto &a) {
        auto sel = canvas->get_selection();
        if (sel_count_type(sel, ObjectType::PAD)) {
            auto uu = sel_find_one(sel, ObjectType::PAD).uuid;
            this->edit_pool_item(ObjectType::PADSTACK, package.pads.at(uu).pool_padstack->uuid);
        }
    });

    footprint_generator_window = FootprintGeneratorWindow::create(main_window, core_package);
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
            for (const auto &it : package.polygons) {
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
        auto r_compile = package.parameter_program.set_code(core_package.parameter_program_code);
        if (r_compile.first) {
            parameter_window->set_error_message("<b>Compile error:</b>" + r_compile.second);
            return;
        }
        else {
            parameter_window->set_error_message("");
        }
        package.parameter_set = core_package.parameter_set;
        auto r = package.parameter_program.run(package.parameter_set);
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


    header_button = Gtk::manage(new HeaderButton);
    main_window->header->set_custom_title(*header_button);
    header_button->show();
    header_button->signal_closed().connect(sigc::mem_fun(*this, &ImpPackage::update_header));

    entry_name = header_button->add_entry("Name");
    entry_name->signal_activate().connect(sigc::mem_fun(*this, &ImpPackage::update_header));

    auto entry_manufacturer = header_button->add_entry("Manufacturer");
    entry_manufacturer->set_completion(create_pool_manufacturer_completion(*pool.get()));

    auto entry_tags = Gtk::manage(new TagEntry(*pool.get(), ObjectType::PACKAGE, true));
    entry_tags->show();
    header_button->add_widget("Tags", entry_tags);

    browser_alt_button = Gtk::manage(new PoolBrowserButton(ObjectType::PACKAGE, *pool.get()));
    browser_alt_button->get_browser().set_show_none(true);
    header_button->add_widget("Alternate for", browser_alt_button);

    {
        entry_name->set_text(package.name);
        entry_manufacturer->set_text(package.manufacturer);
        std::stringstream s;
        entry_tags->set_tags(package.tags);
        if (package.alternate_for)
            browser_alt_button->property_selected_uuid() = package.alternate_for->uuid;
    }

    entry_name->signal_changed().connect([this] { core_package.set_needs_save(); });
    entry_manufacturer->signal_changed().connect([this] { core_package.set_needs_save(); });
    entry_tags->signal_changed().connect([this] { core_package.set_needs_save(); });

    browser_alt_button->property_selected_uuid().signal_changed().connect([this] {
        check_alt_pkg();
        core_package.set_needs_save();
    });

    if (package.name.size() == 0) { // new package
        entry_name->set_text(Glib::path_get_basename(Glib::path_get_dirname(core_package.get_filename())));
    }

    hamburger_menu->append("Import DXF", "win.import_dxf");
    add_tool_action(ToolID::IMPORT_DXF, "import_dxf");

    hamburger_menu->append("Import KiCad package", "win.import_kicad");
    add_tool_action(ToolID::IMPORT_KICAD_PACKAGE, "import_kicad");

    hamburger_menu->append("Reload pool", "win.reload_pool");
    main_window->add_action("reload_pool", [this] { trigger_action(ActionID::RELOAD_POOL); });

    view_options_menu->append("Bottom view", "win.bottom_view");

    connect_action(ActionID::FOOTPRINT_GENERATOR, [this](auto &a) {
        footprint_generator_window->present();
        footprint_generator_window->show_all();
    });

    update_monitor();

    core_package.signal_save().connect([this, entry_manufacturer, entry_tags] {
        package.tags = entry_tags->get_tags();
        package.name = entry_name->get_text();
        package.manufacturer = entry_manufacturer->get_text();
        UUID alt_uuid = browser_alt_button->property_selected_uuid();
        if (alt_uuid) {
            package.alternate_for = pool->get_package(alt_uuid);
        }
        else {
            package.alternate_for = nullptr;
        }
    });

    add_action_button(make_action(ToolID::PLACE_PAD));
    add_action_button_line();
    add_action_button_polygon();
    add_action_button(make_action(ToolID::PLACE_TEXT));
    add_action_button(make_action(ToolID::DRAW_DIMENSION));

    {
        auto &b = add_action_button_menu("action-generate-symbolic");
        b.set_margin_top(5);
        b.add_action(make_action(ActionID::FOOTPRINT_GENERATOR));
        b.add_action(make_action(ToolID::GENERATE_COURTYARD));
        b.add_action(make_action(ToolID::GENERATE_SILKSCREEN));
        b.set_tooltip_text("Generate…");
    }

    update_header();
}


void ImpPackage::update_highlights()
{
    canvas->set_flags_all(0, TriangleInfo::FLAG_HIGHLIGHT);
    canvas->set_highlight_enabled(highlights.size());
    for (const auto &it : highlights) {
        if (it.type == ObjectType::PAD) {
            ObjectRef ref(ObjectType::PAD, it.uuid);
            canvas->set_flags(ref, TriangleInfo::FLAG_HIGHLIGHT, 0);
        }

        else {
            canvas->set_flags(it, TriangleInfo::FLAG_HIGHLIGHT, 0);
        }
    }
}


ActionToolID ImpPackage::get_doubleclick_action(ObjectType type, const UUID &uu)
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

    using Flag = ImpBase::SelectionFilterInfo::Flag;
    std::map<ObjectType, ImpBase::SelectionFilterInfo> r = {
            {ObjectType::PAD, {{}, Flag::WORK_LAYER_ONLY_ENABLED}},
            {ObjectType::LINE, {layers_line, Flag::HAS_OTHERS}},
            {ObjectType::TEXT, {layers_text, Flag::HAS_OTHERS}},
            {ObjectType::JUNCTION, {}},
            {ObjectType::POLYGON, {layers_polygon, Flag::HAS_OTHERS}},
            {ObjectType::DIMENSION, {}},
            {ObjectType::PICTURE, {}},
            {ObjectType::ARC, {layers_line, Flag::HAS_OTHERS}},
    };
    return r;
}


void ImpPackage::update_header()
{
    header_button->set_label(entry_name->get_text());
    set_window_title(entry_name->get_text());
}

ImpPackage::~ImpPackage()
{
    delete view_3d_window;
}

} // namespace horizon
