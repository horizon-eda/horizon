#include "odb_export.hpp"
#include "board/board.hpp"
#include "board/gerber_output_settings.hpp"
#include "board/board_layers.hpp"
#include "canvas_odb.hpp"
#include "db.hpp"
#include "odb_util.hpp"
#include "export_util/tree_writer_fs.hpp"
#include "export_util/tree_writer_archive.hpp"
#include "util/util.hpp"
#include "track_graph.hpp"
#include "util/str_util.hpp"

namespace horizon {


struct Context {
    Context(const Board &brd) : canvas(job, brd)
    {
    }
    ODB::Job job;
    CanvasODB canvas;

    auto &add_layer(const std::string &step_name, int layer_n, ODB::Matrix::Layer::Context context,
                    ODB::Matrix::Layer::Type type)
    {
        const auto name = ODB::get_layer_name(layer_n);
        auto &layer = job.add_matrix_layer(name);
        layer.context = context;
        layer.type = type;
        canvas.layer_features.emplace(layer_n, &job.steps.at(step_name).layer_features.at(name));
        return layer;
    }
};

static std::string get_step_name(const Block &block)
{
    if (block.project_meta.count("project_name"))
        return ODB::make_legal_entity_name(block.project_meta.at("project_name"));
    else
        return "pcb";
}

void export_odb(const Board &brd, const ODBOutputSettings &settings)
{
    if (brd.board_panels.size())
        throw std::runtime_error("panels aren't supported yet");

    Context ctx{brd};
    const std::string step_name = get_step_name(*brd.block);
    ctx.job.add_step(step_name);
    auto &step = ctx.job.steps.at(step_name);
    ctx.canvas.eda_data = &step.eda_data;

    auto job_name_from_settings = settings.job_name;
    trim(job_name_from_settings);
    if (job_name_from_settings.size())
        ctx.job.job_name = ODB::make_legal_entity_name(job_name_from_settings);
    else
        ctx.job.job_name = step_name;

    bool have_top_comp = false;
    bool have_bottom_comp = false;
    {
        std::map<UUID, const Package *> pkgs;
        for (const auto &[uu, pkg] : brd.packages) {
            pkgs.emplace(pkg.package.uuid, &pkg.package);
            if (pkg.flip)
                have_bottom_comp = true;
            else
                have_top_comp = true;
        }
        for (auto &[uu, pkg] : pkgs) {
            step.eda_data.add_package(*pkg);
        }
    }


    using OLayer = ODB::Matrix::Layer;

    // comp top
    if (have_top_comp) {
        auto &layer = ctx.job.add_matrix_layer("comp_+_top");
        layer.context = ODB::Matrix::Layer::Context::BOARD;
        layer.type = ODB::Matrix::Layer::Type::COMPONENT;
        step.comp_top.emplace();
    }

    // paste top
    ctx.add_layer(step_name, BoardLayers::TOP_PASTE, OLayer::Context::BOARD, OLayer::Type::SOLDER_PASTE);

    // silk top
    ctx.add_layer(step_name, BoardLayers::TOP_SILKSCREEN, OLayer::Context::BOARD, OLayer::Type::SILK_SCREEN);

    // mask top
    ctx.add_layer(step_name, BoardLayers::TOP_MASK, OLayer::Context::BOARD, OLayer::Type::SOLDER_MASK);

    // copper(signal) layers
    ctx.add_layer(step_name, BoardLayers::TOP_COPPER, OLayer::Context::BOARD, OLayer::Type::SIGNAL);
    for (unsigned int i = 0; i < brd.get_n_inner_layers(); i++) {
        ctx.add_layer(step_name, -((int)i) - 1, OLayer::Context::BOARD, OLayer::Type::SIGNAL);
    }
    ctx.add_layer(step_name, BoardLayers::BOTTOM_COPPER, OLayer::Context::BOARD, OLayer::Type::SIGNAL);

    // mask bot
    ctx.add_layer(step_name, BoardLayers::BOTTOM_MASK, OLayer::Context::BOARD, OLayer::Type::SOLDER_MASK);

    // silk bot
    ctx.add_layer(step_name, BoardLayers::BOTTOM_SILKSCREEN, OLayer::Context::BOARD, OLayer::Type::SILK_SCREEN);

    // paste bot
    ctx.add_layer(step_name, BoardLayers::BOTTOM_PASTE, OLayer::Context::BOARD, OLayer::Type::SOLDER_PASTE);

    // comp bot
    if (have_bottom_comp) {
        auto &layer = ctx.job.add_matrix_layer("comp_+_bot");
        layer.context = ODB::Matrix::Layer::Context::BOARD;
        layer.type = ODB::Matrix::Layer::Type::COMPONENT;
        step.comp_bot.emplace();
    }

    // drills
    {
        auto &layer = ctx.job.add_matrix_layer(ODB::drills_layer);
        layer.context = ODB::Matrix::Layer::Context::BOARD;
        layer.type = ODB::Matrix::Layer::Type::DRILL;
        layer.span = {ODB::get_layer_name(BoardLayers::TOP_COPPER), ODB::get_layer_name(BoardLayers::BOTTOM_COPPER)};

        ctx.canvas.drill_features = &step.layer_features.at(ODB::drills_layer);
    }
    // rout
    // misc/doc

    ctx.add_layer(step_name, BoardLayers::TOP_ASSEMBLY, OLayer::Context::MISC, OLayer::Type::DOCUMENT);
    ctx.add_layer(step_name, BoardLayers::BOTTOM_ASSEMBLY, OLayer::Context::MISC, OLayer::Type::DOCUMENT);

    for (const auto &[uu, net] : brd.block->nets) {
        step.eda_data.add_net(net);
    }
    for (const auto &[uu, pkg] : brd.packages) {
        auto &comp = step.add_component(pkg);
        auto &opkg = step.eda_data.get_package(pkg.package.uuid);


        {
            auto pads_sorted = pkg.package.get_pads_sorted();
            for (const auto pad : pads_sorted) {
                const auto net_uu = pad->net ? pad->net->uuid : UUID{};
                auto &net = step.eda_data.get_net(net_uu);
                using ST = ODB::EDAData::SubnetToeprint;
                const auto toep_num = comp.toeprints.size();
                auto &subnet = net.add_subnet<ST>(pkg.flip ? ST::Side::BOTTOM : ST::Side::TOP, comp.index, toep_num);
                ctx.canvas.pad_subnets.emplace(std::piecewise_construct, std::forward_as_tuple(uu, pad->uuid),
                                               std::forward_as_tuple(&subnet));

                auto &toep = comp.toeprints.emplace_back(opkg.get_pin(pad->uuid));
                toep.net_num = net.index;
                toep.subnet_num = subnet.index;

                auto pl = pkg.placement;
                if (pkg.flip)
                    pl.invert_angle();
                toep.placement.set_angle(pl.get_angle());
                toep.placement.shift = pl.transform(pad->placement.shift);
            }
        }
    }

    {
        std::map<UUID, TrackGraph> graphs;
        for (const auto &[uu, track] : brd.tracks) {
            if (!track.net)
                continue;
            auto &gr = graphs[track.net->uuid];
            gr.add_track(track);
        }
        for (auto &[uu, gr] : graphs) {
//#define DUMP_TRACK_GRAPH
#ifdef DUMP_TRACK_GRAPH
            const auto name = brd.block->nets.at(uu).name + "_" + (std::string)uu;
            const auto filename = "/tmp/nets/" + name + ".dot";
            gr.dump(brd, filename);
#endif

            gr.merge_edges();

#ifdef DUMP_TRACK_GRAPH
            const auto filename_merged = "/tmp/nets/" + name + "-merged.dot";
            gr.dump(brd, filename_merged);
#endif
            auto &net = step.eda_data.get_net(uu);
            for (auto &edge : gr.edges) {
                if (edge.tracks.size()) {
                    auto &subnet = net.add_subnet<ODB::EDAData::SubnetTrace>();
                    for (auto track : edge.tracks) {
                        ctx.canvas.track_subnets.emplace(track, &subnet);
                    }
                }
            }
        }
    }

    ctx.canvas.update(brd);

    if (auto outline = brd.get_outline(); outline.outline.vertices.size()) {
        auto &profile_feats = step.profile.emplace();
        auto &surf = profile_feats.add_surface();
        surf.data.append_polygon(outline.outline);
        for (const auto &hole : outline.holes) {
            surf.data.append_polygon(hole);
        }
    }


    if (settings.format == ODBOutputSettings::Format::DIRECTORY) {
        const auto dest_dir = fs::u8path(settings.output_directory) / ctx.job.job_name;
        if (fs::exists(dest_dir)) {
            if (!fs::is_empty(dest_dir)) {
                // make sure that we're not removing something that doesn't look like a valid ODB++ job
                if (!fs::is_directory(dest_dir / "matrix"))
                    throw std::runtime_error(dest_dir.u8string() + " doesn't look like a valid ODB++ job");
            }
            fs::remove_all(dest_dir);
        }
        TreeWriterFS tree_writer(settings.output_directory);
        ctx.job.write(tree_writer);
    }
    else {
        static const std::map<ODBOutputSettings::Format, TreeWriterArchive::Type> type_map = {
                {ODBOutputSettings::Format::TGZ, TreeWriterArchive::Type::TGZ},
                {ODBOutputSettings::Format::ZIP, TreeWriterArchive::Type::ZIP},
        };
        TreeWriterArchive tree_writer(settings.output_filename, type_map.at(settings.format));
        ctx.job.write(tree_writer);
    }
}

} // namespace horizon
