#pragma once
#include "block/block.hpp"
#include "board_hole.hpp"
#include "board_package.hpp"
#include "board_rules.hpp"
#include "clipper/clipper.hpp"
#include "common/dimension.hpp"
#include "common/hole.hpp"
#include "common/junction.hpp"
#include "common/layer_provider.hpp"
#include "common/polygon.hpp"
#include "common/keepout.hpp"
#include "common/pdf_export_settings.hpp"
#include "fab_output_settings.hpp"
#include "nlohmann/json_fwd.hpp"
#include "plane.hpp"
#include "pool/pool.hpp"
#include "track.hpp"
#include "util/uuid.hpp"
#include "util/warning.hpp"
#include "via.hpp"
#include "via_padstack_provider.hpp"
#include "connection_line.hpp"
#include "step_export_settings.hpp"
#include "pnp_export_settings.hpp"
#include "airwire.hpp"
#include "included_board.hpp"
#include "board_panel.hpp"
#include "common/picture.hpp"
#include <fstream>
#include <map>
#include <vector>

namespace horizon {
using json = nlohmann::json;

class BoardColors {
public:
    BoardColors();
    Color solder_mask;
    Color substrate;
};

class Board : public ObjectProvider, public LayerProvider {
private:
    // unsigned int update_nets();
    void propagate_nets();
    std::map<int, Layer> layers;

    void delete_dependants();
    void vacuum_junctions();

public:
    Board(const UUID &uu, const json &, Block &block, Pool &pool, ViaPadstackProvider &vpp);
    static Board new_from_file(const std::string &filename, Block &block, Pool &pool, ViaPadstackProvider &vpp);
    Board(const UUID &uu, Block &block);

    void expand(bool careful = false);
    void expand_packages();

    Board(const Board &brd);
    Board(shallow_copy_t sh, const Board &brd);
    void operator=(const Board &brd) = delete;
    void update_refs();
    void update_airwires(bool fast = false, const std::set<UUID> &nets = {});
    void disconnect_package(BoardPackage *pkg);

    void smash_package(BoardPackage *pkg);
    void copy_package_silkscreen_texts(BoardPackage *dest, const BoardPackage *src);
    void unsmash_package(BoardPackage *pkg);
    void smash_package_silkscreen_graphics(BoardPackage *pkg);
    void smash_package_outline(BoardPackage &pkg);
    void smash_panel_outline(BoardPanel &panel);

    Junction *get_junction(const UUID &uu) override;
    Polygon *get_polygon(const UUID &uu) override;
    const std::map<int, Layer> &get_layers() const override;
    void set_n_inner_layers(unsigned int n);
    unsigned int get_n_inner_layers() const;
    void update_plane(Plane *plane, const class CanvasPatch *ca = nullptr,
                      const class CanvasPads *ca_pads = nullptr); // when ca is given, patches will be read from it
    void update_planes();
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
    std::map<UUID, Junction> junctions;
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

    std::vector<Warning> warnings;

    BoardRules rules;
    FabOutputSettings fab_output_settings;

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

    ClipperLib::Paths obstacles;
    ClipperLib::Path track_path;

    enum ExpandFlags {
        EXPAND_ALL = 0xff,
        EXPAND_PROPAGATE_NETS = (1 << 0),
        EXPAND_AIRWIRES = (1 << 1),
        EXPAND_PACKAGES = (1 << 2)
    };

    ExpandFlags expand_flags = EXPAND_ALL;
    std::set<UUID> packages_expand;

    json serialize() const;
    void save_pictures(const std::string &dir) const;
    void load_pictures(const std::string &dir);

private:
    unsigned int n_inner_layers = 0;
    ClipperLib::Paths get_thermals(class Plane *plane, const class CanvasPads *ca) const;
    void flip_package_layer(int &layer) const;
    Board(const Board &brd, CopyMode copy_mode);
};
} // namespace horizon
