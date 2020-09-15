#include "dialogs.hpp"
#include "map_pin.hpp"
#include "map_symbol.hpp"
#include "map_package.hpp"
#include "pool_browser_dialog.hpp"
#include "widgets/pool_browser_part.hpp"
#include "widgets/pool_browser_symbol.hpp"
#include "widgets/pool_browser_padstack.hpp"
#include "symbol_pin_names_window.hpp"
#include "select_net.hpp"
#include "ask_net_merge.hpp"
#include "manage_buses.hpp"
#include "manage_net_classes.hpp"
#include "manage_power_nets.hpp"
#include "ask_datum.hpp"
#include "ask_datum_string.hpp"
#include "ask_datum_angle.hpp"
#include "select_via_padstack.hpp"
#include "annotate.hpp"
#include "pool/part.hpp"
#include "edit_shape.hpp"
#include "edit_pad_parameter_set.hpp"
#include "schematic_properties.hpp"
#include "edit_via.hpp"
#include "edit_plane.hpp"
#include "edit_stackup.hpp"
#include "edit_board_hole.hpp"
#include "edit_frame.hpp"
#include "edit_keepout.hpp"
#include "select_group_tag.hpp"
#include "widgets/spin_button_dim.hpp"
#include "widgets/spin_button_angle.hpp"
#include "renumber_pads_window.hpp"
#include "generate_silkscreen_window.hpp"
#include "select_included_board.hpp"
#include "manage_included_boards.hpp"
#include "enter_datum_window.hpp"
#include "router_settings_window.hpp"
#include <glibmm.h>
#include "pool/ipool.hpp"
#include "widgets/net_selector.hpp"

