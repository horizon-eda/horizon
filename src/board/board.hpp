#pragma once
#include "block/block.hpp"
#include "board_hole.hpp"
#include "board_package.hpp"
#include "board_rules.hpp"
#include "clipper/clipper.hpp"
#include "common/dimension.hpp"
#include "common/hole.hpp"
#include "board_junction.hpp"
#include "common/layer_provider.hpp"
#include "common/polygon.hpp"
#include "common/keepout.hpp"
#include "common/pdf_export_settings.hpp"
#include "gerber_output_settings.hpp"
#include "odb_output_settings.hpp"
#include "nlohmann/json_fwd.hpp"
#include "plane.hpp"
#include "track.hpp"
#include "util/uuid.hpp"
#include "util/warning.hpp"
#include "via.hpp"
#include "connection_line.hpp"
#include "step_export_settings.hpp"
#include "pnp_export_settings.hpp"
#include "airwire.hpp"
#include "included_board.hpp"
#include "board_panel.hpp"
#include "common/picture.hpp"
#include "board_decal.hpp"
#include "util/file_version.hpp"
#include "common/grid_settings.hpp"
#include "board_net_tie.hpp"

namespace horizon {
using json = nlohmann::json;

class BoardColors {
public:
    BoardColors();
    Color solder_mask;
    Color silkscreen;
    Color substrate;
};

using plane_update_status_cb_t = std::function<void(const Plane &plane, const std::string &)>;

class Board : public ObjectProvider, public LayerProvider {
private:
    // unsigned int update_nets();
    void propagate_nets();
    std::map<int, Layer> layers;

    void delete_dependants();
    void vacuum_junctions();

public:
    Board(const UUID &uu, const json &, Block &block, IPool &pool, const std::string &board_directory);
    static Board new_from_file(const std::string &filename, Block &block, IPool &pool);
    Board(const UUID &uu, Block &block);
    static unsigned int get_app_version();

    void expand();
    void expand_some();

    Board(const Board &brd);
    Board(shallow_copy_t sh, const Board &brd);
    void operator=(const Board &brd) = delete;
    void update_refs();
    void update_junction_connections();
    void update_airwires(bool fast, const std::set<UUID> &nets);
    void disconnect_package(BoardPackage *pkg);

    void smash_package(BoardPackage *pkg);
    void unsmash_package(BoardPackage *pkg);
    void smash_package_silkscreen_graphics(BoardPackage *pkg);
    void smash_package_outline(BoardPackage &pkg);
    void smash_panel_outline(BoardPanel &panel);

    Junction *get_junction(const UUID &uu) override;
    Polygon *get_polygon(const UUID &uu) override;
    const std::map<int, Layer> &get_layers() const override;
    void set_n_inner_layers(unsigned int n);
    unsigned int get_n_inner_layers() const;
    void update_plane(
            Plane *plane, const class CanvasPatch *ca = nullptr, const class CanvasPads *ca_pads = nullptr,
            plane_update_status_cb_t status_cb = nullptr,
            const std::atomic_bool &cancel = std::atomic_bool(false)); // when ca is given, patches will be read from it
    void update_planes(plane_update_status_cb_t status_cb = nullptr,
                       const std::atomic_bool &cancel = std::atomic_bool(false));
    std::vector<KeepoutContour> get_keepout_contours() const;
    std::pair<Coordi, Coordi> get_bbox() const;
    void update_pdf_export_settings(PDFExportSettings &settings);
    std::map<const BoardPackage *, PnPRow> get_PnP(const PnPExportSettings &settings) const;


    UUID uuid;
    Block *block;
    std::string name;
    std::map<UUID, Polygon> polygons;
    std::map<UUID, BoardHole> holes;
    std::map<UUID, BoardPackage> packages;
    std::map<UUID, BoardJunction> junctions;
    std::map<UUID, Track> tracks;
    std::map<UUID, Via> vias;
    std::map<UUID, Text> texts;
    std::map<UUID, Line> lines;
    std::map<UUID, Arc> arcs;
    std::map<UUID, Plane> planes;
    std::map<UUID, Keepout> keepouts;
    std::map<UUID, Dimension> dimensions;
    std::map<UUID, ConnectionLine> connection_lines;
    std::map<UUID, IncludedBoard> included_boards;
    std::map<UUID, BoardPanel> board_panels;
    std::map<UUID, Picture> pictures;
    std::map<UUID, BoardDecal> decals;
    std::map<UUID, BoardNetTie> net_ties;

    std::vector<Warning> warnings;

    enum class OutputFormat { GERBER, ODB };
    OutputFormat output_format = OutputFormat::GERBER;
    static const LutEnumStr<OutputFormat> output_format_lut;

    BoardRules rules;
    GerberOutputSettings gerber_output_settings;
    ODBOutputSettings odb_output_settings;
    GridSettings grid_settings;

    std::map<UUID, std::list<Airwire>> airwires;

    class StackupLayer {
    public:
        StackupLayer(int l, const json &j);
        StackupLayer(int l);
        json serialize() const;
        int layer;
        uint64_t thickness = 0.035_mm;
        uint64_t substrate_thickness = .1_mm;
    };
    std::map<int, StackupLayer> stackup;

    BoardColors colors;
    PDFExportSettings pdf_export_settings;
    STEPExportSettings step_export_settings;
    PnPExportSettings pnp_export_settings;

    FileVersion version;

    enum ExpandFlags {
        EXPAND_NONE = 0,
        EXPAND_ALL = 0xff,
        EXPAND_PROPAGATE_NETS = (1 << 0),
        EXPAND_AIRWIRES = (1 << 1),
        EXPAND_PACKAGES = (1 << 2),
        EXPAND_VIAS = (1 << 3),
        EXPAND_ALL_AIRWIRES = (1 << 4),
    };

    ExpandFlags expand_flags = EXPAND_ALL;
    std::set<UUID> airwires_expand;

    json serialize() const;
    json serialize_planes() const;
    void load_planes(const json &j);
    void load_planes_from_file(const std::string &filename);
    void save_planes(const std::string &filename);

    void save_pictures(const std::string &dir) const;
    void load_pictures(const std::string &dir);

    ItemSet get_pool_items_used() const;

    void flip_package_layer(int &layer) const;
    int get_package_layer(bool flip, int layer) const;
    ParameterSet get_parameters() const;

    struct Outline {
        Polygon outline;            // clockwise
        std::vector<Polygon> holes; // counter-clockwise

        RulesCheckResult errors;
    };

    Outline get_outline() const;
    Outline get_outline_and_errors() const;

    std::set<LayerRange> get_drill_spans() const;

    std::string board_directory; // for resolving relative paths in included boards

private:
    unsigned int n_inner_layers = 0;
    ClipperLib::Paths get_thermals(class Plane *plane, const class CanvasPads *ca) const;
    void update_all_airwires();
    void update_airwire(bool fast, const UUID &net);

    Board(const Board &brd, CopyMode copy_mode);
    void expand_packages();
    Outline get_outline(bool with_errors) const;
};

inline Board::ExpandFlags operator|(Board::ExpandFlags a, Board::ExpandFlags b)
{
    return static_cast<Board::ExpandFlags>(static_cast<int>(a) | static_cast<int>(b));
}

inline Board::ExpandFlags operator|=(Board::ExpandFlags &a, Board::ExpandFlags b)
{
    return a = (a | b);
}


} // namespace horizon
