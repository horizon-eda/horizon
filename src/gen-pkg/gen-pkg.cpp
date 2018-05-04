#include "pool/package.hpp"
#include "util/util.hpp"
#include "nlohmann/json.hpp"
#include "pool/pool.hpp"
#include "board/board_layers.hpp"
#include <glibmm/miscutils.h>
#include <giomm.h>

using namespace horizon;

#include "gen-uuids.hpp"

std::unique_ptr<Pool> pool;

const int64_t pitch = 2.54_mm;

static void make_center_rect(Polygon &poly, int64_t w, int64_t h)
{
    poly.vertices.clear();
    poly.append_vertex(Coordi(-w / 2, h / 2));
    poly.append_vertex(Coordi(w / 2, h / 2));
    poly.append_vertex(Coordi(w / 2, -h / 2));
    poly.append_vertex(Coordi(-w / 2, -h / 2));
}

static const UUID &get_other_uuid(int &idx)
{
    return other_uuids.at(idx++);
}

static void make_package_poly(Package &pkg, int &uuid_idx, int64_t w, int64_t h)
{
    auto uu = get_other_uuid(uuid_idx);
    auto &poly = pkg.polygons.emplace(uu, uu).first->second;
    make_center_rect(poly, w, h);
    poly.layer = BoardLayers::TOP_PACKAGE;
}

static void make_courtyard_poly(Package &pkg, int &uuid_idx, int64_t w, int64_t h)
{
    auto uu = get_other_uuid(uuid_idx);
    auto &poly = pkg.polygons.emplace(uu, uu).first->second;
    make_center_rect(poly, w, h);
    poly.layer = BoardLayers::TOP_COURTYARD;
    poly.parameter_class = "courtyard";

    pkg.parameter_set[ParameterID::COURTYARD_EXPANSION] = .25_mm;
    std::stringstream ss;
    ss.imbue(std::locale("C"));
    ss << std::fixed << std::setprecision(3);
    ss << w / 1e6 << "mm " << h / 1e6 << "mm\n";
    ss << "get-parameter [ courtyard_expansion ]\n2 * "
          "+xy\nset-polygon [ courtyard rectangle ";
    ss << "0mm 0mm ]";
    pkg.parameter_program.set_code(ss.str());
}

static void make_assy_poly_chamfer(Package &pkg, int &uuid_idx, int64_t w, int64_t h, int64_t deco_size)
{
    auto uu = get_other_uuid(uuid_idx);
    auto &poly = pkg.polygons.emplace(uu, uu).first->second;
    poly.append_vertex(Coordi(-w / 2 + deco_size, h / 2));
    poly.append_vertex(Coordi(w / 2, h / 2));
    poly.append_vertex(Coordi(w / 2, -h / 2));
    poly.append_vertex(Coordi(-w / 2, -h / 2));
    poly.append_vertex(Coordi(-w / 2, h / 2 - deco_size));
    poly.layer = BoardLayers::TOP_ASSEMBLY;
}

static Text &make_assy_refdes(Package &pkg, int &uuid_idx, int64_t x, int64_t y, int angle_deg)
{
    auto uu = get_other_uuid(uuid_idx);
    auto &txt = pkg.texts.emplace(uu, uu).first->second;
    txt.placement.shift = {x, y};
    txt.placement.set_angle_deg(angle_deg);
    txt.text = "$RD";
    txt.layer = BoardLayers::TOP_ASSEMBLY;
    txt.size = 1.5_mm;
    return txt;
}

static Text &make_silkscreen_refdes(Package &pkg, int &uuid_idx, int64_t x, int64_t y, int angle_deg)
{
    auto uu = get_other_uuid(uuid_idx);
    auto &txt = pkg.texts.emplace(uu, uu).first->second;
    txt.placement.shift = {x, y};
    txt.placement.set_angle_deg(angle_deg);
    txt.text = "$RD";
    txt.layer = BoardLayers::TOP_SILKSCREEN;
    txt.size = 1_mm;
    txt.width = .15_mm;
    return txt;
}