namespace horizon {
void Dialogs::set_parent(Gtk::Window *w)
{
    parent = w;
}

void Dialogs::set_interface(ImpInterface *intf)
{
    interface = intf;
}

std::optional<UUID> Dialogs::map_pin(const std::vector<std::pair<const Pin *, bool>> &pins)
{
    MapPinDialog dia(parent, pins);
    auto r = dia.run();
    if (r == Gtk::RESPONSE_OK && dia.selection_valid) {
        return dia.selected_uuid;
    }
    else {
        return {};
    }
}
std::optional<UUIDPath<2>> Dialogs::map_symbol(const std::map<UUIDPath<2>, std::string> &gates)
{
    MapSymbolDialog dia(parent, gates);
    auto r = dia.run();
    if (r == Gtk::RESPONSE_OK && dia.selection_valid) {
        return dia.selected_uuid_path;
    }
    else {
        return {};
    }
}

std::optional<UUID> Dialogs::select_symbol(IPool &pool, const UUID &unit_uuid)
{
    PoolBrowserDialog dia(parent, ObjectType::SYMBOL, pool);
    auto &br = dynamic_cast<PoolBrowserSymbol &>(dia.get_browser());
    br.set_unit_uuid(unit_uuid);
    auto r = dia.run();
    if (r == Gtk::RESPONSE_OK) {
        return br.get_selected();
    }
    else {
        return {};
    }
}

std::optional<UUID> Dialogs::select_padstack(IPool &pool, const UUID &package_uuid)
{
    PoolBrowserDialog dia(parent, ObjectType::PADSTACK, pool);
    auto &br = dynamic_cast<PoolBrowserPadstack &>(dia.get_browser());
    br.set_package_uuid(package_uuid);
    br.set_include_padstack_type(Padstack::Type::MECHANICAL, true);
    if (dia.run() == Gtk::RESPONSE_OK) {
        return br.get_selected();
    }
    else {
        return {};
    }
}

std::optional<UUID> Dialogs::select_hole_padstack(IPool &pool)
{
    PoolBrowserDialog dia(parent, ObjectType::PADSTACK, pool);
    auto &br = dynamic_cast<PoolBrowserPadstack &>(dia.get_browser());
    br.set_include_padstack_type(Padstack::Type::MECHANICAL, true);
    br.set_include_padstack_type(Padstack::Type::HOLE, true);
    br.set_include_padstack_type(Padstack::Type::TOP, false);
    br.set_include_padstack_type(Padstack::Type::THROUGH, false);
    br.set_include_padstack_type(Padstack::Type::BOTTOM, false);
    if (dia.run() == Gtk::RESPONSE_OK) {
        return br.get_selected();
    }
    else {
        return {};
    }
}

std::optional<UUID> Dialogs::select_entity(IPool &pool)
{
    PoolBrowserDialog dia(parent, ObjectType::ENTITY, pool);
    if (dia.run() == Gtk::RESPONSE_OK) {
        return dia.get_browser().get_selected();
    }
    else {
        return {};
    }
}

std::optional<UUID> Dialogs::select_unit(IPool &pool)
{
    PoolBrowserDialog dia(parent, ObjectType::UNIT, pool);
    if (dia.run() == Gtk::RESPONSE_OK) {
        return dia.get_browser().get_selected();
    }
    else {
        return {};
    }
}

std::optional<UUID> Dialogs::select_net(const Block &block, bool power_only, const UUID &net_default)
{
    SelectNetDialog dia(parent, block, "Select net");
    dia.net_selector->set_power_only(power_only);
    dia.net_selector->select_net(net_default);
    auto r = dia.run();
    if (r == Gtk::RESPONSE_OK && dia.valid) {
        return dia.net;
    }
    else {
        return {};
    }
}
std::optional<UUID> Dialogs::select_bus(const Block &block)
{
    SelectNetDialog dia(parent, block, "Select bus");
    dia.net_selector->set_bus_mode(true);
    auto r = dia.run();
    if (r == Gtk::RESPONSE_OK && dia.valid) {
        return dia.net;
    }
    else {
        return {};
    }
}
std::optional<UUID> Dialogs::select_bus_member(const Block &block, const UUID &bus_uuid)
{
    SelectNetDialog dia(parent, block, "Select bus member");
    dia.net_selector->set_bus_member_mode(bus_uuid);
    auto r = dia.run();
    if (r == Gtk::RESPONSE_OK && dia.valid) {
        return dia.net;
    }
    else {
        return {};
    }
}

std::optional<UUID> Dialogs::select_group_tag(const Block &block, bool tag_mode, const UUID &current)
{
    SelectGroupTagDialog dia(parent, block, tag_mode);
    auto r = dia.run();
    if (r == Gtk::RESPONSE_OK && dia.selection_valid) {
        return dia.selected_uuid;
    }
    else {
        return {};
    }
}

std::optional<UUID> Dialogs::select_included_board(const Board &brd)
{
    SelectIncludedBoardDialog dia(parent, brd);
    auto r = dia.run();
    if (r == Gtk::RESPONSE_OK && dia.valid) {
        return dia.selected_uuid;
    }
    else {
        return {};
    }
}

bool Dialogs::edit_shapes(std::set<Shape *> shapes)
{
    ShapeDialog dia(parent, shapes);
    return dia.run() == Gtk::RESPONSE_OK;
}

bool Dialogs::edit_via(std::set<class Via *> &vias, class ViaPadstackProvider &vpp)
{
    EditViaDialog dia(parent, vias, vpp);
    return dia.run() == Gtk::RESPONSE_OK;
}

unsigned int Dialogs::ask_net_merge(Net &net, Net &into)
{
    AskNetMergeDialog dia(parent, net, into);
    return dia.run();
}

bool Dialogs::manage_buses(Block &b)
{
    ManageBusesDialog dia(parent, b);
    return dia.run() == Gtk::RESPONSE_OK;
}

bool Dialogs::manage_net_classes(Block &b)
{
    ManageNetClassesDialog dia(parent, b);
    return dia.run() == Gtk::RESPONSE_OK;
}

bool Dialogs::manage_power_nets(Block &b)
{
    ManagePowerNetsDialog dia(parent, b);
    return dia.run() == Gtk::RESPONSE_OK;
}

bool Dialogs::manage_included_boards(Board &b)
{
    ManageIncludedBoardsDialog dia(parent, b);
    return dia.run() == Gtk::RESPONSE_OK;
}

bool Dialogs::annotate(Schematic &s)
{
    AnnotateDialog dia(parent, s);
    return dia.run() == Gtk::RESPONSE_OK;
}

bool Dialogs::edit_pad_parameter_set(std::set<class Pad *> &pads, IPool &pool, class Package &pkg)
{
    PadParameterSetDialog dia(parent, pads, pool, pkg);
    return dia.run() == Gtk::RESPONSE_OK;
}

bool Dialogs::edit_board_hole(std::set<class BoardHole *> &holes, IPool &pool, Block &block)
{
    BoardHoleDialog dia(parent, holes, pool, block);
    return dia.run() == Gtk::RESPONSE_OK;
}

bool Dialogs::edit_schematic_properties(class Schematic &sch, class IPool &pool)
{
    SchematicPropertiesDialog dia(parent, sch, pool);
    return dia.run() == Gtk::RESPONSE_OK;
}

bool Dialogs::edit_frame_properties(class Frame &fr)
{
    EditFrameDialog dia(parent, fr);
    return dia.run() == Gtk::RESPONSE_OK;
}

std::optional<int64_t> Dialogs::ask_datum(const std::string &label, int64_t def)
{
    AskDatumDialog dia(parent, label);
    dia.sp->set_value(def);
    dia.sp->select_region(0, -1);
    auto r = dia.run();
    if (r == Gtk::RESPONSE_OK) {
        return dia.sp->get_value_as_int();
    }
    else {
        return {};
    }
}

std::optional<std::string> Dialogs::ask_datum_string(const std::string &label, const std::string &def)
{
    AskDatumStringDialog dia(parent, label, false);
    dia.set_text(def);
    auto r = dia.run();
    if (r == Gtk::RESPONSE_OK) {
        return dia.get_text();
    }
    else {
        return {};
    }
}

std::optional<std::string> Dialogs::ask_datum_string_multiline(const std::string &label, const std::string &def)
{
    AskDatumStringDialog dia(parent, label, true);
    dia.set_text(def);
    auto r = dia.run();
    if (r == Gtk::RESPONSE_OK) {
        return dia.get_text();
    }
    else {
        return {};
    }
}

std::optional<Coordi> Dialogs::ask_datum_coord(const std::string &label, Coordi def)
{
    AskDatumDialog dia(parent, label, true);
    dia.sp_x->set_value(def.x);
    dia.sp_y->set_value(def.y);
    dia.sp_x->select_region(0, -1);
    dia.cb_x->set_sensitive(false);
    dia.cb_y->set_sensitive(false);
    auto r = dia.run();
    if (r == Gtk::RESPONSE_OK) {
        return {{dia.sp_x->get_value_as_int(), dia.sp_y->get_value_as_int()}};
    }
    else {
        return {};
    }
}

std::optional<std::pair<Coordi, std::pair<bool, bool>>> Dialogs::ask_datum_coord2(const std::string &label, Coordi def)
{
    AskDatumDialog dia(parent, label, true);
    dia.sp_x->set_value(def.x);
    dia.sp_y->set_value(def.y);
    dia.sp_x->select_region(0, -1);
    auto r = dia.run();
    if (r == Gtk::RESPONSE_OK) {
        return {{{dia.sp_x->get_value_as_int(), dia.sp_y->get_value_as_int()},
                 {dia.cb_x->get_active(), dia.cb_y->get_active()}}};
    }
    else {
        return {};
    }
}

std::optional<int> Dialogs::ask_datum_angle(const std::string &label, int def)
{
    AskDatumAngleDialog dia(parent, label);
    dia.sp->set_value(def);
    dia.sp->select_region(0, -1);
    auto r = dia.run();
    if (r == Gtk::RESPONSE_OK) {
        return dia.sp->get_value();
    }
    else {
        return {};
    }
}

std::optional<UUID> Dialogs::select_part(IPool &pool, const UUID &entity_uuid, const UUID &part_uuid, bool show_none)
{
    PoolBrowserDialog dia(parent, ObjectType::PART, pool);
    auto &br = dynamic_cast<PoolBrowserPart &>(dia.get_browser());
    br.set_show_none(show_none);
    br.set_entity_uuid(entity_uuid);
    if (part_uuid) {
        auto part = pool.get_part(part_uuid);
        br.set_MPN(part->get_MPN());
    }
    auto r = dia.run();
    if (r == Gtk::RESPONSE_OK) {
        return br.get_selected();
    }
    else {
        return {};
    }
}

std::optional<UUID> Dialogs::map_package(const std::vector<std::pair<Component *, bool>> &components)
{
    MapPackageDialog dia(parent, components);
    auto r = dia.run();
    if (r == Gtk::RESPONSE_OK && dia.selection_valid) {
        return dia.selected_uuid;
    }
    else {
        return {};
    }
}

std::optional<UUID> Dialogs::select_via_padstack(class ViaPadstackProvider &vpp)
{
    SelectViaPadstackDialog dia(parent, vpp);
    auto r = dia.run();
    if (r == Gtk::RESPONSE_OK && dia.selection_valid) {
        return dia.selected_uuid;
    }
    else {
        return {};
    }
}

std::optional<UUID> Dialogs::select_decal(IPool &pool)
{
    PoolBrowserDialog dia(parent, ObjectType::DECAL, pool);
    auto &br = dia.get_browser();
    auto r = dia.run();
    if (r == Gtk::RESPONSE_OK) {
        return br.get_selected();
    }
    else {
        return {};
    }
}

bool Dialogs::edit_plane(class Plane &plane, class Board &brd)
{
    EditPlaneDialog dia(parent, plane, brd);
    return dia.run() == Gtk::RESPONSE_OK;
}

bool Dialogs::edit_keepout(class Keepout &keepout, class IDocument &c, bool add_mode)
{
    EditKeepoutDialog dia(parent, keepout, c, add_mode);
    return dia.run() == Gtk::RESPONSE_OK;
}

bool Dialogs::edit_stackup(class IDocumentBoard &doc)
{
    EditStackupDialog dia(parent, doc);
    return dia.run() == Gtk::RESPONSE_OK;
}

std::optional<std::string> Dialogs::ask_dxf_filename()
{
    GtkFileChooserNative *native = gtk_file_chooser_native_new("Import DXF", GTK_WINDOW(parent->gobj()),
                                                               GTK_FILE_CHOOSER_ACTION_OPEN, "_Open", "_Cancel");
    auto chooser = Glib::wrap(GTK_FILE_CHOOSER(native));
    auto filter = Gtk::FileFilter::create();
    filter->set_name("DXF drawing");
    filter->add_pattern("*.dxf");
    filter->add_pattern("*.DXF");
    chooser->add_filter(filter);

    if (gtk_native_dialog_run(GTK_NATIVE_DIALOG(native)) == GTK_RESPONSE_ACCEPT) {
        return chooser->get_filename();
    }
    else {
        return {};
    }
}

std::optional<std::string> Dialogs::ask_kicad_package_filename()
{
    GtkFileChooserNative *native = gtk_file_chooser_native_new("Import KiCad package", GTK_WINDOW(parent->gobj()),
                                                               GTK_FILE_CHOOSER_ACTION_OPEN, "_Open", "_Cancel");
    auto chooser = Glib::wrap(GTK_FILE_CHOOSER(native));
    auto filter = Gtk::FileFilter::create();
    filter->set_name("KiCad package");
    filter->add_pattern("*.kicad_mod");
    chooser->add_filter(filter);

    if (gtk_native_dialog_run(GTK_NATIVE_DIALOG(native)) == GTK_RESPONSE_ACCEPT) {
        return chooser->get_filename();
    }
    else {
        return {};
    }
}

std::optional<std::string> Dialogs::ask_picture_filename()
{
    GtkFileChooserNative *native = gtk_file_chooser_native_new("Import Picture", GTK_WINDOW(parent->gobj()),
                                                               GTK_FILE_CHOOSER_ACTION_OPEN, "_Open", "_Cancel");
    auto chooser = Glib::wrap(GTK_FILE_CHOOSER(native));
    auto filter = Gtk::FileFilter::create();
    filter->set_name("Supported image formats");
    filter->add_pixbuf_formats();
    chooser->add_filter(filter);

    if (gtk_native_dialog_run(GTK_NATIVE_DIALOG(native)) == GTK_RESPONSE_ACCEPT) {
        return chooser->get_filename();
    }
    else {
        return {};
    }
}

class SymbolPinNamesWindow *Dialogs::show_symbol_pin_names_window(class SchematicSymbol &symbol)
{
    if (window_nonmodal)
        return nullptr;
    auto win = new SymbolPinNamesWindow(parent, interface, symbol);
    window_nonmodal = win;
    win->present();
    return win;
}

class RenumberPadsWindow *Dialogs::show_renumber_pads_window(class Package &pkg, const std::set<UUID> &pads)
{
    if (window_nonmodal)
        return nullptr;
    auto win = new RenumberPadsWindow(parent, interface, pkg, pads);
    window_nonmodal = win;
    win->present();
    return win;
}

class GenerateSilkscreenWindow *Dialogs::show_generate_silkscreen_window(class ToolSettings &settings)
{
    if (window_nonmodal)
        return nullptr;
    auto win = new GenerateSilkscreenWindow(parent, interface, settings);
    window_nonmodal = win;
    win->present();
    return win;
}

EnterDatumWindow *Dialogs::show_enter_datum_window(const std::string &label, int64_t def)
{
    if (window_nonmodal) {
        if (auto win = dynamic_cast<EnterDatumWindow *>(window_nonmodal)) {
            win->present();
            return win;
        }
    }
    auto win = new EnterDatumWindow(parent, interface, label, def);
    window_nonmodal = win;
    win->signal_hide().connect([this] { close_nonmodal(); });
    win->present();
    return win;
}

RouterSettingsWindow *Dialogs::show_router_settings_window(ToolSettings &settings)
{
    if (window_nonmodal) {
        if (auto win = dynamic_cast<RouterSettingsWindow *>(window_nonmodal)) {
            win->present();
            return win;
        }
    }
    auto win = new RouterSettingsWindow(parent, interface, settings);
    window_nonmodal = win;
    win->signal_hide().connect([this] { close_nonmodal(); });
    win->present();
    return win;
}

void Dialogs::close_nonmodal()
{
    delete window_nonmodal;
    window_nonmodal = nullptr;
}

ToolWindow *Dialogs::get_nonmodal()
{
    return window_nonmodal;
}

} // namespace horizon
