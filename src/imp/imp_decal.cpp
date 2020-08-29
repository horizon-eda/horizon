#include "imp_decal.hpp"
#include "canvas/canvas_gl.hpp"
#include "header_button.hpp"
#include "board/board_layers.hpp"
#include "core/tool_id.hpp"
#include "widgets/action_button.hpp"
#include "util/util.hpp"
#include "widgets/layer_box.hpp"

namespace horizon {
ImpDecal::ImpDecal(const std::string &decal_filename, const std::string &pool_path)
    : ImpLayer(pool_path), core_decal(decal_filename, *pool), decal(core_decal.get_decal())
{
    core = &core_decal;
    core_decal.signal_tool_changed().connect(sigc::mem_fun(*this, &ImpBase::handle_tool_change));
}

void ImpDecal::canvas_update()
{
    canvas->update(*core_decal.get_canvas_data());
}

void ImpDecal::construct()
{
    ImpLayer::construct_layer_box();

    main_window->set_title("Decal - Interactive Manipulator");
    state_store = std::make_unique<WindowStateStore>(main_window, "imp-decal");

    hamburger_menu->append("Import DXF", "win.import_dxf");
    add_tool_action(ToolID::IMPORT_DXF, "import_dxf");

    hamburger_menu->append("Import KiCad package", "win.import_kicad");
    add_tool_action(ToolID::IMPORT_KICAD_PACKAGE, "import_kicad");

    header_button = Gtk::manage(new HeaderButton);
    main_window->header->set_custom_title(*header_button);
    header_button->show();
    header_button->signal_closed().connect(sigc::mem_fun(*this, &ImpDecal::update_header));

    name_entry = header_button->add_entry("Name");
    name_entry->set_text(decal.name);
    name_entry->set_width_chars(std::max(decal.name.size(), (size_t)20));
    name_entry->signal_changed().connect([this] { core_decal.set_needs_save(); });
    name_entry->signal_activate().connect(sigc::mem_fun(*this, &ImpDecal::update_header));

    core_decal.signal_save().connect([this] {
        decal.name = name_entry->get_text();
        header_button->set_label(decal.name);
    });

    add_action_button_line();
    add_action_button_polygon();
    add_action_button(make_action(ToolID::PLACE_TEXT));

    update_header();
}

std::map<ObjectType, ImpBase::SelectionFilterInfo> ImpDecal::get_selection_filter_info() const
{
    const std::vector<int> my_layers = {
            BoardLayers::TOP_ASSEMBLY,
            BoardLayers::TOP_SILKSCREEN,
            BoardLayers::TOP_MASK,
            BoardLayers::TOP_COPPER,

    };
    std::map<ObjectType, ImpBase::SelectionFilterInfo> r = {
            {ObjectType::JUNCTION, {}},
            {ObjectType::POLYGON, {my_layers, false}},
            {ObjectType::LINE, {my_layers, false}},
            {ObjectType::ARC, {my_layers, false}},
            {ObjectType::TEXT, {my_layers, false}},
    };
    return r;
}

void ImpDecal::update_header()
{
    header_button->set_label(name_entry->get_text());
    set_window_title(name_entry->get_text());
}

void ImpDecal::load_default_layers()
{
    layer_box->load_from_json(
            json_from_resource("/org/horizon-eda/horizon/imp/"
                               "layer_display_decal.json"));
}

}; // namespace horizon