void make_silkscreen_box(Package &pkg, int &uuid_idx, int64_t w, int64_t h, int64_t expand = 0)
{
    std::array<Junction *, 4> junctions;
    for (int i = 0; i < 4; i++) {
        auto uu = get_other_uuid(uuid_idx);
        junctions[i] = &pkg.junctions.emplace(uu, uu).first->second;
    }
    junctions[0]->position = Coordi(-w / 2 - expand, h / 2 + expand);
    junctions[1]->position = Coordi(w / 2 + expand, h / 2 + expand);
    junctions[2]->position = Coordi(w / 2 + expand, -h / 2 - expand);
    junctions[3]->position = Coordi(-w / 2 - expand, -h / 2 - expand);
    for (int i = 0; i < 4; i++) {
        auto j = (i + 1) % 4;
        auto uu = get_other_uuid(uuid_idx);
        auto &line = pkg.lines.emplace(uu, uu).first->second;
        line.from = junctions.at(i);
        line.to = junctions.at(j);
        line.width = .15_mm;
        line.layer = BoardLayers::TOP_SILKSCREEN;
    }
}

void make_silkscreen_arrow(Package &pkg, int &uuid_idx, int64_t x, int64_t y)
{
    std::array<Junction *, 3> junctions;
    for (int i = 0; i < 3; i++) {
        auto uu = get_other_uuid(uuid_idx);
        junctions[i] = &pkg.junctions.emplace(uu, uu).first->second;
    }
    const int64_t w = .75_mm;
    const int64_t h = .75_mm;
    junctions[0]->position = Coordi(x, y);
    junctions[1]->position = Coordi(x - w, y - h / 2);
    junctions[2]->position = Coordi(x - w, y + h / 2);
    for (int i = 0; i < 3; i++) {
        auto j = (i + 1) % 3;
        auto uu = get_other_uuid(uuid_idx);
        auto &line = pkg.lines.emplace(uu, uu).first->second;
        line.from = junctions.at(i);
        line.to = junctions.at(j);
        line.width = .15_mm;
        line.layer = BoardLayers::TOP_SILKSCREEN;
    }
}

Package::Model &make_model(Package &pkg, const std::string &model_filename)
{
    auto uu = model_uuid;
    auto &model = pkg.models
                          .emplace(std::piecewise_construct, std::forward_as_tuple(uu),
                                   std::forward_as_tuple(uu, model_filename))
                          .first->second;
    pkg.default_model = model.uuid;
    return model;
}

static Package make_package_pinhd_1x(unsigned int n_pads)
{
    Package pkg(pin_header_1x_uuids.at(n_pads - 1));
    auto ps = pool->get_padstack("296cf69b-9d53-45e4-aaab-4aedf4087d3a");
    int uuid_idx = 0;
    const int64_t y0 = (n_pads - 1) * pitch / 2;
    for (unsigned int i = 0; i < n_pads; i++) {
        auto uu = pin_pads_uuids.at(i);
        auto &pad = pkg.pads.emplace(std::piecewise_construct, std::forward_as_tuple(uu), std::forward_as_tuple(uu, ps))
                            .first->second;
        pad.name = std::to_string(i + 1);
        pad.placement.shift.y = y0 - i * pitch;
        pad.parameter_set[ParameterID::HOLE_DIAMETER] = 1_mm;
        pad.parameter_set[ParameterID::PAD_DIAMETER] = 1.7_mm;
    }

    make_package_poly(pkg, uuid_idx, pitch, n_pads * pitch);
    make_courtyard_poly(pkg, uuid_idx, pitch, n_pads * pitch);
    make_assy_poly_chamfer(pkg, uuid_idx, pitch, n_pads * pitch, 1_mm);
    make_assy_refdes(pkg, uuid_idx, 0, -(n_pads * pitch) / 2, 90);
    make_silkscreen_refdes(pkg, uuid_idx, -pitch / 2, n_pads * pitch / 2 + 1.27_mm, 0);
    make_silkscreen_box(pkg, uuid_idx, pitch, n_pads * pitch);
    make_silkscreen_arrow(pkg, uuid_idx, -pitch / 2 - .5_mm, y0);

    {
        std::stringstream ss;
        ss << "3d_models/connector/header/2.54mm/PinHeader_1x";
        ss << std::setw(2) << std::setfill('0') << n_pads;
        ss << "_P2.54mm_Vertical.step";
        auto &model = make_model(pkg, ss.str());
        model.y = y0;
    }
    {
        pkg.tags = {"generic", "header"};
        pkg.name = "Pin header 1×" + std::to_string(n_pads) + ", 2.54mm pitch, vertical";
    }

    return pkg;
}

