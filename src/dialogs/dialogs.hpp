#pragma once
#include <memory>
#include "util/uuid.hpp"
#include "common/common.hpp"
#include "util/uuid_path.hpp"
#include "parameter/set.hpp"
#include <map>
#include <set>
#include <optional>

namespace Gtk {
class Window;
}

namespace horizon {
class Dialogs {
public:
    Dialogs(){};
    void set_parent(Gtk::Window *w);
    void set_interface(class ImpInterface *intf);

    std::pair<bool, UUID> map_pin(const std::vector<std::pair<const class Pin *, bool>> &pins);
    std::pair<bool, UUIDPath<2>> map_symbol(const std::map<UUIDPath<2>, std::string> &gates);
    std::pair<bool, UUID> map_package(const std::vector<std::pair<class Component *, bool>> &components);
    std::pair<bool, UUID> select_symbol(class Pool *p, const UUID &unit_uuid);
    std::pair<bool, UUID> select_part(class Pool *p, const UUID &entity_uuid, const UUID &part_uuid,
                                      bool show_none = false);
    std::pair<bool, UUID> select_entity(class Pool *pool);
    std::pair<bool, UUID> select_unit(class Pool *pool);
    std::pair<bool, UUID> select_padstack(class Pool *pool, const UUID &package_uuid);
    std::pair<bool, UUID> select_hole_padstack(class Pool *pool);
    std::pair<bool, UUID> select_via_padstack(class ViaPadstackProvider *vpp);
    std::pair<bool, UUID> select_net(class Block *block, bool power_only, const UUID &net_default = UUID());
    std::pair<bool, UUID> select_bus(class Block *block);
    std::pair<bool, UUID> select_bus_member(class Block *block, const UUID &bus_uuid);
    std::pair<bool, UUID> select_group_tag(const class Block *block, bool tag_mode, const UUID &current);
    std::pair<bool, UUID> select_included_board(const class Board &brd);
    unsigned int ask_net_merge(class Net *net, class Net *into);
    bool manage_buses(class Block *b);
    bool manage_net_classes(class Block *b);
    bool manage_power_nets(class Block *b);
    bool manage_via_templates(class Board *b, class ViaPadstackProvider *vpp);
    bool manage_included_boards(class Board &b);
    bool edit_pad_parameter_set(std::set<class Pad *> &pads, class Pool *pool, class Package *pkg);
    bool edit_board_hole(std::set<class BoardHole *> &holes, class Pool *pool, class Block *block);
    bool annotate(class Schematic *s);
    bool edit_plane(class Plane *plane, class Board *brd, class Block *block);
    bool edit_keepout(class Keepout *keepout, class IDocument *c, bool add_mode);
    bool edit_stackup(class IDocumentBoard *brd);
    bool edit_schematic_properties(class Schematic *s, class Pool *pool);
    bool edit_frame_properties(class Frame *fr);
    std::pair<bool, int64_t> ask_datum(const std::string &label, int64_t def = 0);
    std::pair<bool, Coordi> ask_datum_coord(const std::string &label, Coordi def = Coordi());
    std::tuple<bool, Coordi, std::pair<bool, bool>> ask_datum_coord2(const std::string &label, Coordi def = Coordi());
    std::pair<bool, std::string> ask_datum_string_multiline(const std::string &label, const std::string &def);
    std::pair<bool, std::string> ask_datum_string(const std::string &label, const std::string &def);
    std::pair<bool, int> ask_datum_angle(const std::string &label, int def = 0);
    bool edit_shapes(std::set<class Shape *> shapes);
    bool edit_via(std::set<class Via *> &vias, class ViaPadstackProvider &vpp);
    std::pair<bool, std::string> ask_dxf_filename();
    std::pair<bool, std::string> ask_kicad_package_filename();
    std::optional<std::string> ask_picture_filename();

    class SymbolPinNamesWindow *show_symbol_pin_names_window(class SchematicSymbol *symbol);
    class RenumberPadsWindow *show_renumber_pads_window(class Package *pkg, const std::set<UUID> &pads);
    class GenerateSilkscreenWindow *show_generate_silkscreen_window(class ToolSettings *settings);
    class EnterDatumWindow *show_enter_datum_window(const std::string &label, int64_t def = 0);

    void close_nonmodal();
    class ToolWindow *get_nonmodal();

private:
    Gtk::Window *parent = nullptr;
    class ImpInterface *interface = nullptr;
    class ToolWindow *window_nonmodal = nullptr;
};
} // namespace horizon
