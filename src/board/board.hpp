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
#include <fstream>
#include <map>
#include <vector>

namespace horizon {
using json = nlohmann::json;

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
    void operator=(const Board &brd) = delete;
    void update_refs();
    void update_airwires(bool fast = false, const std::set<UUID> &nets = {});
    void disconnect_package(BoardPackage *pkg);

    void smash_package(BoardPackage *pkg);
    void unsmash_package(BoardPackage *pkg);
    void smash_package_silkscreen_graphics(BoardPackage *pkg);

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

    UUID uuid;
    Block *block;
    std::string name;
    std::map<UUID, Polygon> polygons;
    std::map<UUID, BoardHole> holes;
    std::map<UUID, BoardPackage> packages;
    std::map<UUID, Junction> junctions;
    std::map<UUID, Track> tracks;
    std::map<UUID, Track> airwires;
    std::map<UUID, Via> vias;
    std::map<UUID, Text> texts;
    std::map<UUID, Line> lines;
    std::map<UUID, Arc> arcs;
    std::map<UUID, Plane> planes;
    std::map<UUID, Keepout> keepouts;
    std::map<UUID, Dimension> dimensions;
    std::map<UUID, ConnectionLine> connection_lines;

    std::vector<Warning> warnings;

    BoardRules rules;
    FabOutputSettings fab_output_settings;

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

    class Colors {
    public:
        Colors();
        Color solder_mask;
        Color substrate;
    };
    Colors colors;
    PDFExportSettings pdf_export_settings;
    STEPExportSettings step_export_settings;

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

private:
    unsigned int n_inner_layers = 0;
    ClipperLib::Paths get_thermals(class Plane *plane, const class CanvasPads *ca) const;
    void flip_package_layer(int &layer) const;
};
} // namespace horizon
