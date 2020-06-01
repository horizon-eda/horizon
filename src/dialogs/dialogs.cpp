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
#include <glibmm.h>

namespace horizon {
void Dialogs::set_parent(Gtk::Window *w)
{
    parent = w;
}

void Dialogs::set_interface(ImpInterface *intf)
{
    interface = intf;
}

std::pair<bool, UUID> Dialogs::map_pin(const std::vector<std::pair<const Pin *, bool>> &pins)
{
    MapPinDialog dia(parent, pins);
    auto r = dia.run();
    if (r == Gtk::RESPONSE_OK) {
        return {dia.selection_valid, dia.selected_uuid};
    }
    else {
        return {false, UUID()};
    }
}
std::pair<bool, UUIDPath<2>> Dialogs::map_symbol(const std::map<UUIDPath<2>, std::string> &gates)
{
    MapSymbolDialog dia(parent, gates);
    auto r = dia.run();
    if (r == Gtk::RESPONSE_OK) {
        return {dia.selection_valid, dia.selected_uuid_path};
    }
    else {
        return {false, UUIDPath<2>()};
    }
}

std::pair<bool, UUID> Dialogs::select_symbol(Pool *pool, const UUID &unit_uuid)
{
    PoolBrowserDialog dia(parent, ObjectType::SYMBOL, pool);
    auto br = dynamic_cast<PoolBrowserSymbol *>(dia.get_browser());
    br->set_unit_uuid(unit_uuid);
    auto r = dia.run();
    if (r == Gtk::RESPONSE_OK) {
        auto uu = br->get_selected();
        return {uu, uu};
    }
    else {
        return {false, UUID()};
    }
}

std::pair<bool, UUID> Dialogs::select_padstack(Pool *pool, const UUID &package_uuid)
{
    PoolBrowserDialog dia(parent, ObjectType::PADSTACK, pool);
    auto br = dynamic_cast<PoolBrowserPadstack *>(dia.get_browser());
    br->set_package_uuid(package_uuid);
    br->set_include_padstack_type(Padstack::Type::MECHANICAL, true);
    auto r = dia.run();
    if (r == Gtk::RESPONSE_OK) {
        auto uu = br->get_selected();
        return {uu, uu};
    }
    else {
        return {false, UUID()};
    }
}

std::pair<bool, UUID> Dialogs::select_hole_padstack(Pool *pool)
{
    PoolBrowserDialog dia(parent, ObjectType::PADSTACK, pool);
    auto br = dynamic_cast<PoolBrowserPadstack *>(dia.get_browser());
    br->set_include_padstack_type(Padstack::Type::MECHANICAL, true);
    br->set_include_padstack_type(Padstack::Type::HOLE, true);
    br->set_include_padstack_type(Padstack::Type::TOP, false);
    br->set_include_padstack_type(Padstack::Type::THROUGH, false);
    br->set_include_padstack_type(Padstack::Type::BOTTOM, false);
    auto r = dia.run();
    if (r == Gtk::RESPONSE_OK) {
        auto uu = br->get_selected();
        return {uu, uu};
    }
    else {
        return {false, UUID()};
    }
}

std::pair<bool, UUID> Dialogs::select_entity(Pool *pool)
{
    PoolBrowserDialog dia(parent, ObjectType::ENTITY, pool);
    auto r = dia.run();
    if (r == Gtk::RESPONSE_OK) {
        auto uu = dia.get_browser()->get_selected();
        return {uu, uu};
    }
    else {
        return {false, UUID()};
    }
}

std::pair<bool, UUID> Dialogs::select_unit(Pool *pool)
{
    PoolBrowserDialog dia(parent, ObjectType::UNIT, pool);
    auto r = dia.run();
    if (r == Gtk::RESPONSE_OK) {
        auto uu = dia.get_browser()->get_selected();
        return {uu, uu};
    }
    else {
        return {false, UUID()};
    }
}

std::pair<bool, UUID> Dialogs::select_net(class Block *block, bool power_only, const UUID &net_default)
{
    SelectNetDialog dia(parent, block, "Select net");
    dia.net_selector->set_power_only(power_only);
    dia.net_selector->select_net(net_default);
    auto r = dia.run();
    if (r == Gtk::RESPONSE_OK) {
        return {dia.valid, dia.net};
    }
    else {
        return {false, UUID()};
    }
}
std::pair<bool, UUID> Dialogs::select_bus(class Block *block)
{
    SelectNetDialog dia(parent, block, "Select bus");
    dia.net_selector->set_bus_mode(true);
    auto r = dia.run();
    if (r == Gtk::RESPONSE_OK) {
        return {dia.valid, dia.net};
    }
    else {
        return {false, UUID()};
    }
}
std::pair<bool, UUID> Dialogs::select_bus_member(class Block *block, const UUID &bus_uuid)
{
    SelectNetDialog dia(parent, block, "Select bus member");
    dia.net_selector->set_bus_member_mode(bus_uuid);
    auto r = dia.run();
    if (r == Gtk::RESPONSE_OK) {
        return {dia.valid, dia.net};
    }
    else {
        return {false, UUID()};
    }
}

std::pair<bool, UUID> Dialogs::select_group_tag(const class Block *block, bool tag_mode, const UUID &current)
{
    SelectGroupTagDialog dia(parent, block, tag_mode);
    auto r = dia.run();
    if (r == Gtk::RESPONSE_OK) {
        return {dia.selection_valid, dia.selected_uuid};
    }
    else {
        return {false, UUID()};
    }
}

std::pair<bool, UUID> Dialogs::select_included_board(const Board &brd)
{
    SelectIncludedBoardDialog dia(parent, brd);
    auto r = dia.run();
    if (r == Gtk::RESPONSE_OK) {
        return {dia.valid, dia.selected_uuid};
    }
    else {
        return {false, UUID()};
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

unsigned int Dialogs::ask_net_merge(Net *net, Net *into)
{
    AskNetMergeDialog dia(parent, net, into);
    return dia.run();
}

bool Dialogs::manage_buses(Block *b)
{
    ManageBusesDialog dia(parent, b);
    return dia.run() == Gtk::RESPONSE_OK;
}

bool Dialogs::manage_net_classes(Block *b)
{
    ManageNetClassesDialog dia(parent, b);
    return dia.run() == Gtk::RESPONSE_OK;
}

bool Dialogs::manage_power_nets(Block *b)
{
    ManagePowerNetsDialog dia(parent, b);
    return dia.run() == Gtk::RESPONSE_OK;
}

bool Dialogs::manage_included_boards(Board &b)
{
    ManageIncludedBoardsDialog dia(parent, b);
    return dia.run() == Gtk::RESPONSE_OK;
}

bool Dialogs::annotate(Schematic *s)
{
    AnnotateDialog dia(parent, s);
    return dia.run() == Gtk::RESPONSE_OK;
}

bool Dialogs::edit_pad_parameter_set(std::set<class Pad *> &pads, Pool *pool, class Package *pkg)
{
    PadParameterSetDialog dia(parent, pads, pool, pkg);
    return dia.run() == Gtk::RESPONSE_OK;
}

bool Dialogs::edit_board_hole(std::set<class BoardHole *> &holes, Pool *pool, Block *block)
{
    BoardHoleDialog dia(parent, holes, pool, block);
    return dia.run() == Gtk::RESPONSE_OK;
}

bool Dialogs::edit_schematic_properties(class Schematic *sch, class Pool *pool)
{
    SchematicPropertiesDialog dia(parent, sch, pool);
    return dia.run() == Gtk::RESPONSE_OK;
}

bool Dialogs::edit_frame_properties(class Frame *fr)
{
    EditFrameDialog dia(parent, fr);
    return dia.run() == Gtk::RESPONSE_OK;
}

std::pair<bool, int64_t> Dialogs::ask_datum(const std::string &label, int64_t def)
{
    AskDatumDialog dia(parent, label);
    dia.sp->set_value(def);
    dia.sp->select_region(0, -1);
    auto r = dia.run();
    if (r == Gtk::RESPONSE_OK) {
        return {true, dia.sp->get_value_as_int()};
    }
    else {
        return {false, 0};
    }
}

std::pair<bool, std::string> Dialogs::ask_datum_string(const std::string &label, const std::string &def)
{
    AskDatumStringDialog dia(parent, label, false);
    dia.set_text(def);
    auto r = dia.run();
    if (r == Gtk::RESPONSE_OK) {
        return {true, dia.get_text()};
    }
    else {
        return {false, ""};
    }
}

std::pair<bool, std::string> Dialogs::ask_datum_string_multiline(const std::string &label, const std::string &def)
{
    AskDatumStringDialog dia(parent, label, true);
    dia.set_text(def);
    auto r = dia.run();
    if (r == Gtk::RESPONSE_OK) {
        return {true, dia.get_text()};
    }
    else {
        return {false, ""};
    }
}

std::pair<bool, Coordi> Dialogs::ask_datum_coord(const std::string &label, Coordi def)
{
    AskDatumDialog dia(parent, label, true);
    dia.sp_x->set_value(def.x);
    dia.sp_y->set_value(def.y);
    dia.sp_x->select_region(0, -1);
    dia.cb_x->set_sensitive(false);
    dia.cb_y->set_sensitive(false);
    auto r = dia.run();
    if (r == Gtk::RESPONSE_OK) {
        return {true, {dia.sp_x->get_value_as_int(), dia.sp_y->get_value_as_int()}};
    }
    else {
        return {false, Coordi()};
    }
}

std::tuple<bool, Coordi, std::pair<bool, bool>> Dialogs::ask_datum_coord2(const std::string &label, Coordi def)
{
    AskDatumDialog dia(parent, label, true);
    dia.sp_x->set_value(def.x);
    dia.sp_y->set_value(def.y);
    dia.sp_x->select_region(0, -1);
    auto r = dia.run();
    if (r == Gtk::RESPONSE_OK) {
        return std::make_tuple<bool, Coordi, std::pair<bool, bool>>(
                true, {dia.sp_x->get_value_as_int(), dia.sp_y->get_value_as_int()},
                std::make_pair(dia.cb_x->get_active(), dia.cb_y->get_active()));
    }
    else {
        return std::make_tuple<bool, Coordi, std::pair<bool, bool>>(false, Coordi(), {false, false});
    }
}

std::pair<bool, int> Dialogs::ask_datum_angle(const std::string &label, int def)
{
    AskDatumAngleDialog dia(parent, label);
    dia.sp->set_value(def);
    dia.sp->select_region(0, -1);
    auto r = dia.run();
    if (r == Gtk::RESPONSE_OK) {
        return {true, dia.sp->get_value()};
    }
    else {
        return {false, 0};
    }
}

std::pair<bool, UUID> Dialogs::select_part(Pool *pool, const UUID &entity_uuid, const UUID &part_uuid, bool show_none)
{
    PoolBrowserDialog dia(parent, ObjectType::PART, pool);
    auto br = dynamic_cast<PoolBrowserPart *>(dia.get_browser());
    br->set_show_none(show_none);
    br->set_entity_uuid(entity_uuid);
    if (part_uuid) {
        auto part = pool->get_part(part_uuid);
        br->set_MPN(part->get_MPN());
    }
    auto r = dia.run();
    if (r == Gtk::RESPONSE_OK) {
        auto uu = br->get_selected();
        return {uu, uu};
    }
    else {
        return {false, UUID()};
    }
}

std::pair<bool, UUID> Dialogs::map_package(const std::vector<std::pair<Component *, bool>> &components)
{
    MapPackageDialog dia(parent, components);
    auto r = dia.run();
    if (r == Gtk::RESPONSE_OK) {
        return {dia.selection_valid, dia.selected_uuid};
    }
    else {
        return {false, UUID()};
    }
}

std::pair<bool, UUID> Dialogs::select_via_padstack(class ViaPadstackProvider *vpp)
{
    SelectViaPadstackDialog dia(parent, vpp);
    auto r = dia.run();
    if (r == Gtk::RESPONSE_OK) {
        return {dia.selection_valid, dia.selected_uuid};
    }
    else {
        return {false, UUID()};
    }
}

bool Dialogs::edit_plane(class Plane *plane, class Board *brd, class Block *block)
{
    EditPlaneDialog dia(parent, plane, brd, block);
    return dia.run() == Gtk::RESPONSE_OK;
}

bool Dialogs::edit_keepout(class Keepout *keepout, class IDocument *c, bool add_mode)
{
    EditKeepoutDialog dia(parent, keepout, c, add_mode);
    return dia.run() == Gtk::RESPONSE_OK;
}

bool Dialogs::edit_stackup(class IDocumentBoard *doc)
{
    EditStackupDialog dia(parent, doc);
    return dia.run() == Gtk::RESPONSE_OK;
}

std::pair<bool, std::string> Dialogs::ask_dxf_filename()
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
        return {true, chooser->get_filename()};
    }
    else {
        return {false, ""};
    }
}

std::pair<bool, std::string> Dialogs::ask_kicad_package_filename()
{
    GtkFileChooserNative *native = gtk_file_chooser_native_new("Import KiCad package", GTK_WINDOW(parent->gobj()),
                                                               GTK_FILE_CHOOSER_ACTION_OPEN, "_Open", "_Cancel");
    auto chooser = Glib::wrap(GTK_FILE_CHOOSER(native));
    auto filter = Gtk::FileFilter::create();
    filter->set_name("KiCad package");
    filter->add_pattern("*.kicad_mod");
    chooser->add_filter(filter);

    if (gtk_native_dialog_run(GTK_NATIVE_DIALOG(native)) == GTK_RESPONSE_ACCEPT) {
        return {true, chooser->get_filename()};
    }
    else {
        return {false, ""};
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
        return std::nullopt;
    }
}

class SymbolPinNamesWindow *Dialogs::show_symbol_pin_names_window(class SchematicSymbol *symbol)
{
    if (window_nonmodal)
        return nullptr;
    auto win = new SymbolPinNamesWindow(parent, interface, symbol);
    window_nonmodal = win;
    win->present();
    return win;
}

class RenumberPadsWindow *Dialogs::show_renumber_pads_window(class Package *pkg, const std::set<UUID> &pads)
{
    if (window_nonmodal)
        return nullptr;
    auto win = new RenumberPadsWindow(parent, interface, pkg, pads);
    window_nonmodal = win;
    win->present();
    return win;
}

class GenerateSilkscreenWindow *Dialogs::show_generate_silkscreen_window(class ToolSettings *settings)
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
