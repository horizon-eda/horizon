#include "dialogs.hpp"
#include "map_uuid_path.hpp"
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
#include "annotate.hpp"
#include "pool/part.hpp"
#include "edit_shape.hpp"
#include "pad_parameter_set_window.hpp"
#include "schematic_properties.hpp"
#include "project_properties.hpp"
#include "edit_via.hpp"
#include "edit_plane_window.hpp"
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
#include "enter_datum_angle_window.hpp"
#include "enter_datum_scale_window.hpp"
#include "router_settings_window.hpp"
#include "edit_custom_value.hpp"
#include "map_net_tie.hpp"
#include "select_via_definition.hpp"
#include <glibmm.h>
#include "pool/ipool.hpp"
#include "widgets/net_selector.hpp"
#include "manage_ports.hpp"
#include "select_block.hpp"
#include "align_and_distribute_window.hpp"
#include "edit_text_window.hpp"
#include "plane_update.hpp"
#include "util/automatic_prefs.hpp"
#include "board/board.hpp"

namespace horizon {
void Dialogs::set_parent(Gtk::Window *w)
{
    parent = w;
}

void Dialogs::set_interface(ImpInterface *intf)
{
    interface = intf;
}

std::optional<UUIDPath<2>> Dialogs::map_uuid_path(const std::string &title,
                                                  const std::map<UUIDPath<2>, std::string> &items)
{
    MapUUIDPathDialog dia(parent, items);
    dia.set_title(title);
    auto r = dia.run();
    if (r == Gtk::RESPONSE_OK && dia.selection_valid) {
        return dia.selected_uuid_path;
    }
    else {
        return {};
    }
}

std::optional<UUID> Dialogs::select_symbol(IPool &pool, const UUID &unit_uuid, const UUID &sym_default)
{
    PoolBrowserDialog dia(parent, ObjectType::SYMBOL, pool);
    auto &br = dynamic_cast<PoolBrowserSymbol &>(dia.get_browser());
    br.set_unit_uuid(unit_uuid);
    if (sym_default)
        br.go_to(sym_default);
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
    using PT = Padstack::Type;
    br.set_padstacks_included({PT::TOP, PT::BOTTOM, PT::THROUGH, PT::MECHANICAL});
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
    using PT = Padstack::Type;
    br.set_padstacks_included({PT::MECHANICAL, PT::HOLE});
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

std::optional<UUID> Dialogs::select_block(const BlocksSchematic &blocks)
{
    SelectBlockDialog dia(parent, blocks);
    auto r = dia.run();
    if (r == Gtk::RESPONSE_OK && dia.valid) {
        return dia.selected_uuid;
    }
    else {
        return {};
    }
}

std::optional<UUID> Dialogs::select_via_definition(const RuleViaDefinitions &rule, const LayerProvider &lprv,
                                                   const class ViaDefinition &via_def_from_rules, class IPool &pool)
{
    SelectViaDefinitionDialog dia(parent, rule, lprv, via_def_from_rules, pool);
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

bool Dialogs::edit_via(std::set<class Via *> &vias, class IPool &pool, IPool &pool_caching,
                       const class LayerProvider &prv, const class RuleViaDefinitions &defs)
{
    EditViaDialog dia(parent, vias, pool, pool_caching, prv, defs);
    return dia.run() == Gtk::RESPONSE_OK;
}

bool Dialogs::edit_custom_value(class SchematicSymbol &sym)
{
    EditCustomValueDialog dia(parent, sym);
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

bool Dialogs::manage_net_classes(IBlockProvider &b)
{
    ManageNetClassesDialog dia(parent, b);
    return dia.run() == Gtk::RESPONSE_OK;
}

bool Dialogs::manage_power_nets(IBlockProvider &b)
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

bool Dialogs::edit_board_hole(std::set<class BoardHole *> &holes, IPool &pool, IPool &pool_caching, Block &block)
{
    BoardHoleDialog dia(parent, holes, pool, pool_caching, block);
    return dia.run() == Gtk::RESPONSE_OK;
}

bool Dialogs::edit_schematic_properties(class IDocumentSchematicBlockSymbol &doc, const UUID &block, const UUID &sheet)
{
    SchematicPropertiesDialog dia(parent, doc);
    dia.select_sheet(block, sheet);
    return dia.run() == Gtk::RESPONSE_OK;
}

bool Dialogs::edit_project_properties(class Block &block)
{
    ProjectPropertiesDialog dia(parent, block);
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
    AskDatumStringDialog dia(parent, label, TextEditor::Lines::SINGLE);
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
    if (entity_uuid)
        br.search();
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

std::optional<UUID> Dialogs::map_net_tie(const std::set<class NetTie *> &net_ties)
{
    MapNetTieDialog dia(parent, net_ties);
    auto r = dia.run();
    if (r == Gtk::RESPONSE_OK && dia.selection_valid) {
        return dia.selected_uuid;
    }
    else {
        return {};
    }
}

std::optional<UUID> Dialogs::select_via_padstack(class IPool &pool)
{
    PoolBrowserDialog dia(parent, ObjectType::PADSTACK, pool);
    auto &br = dynamic_cast<PoolBrowserPadstack &>(dia.get_browser());
    br.set_padstacks_included({Padstack::Type::VIA});
    if (dia.run() == Gtk::RESPONSE_OK) {
        return br.get_selected();
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

bool Dialogs::manage_ports(Block &b)
{
    ManagePortsDialog dia(parent, b);
    return dia.run() == Gtk::RESPONSE_OK;
}

static void bind_board_and_plane(SQLite::Query &q, const Board &brd, const Plane *plane)
{
    q.bind(1, brd.uuid);
    if (plane)
        q.bind(2, plane->uuid);
    else
        q.bind(2, UUID());
}

bool Dialogs::update_plane(class Board &brd, class Plane *plane)
{
    Glib::Timer timer;
    bool retval;
    bool dialog_was_shown = false;
    {
        PlaneUpdateDialog dia(*parent, brd, plane);
        bool immediately_show_dialog = false;
        SQLite::Query q(AutomaticPreferences::get().db,
                        "SELECT visible FROM plane_update_dialog_visibility WHERE board = ? AND plane = ?");
        bind_board_and_plane(q, brd, plane);
        if (q.step()) {
            immediately_show_dialog = q.get<int>(0);
        }

        if (!immediately_show_dialog) {
            auto invisible = gtk_invisible_new();
            gtk_grab_add(invisible);
            parent->get_window()->set_cursor(Gdk::Cursor::create(parent->get_display(), "wait"));
            auto loop = Glib::MainLoop::create();
            auto timeout = Glib::signal_timeout().connect(
                    [&loop] {
                        if (loop->is_running())
                            loop->quit();
                        return false;
                    },
                    2000);
            auto done = dia.signal_done().connect([&loop] {
                if (loop->is_running())
                    loop->quit();
            });
            loop->run();
            gtk_grab_remove(invisible);
            gtk_widget_destroy(invisible);

            parent->get_window()->set_cursor();
            done->disconnect();
            timeout.disconnect();
        }

        if (!dia.is_done()) {
            dialog_was_shown = true;
            while (dia.run() != 1)
                ;
        }
        retval = !dia.was_cancelled();
    }
    timer.stop();
    bool show_dialog = dialog_was_shown;
    if (timer.elapsed() < 2)
        show_dialog = false;

    if (retval) {
        SQLite::Query q(
                AutomaticPreferences::get().db,
                "INSERT OR REPLACE INTO plane_update_dialog_visibility (board, plane, visible) VALUES (?, ?, ?)");
        bind_board_and_plane(q, brd, plane);
        q.bind(3, show_dialog);
        q.step();
    }

    return retval;
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
    if (auto win = dynamic_cast<EnterDatumWindow *>(window_nonmodal)) {
        win->present();
        return win;
    }
    auto win = new EnterDatumWindow(parent, interface, label, def);
    window_nonmodal = win;
    win->signal_hide().connect([this] { close_nonmodal(); });
    win->present();
    return win;
}

EnterDatumAngleWindow *Dialogs::show_enter_datum_angle_window(const std::string &label, uint16_t def)
{
    if (auto win = dynamic_cast<EnterDatumAngleWindow *>(window_nonmodal)) {
        win->present();
        return win;
    }
    auto win = new EnterDatumAngleWindow(parent, interface, label, def);
    window_nonmodal = win;
    win->signal_hide().connect([this] { close_nonmodal(); });
    win->present();
    return win;
}

EnterDatumScaleWindow *Dialogs::show_enter_datum_scale_window(const std::string &label, double def)
{
    if (auto win = dynamic_cast<EnterDatumScaleWindow *>(window_nonmodal)) {
        win->present();
        return win;
    }
    auto win = new EnterDatumScaleWindow(parent, interface, label, def);
    window_nonmodal = win;
    win->signal_hide().connect([this] { close_nonmodal(); });
    win->present();
    return win;
}

RouterSettingsWindow *Dialogs::show_router_settings_window(ToolSettings &settings)
{
    if (auto win = dynamic_cast<RouterSettingsWindow *>(window_nonmodal)) {
        win->present();
        return win;
    }
    auto win = new RouterSettingsWindow(parent, interface, settings);
    window_nonmodal = win;
    win->signal_hide().connect([this] { close_nonmodal(); });
    win->present();
    return win;
}

PadParameterSetWindow *Dialogs::show_pad_parameter_set_window(std::set<class Pad *> &pads, class IPool &pool,
                                                              class Package &pkg)
{
    if (auto win = dynamic_cast<PadParameterSetWindow *>(window_nonmodal)) {
        win->present();
        return win;
    }
    auto win = new PadParameterSetWindow(parent, interface, pads, pool, pkg);
    window_nonmodal = win;
    win->signal_hide().connect([this] { close_nonmodal(); });
    win->present();
    return win;
}

AlignAndDistributeWindow *Dialogs::show_align_and_distribute_window()
{
    if (auto win = dynamic_cast<AlignAndDistributeWindow *>(window_nonmodal)) {
        win->present();
        return win;
    }
    auto win = new AlignAndDistributeWindow(parent, interface);
    window_nonmodal = win;
    win->signal_hide().connect([this] { close_nonmodal(); });
    win->present();
    return win;
}


EditPlaneWindow *Dialogs::show_edit_plane_window(class Plane &plane, class Board &brd)
{
    if (auto win = dynamic_cast<EditPlaneWindow *>(window_nonmodal)) {
        win->present();
        return win;
    }
    auto win = new EditPlaneWindow(parent, interface, plane, brd);
    window_nonmodal = win;
    win->signal_hide().connect([this] { close_nonmodal(); });
    win->present();
    return win;
}


EditTextWindow *Dialogs::show_edit_text_window(class Text &text, bool use_ok)
{
    if (auto win = dynamic_cast<EditTextWindow *>(window_nonmodal)) {
        win->present();
        return win;
    }
    auto win = new EditTextWindow(parent, interface, text, use_ok);
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