static Package make_package_pinhd_2x(unsigned int n_pads)
{
    Package pkg(pin_header_2x_uuids.at(n_pads - 1));
    auto ps = pool->get_padstack("296cf69b-9d53-45e4-aaab-4aedf4087d3a");
    int uuid_idx = 0;
    const int64_t y0 = (n_pads - 1) * pitch / 2;
    for (unsigned int i = 0; i < n_pads * 2; i++) {
        auto uu = pin_pads_uuids.at(i);
        auto &pad = pkg.pads.emplace(std::piecewise_construct, std::forward_as_tuple(uu), std::forward_as_tuple(uu, ps))
                            .first->second;
        pad.name = std::to_string(i + 1);
        pad.placement.shift.y = y0 - i / 2 * pitch;
        pad.placement.shift.x = i & 1 ? pitch / 2 : -pitch / 2;
        pad.parameter_set[ParameterID::HOLE_DIAMETER] = 1_mm;
        pad.parameter_set[ParameterID::PAD_DIAMETER] = 1.7_mm;
    }

    make_package_poly(pkg, uuid_idx, pitch * 2, n_pads * pitch);
    make_courtyard_poly(pkg, uuid_idx, pitch * 2, n_pads * pitch);
    make_assy_poly_chamfer(pkg, uuid_idx, pitch * 2, n_pads * pitch, 1_mm);
    make_assy_refdes(pkg, uuid_idx, 0, -(n_pads * pitch) / 2, 90);
    make_silkscreen_refdes(pkg, uuid_idx, -pitch, n_pads * pitch / 2 + 1.27_mm, 0);
    make_silkscreen_box(pkg, uuid_idx, pitch * 2, n_pads * pitch);
    make_silkscreen_arrow(pkg, uuid_idx, -pitch - .5_mm, y0);

    {
        std::stringstream ss;
        ss << "3d_models/connector/header/2.54mm/PinHeader_2x";
        ss << std::setw(2) << std::setfill('0') << n_pads;
        ss << "_P2.54mm_Vertical.step";
        auto &model = make_model(pkg, ss.str());
        model.y = y0;
        model.x = -pitch / 2;
    }
    {
        pkg.tags = {"generic", "header"};
        pkg.name = "Pin header 2×" + std::to_string(n_pads) + ", 2.54mm pitch, vertical";
    }

    return pkg;
}

static Package make_package_idc_254_vertical(unsigned int n_pads)
{
    Package pkg(idc_254_shrouded_vertical_uuids.at(n_pads - 1));
    auto ps = pool->get_padstack("296cf69b-9d53-45e4-aaab-4aedf4087d3a");
    int uuid_idx = 0;
    const int64_t y0 = (n_pads - 1) * pitch / 2;
    for (unsigned int i = 0; i < n_pads * 2; i++) {
        auto uu = pin_pads_uuids.at(i);
        auto &pad = pkg.pads.emplace(std::piecewise_construct, std::forward_as_tuple(uu), std::forward_as_tuple(uu, ps))
                            .first->second;
        pad.name = std::to_string(i + 1);
        pad.placement.shift.y = y0 - i / 2 * pitch;
        pad.placement.shift.x = i & 1 ? pitch / 2 : -pitch / 2;
        pad.parameter_set[ParameterID::HOLE_DIAMETER] = 1_mm;
        pad.parameter_set[ParameterID::PAD_DIAMETER] = 1.7_mm;
    }

    int64_t w = 8.8_mm;
    int64_t h = (n_pads - 1) * pitch + 10.16_mm;

    make_package_poly(pkg, uuid_idx, w, h);
    make_courtyard_poly(pkg, uuid_idx, w, h);

    int64_t notch_w = 1.27_mm;
    int64_t notch_h = 5.08_mm;

    {
        auto uu = get_other_uuid(uuid_idx);
        auto &poly = pkg.polygons.emplace(uu, uu).first->second;

        poly.append_vertex(Coordi(-w / 2, h / 2));
        poly.append_vertex(Coordi(w / 2, h / 2));
        poly.append_vertex(Coordi(w / 2, -h / 2));
        poly.append_vertex(Coordi(-w / 2, -h / 2));
        poly.append_vertex(Coordi(-w / 2, -notch_h / 2));
        poly.append_vertex(Coordi(-w / 2 + notch_w, -notch_h / 2));
        poly.append_vertex(Coordi(-w / 2 + notch_w, notch_h / 2));
        poly.append_vertex(Coordi(-w / 2, notch_h / 2));

        poly.layer = BoardLayers::TOP_ASSEMBLY;
    }

    auto &assy_rd = make_assy_refdes(pkg, uuid_idx, 0, -h / 2, 90);
    assy_rd.size = 3_mm;

    make_silkscreen_refdes(pkg, uuid_idx, -w / 2, h / 2 + 1.27_mm, 0);
    {
        std::array<Junction *, 8> junctions;
        for (int i = 0; i < junctions.size(); i++) {
            auto uu = get_other_uuid(uuid_idx);
            junctions[i] = &pkg.junctions.emplace(uu, uu).first->second;
        }
        junctions[0]->position = Coordi(-w / 2, h / 2);
        junctions[1]->position = Coordi(w / 2, h / 2);
        junctions[2]->position = Coordi(w / 2, -h / 2);
        junctions[3]->position = Coordi(-w / 2, -h / 2);
        junctions[4]->position = Coordi(-w / 2, -notch_h / 2);
        junctions[5]->position = Coordi(-w / 2 + notch_w, -notch_h / 2);
        junctions[6]->position = Coordi(-w / 2 + notch_w, notch_h / 2);
        junctions[7]->position = Coordi(-w / 2, notch_h / 2);

        for (int i = 0; i < junctions.size(); i++) {
            auto j = (i + 1) % junctions.size();
            auto uu = get_other_uuid(uuid_idx);
            auto &line = pkg.lines.emplace(uu, uu).first->second;
            line.from = junctions.at(i);
            line.to = junctions.at(j);
            line.width = .15_mm;
            line.layer = BoardLayers::TOP_SILKSCREEN;
        }
    }
    make_silkscreen_arrow(pkg, uuid_idx, -w / 2 - .5_mm, y0);

    {
        std::stringstream ss;
        ss << "3d_models/connector/header-idc/2.54mm/IDC-Header_2x";
        ss << std::setw(2) << std::setfill('0') << n_pads;
        ss << "_P2.54mm_Vertical.step";
        auto &model = make_model(pkg, ss.str());
        model.y = y0;
        model.x = -pitch / 2;
    }
    {
        pkg.tags = {"generic", "header"};
        pkg.name = "IDC header 2×" + std::to_string(n_pads) + ", 2.54mm pitch, vertical";
    }

    return pkg;
}


int main(int c_argc, char *c_argv[])
{
    Gio::init();

    auto pool_base_path = getenv("HORIZON_POOL");
    if (pool_base_path) {
        pool.reset(new Pool(pool_base_path));
    }
    else {
        std::cout << "environment variable HORIZON_POOL not set" << std::endl;
        return 1;
    }

    // 1x 2.54mm
    for (int i = 1; i <= 40; i++) {
        auto dir = Glib::build_filename(pool->get_base_path(), "packages", "connectors", "header", "pin", "2.54",
                                        "1x" + std::to_string(i) + "-vertical");
        if (!Glib::file_test(dir, Glib::FILE_TEST_IS_DIR))
            Gio::File::create_for_path(dir)->make_directory_with_parents();
        save_json_to_file(Glib::build_filename(dir, "package.json"), make_package_pinhd_1x(i).serialize());
    }

    // 2x 2.54mm
    for (int i = 1; i <= 40; i++) {
        auto dir = Glib::build_filename(pool->get_base_path(), "packages", "connectors", "header", "pin", "2.54",
                                        "2x" + std::to_string(i) + "-vertical");
        if (!Glib::file_test(dir, Glib::FILE_TEST_IS_DIR))
            Gio::File::create_for_path(dir)->make_directory_with_parents();
        save_json_to_file(Glib::build_filename(dir, "package.json"), make_package_pinhd_2x(i).serialize());
    }

    // 2.54mm idc shrouded vertical
    {
        const std::vector<int> pin_counts = {3, 4, 5, 6, 7, 8, 10, 13, 15, 17, 20, 25, 30};

        for (int i : pin_counts) {
            auto dir = Glib::build_filename(pool->get_base_path(), "packages", "connectors", "header", "idc", "2.54",
                                            "2x" + std::to_string(i) + "-vertical");
            if (!Glib::file_test(dir, Glib::FILE_TEST_IS_DIR))
                Gio::File::create_for_path(dir)->make_directory_with_parents();
            save_json_to_file(Glib::build_filename(dir, "package.json"), make_package_idc_254_vertical(i).serialize());
        }
    }


    return 0;
}
