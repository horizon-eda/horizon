#include "pool/package.hpp"
#include "util/util.hpp"
#include "util/geom_util.hpp"
#include <nlohmann/json.hpp>
#include "pool/pool.hpp"
#include "board/board_layers.hpp"
#include "pool/part.hpp"
#include "pool/entity.hpp"
#include "pool/symbol.hpp"
#include "pool/pool_manager.hpp"
#include <glibmm/miscutils.h>
#include <giomm.h>
#include <git2.h>
#include "util/autofree_ptr.hpp"
#include "pool-update/pool-update.hpp"
#include "common/object_descr.hpp"
#include "canvas_cairo2.hpp"
#include "util/version.hpp"
#include "board/board.hpp"
#include "export_3d_image/export_3d_image.hpp"
#include "checks/check_entity.hpp"
#include "checks/check_part.hpp"
#include "checks/check_unit.hpp"
#include "checks/check_util.hpp"
#include "checks/check_item.hpp"
#include "util/once.hpp"
#include "frame/frame.hpp"
#include <iostream>

using namespace horizon;

int diff_file_cb_c(const git_diff_delta *delta, float progress, void *pl)
{
    auto db = reinterpret_cast<SQLite::Database *>(pl);
    SQLite::Query q(*db, "INSERT INTO 'git_files' VALUES (?, ?)");
    q.bind(1, std::string(delta->new_file.path));
    q.bind(2, static_cast<int>(delta->status));
    q.step();
    return 0;
}

static std::string delta_to_string(git_delta_t delta)
{
    switch (delta) {
    case GIT_DELTA_ADDED:
        return "New";

    case GIT_DELTA_MODIFIED:
        return "Modified";

    case GIT_DELTA_DELETED:
        return "Deleted";

    case GIT_DELTA_RENAMED:
        return "Renamed";

    default:
        return "Unknown (" + std::to_string(static_cast<int>(delta)) + ")";
    }
}


static std::string surround_if(const char *prefix, const char *suffix, const std::string &s, bool cond = true)
{
    if (s.size() && cond)
        return prefix + s + suffix;
    else
        return s;
}

static const std::string whitespace_warning = "(:warning: has trailing/leading whitespace)";
class PinDirectionMap {
public:
    const std::map<Pin::Direction, std::string> &get()
    {
        if (!m) {
            m = new std::map<Pin::Direction, std::string>;
            for (const auto &it : Pin::direction_names) {
                m->emplace(it.first, it.second);
            }
        }
        return *m;
    }

private:
    std::map<Pin::Direction, std::string> *m = nullptr;
};

static PinDirectionMap pin_direction_map;

class Reviewer {
public:
    int main(int c_argc, char *c_argv[]);

private:
    void print_rules_check_result(const RulesCheckResult &r, const std::string &name = "Checks");
    int count_manufactuer(const std::string &mfr);
    int parse_options(int c_argc, char *c_argv[]);
    void review_symbol(const UUID &uu, const std::string &title_prefix);

    std::ofstream ofs;
    std::optional<Pool> pool;

    std::string images_dir;
    std::string images_prefix;
    std::string labels_filename;
    std::string versions_filename;
    std::string pool_base_path;
    std::string output_filename;
    bool do_pool_update = false;

    std::map<ObjectType, unsigned int> max_versions;
};

int Reviewer::count_manufactuer(const std::string &mfr)
{
    SQLite::Query q(pool->db, "SELECT COUNT(*) FROM parts WHERE manufacturer = ?");
    q.bind(1, mfr);
    q.step();
    return q.get<int>(0);
}


void Reviewer::print_rules_check_result(const RulesCheckResult &r, const std::string &name)
{
    if (r.level != RulesCheckErrorLevel::PASS) {
        ofs << name << " didn't pass\n";
        for (const auto &error : r.errors) {
            ofs << " - ";
            switch (error.level) {
            case RulesCheckErrorLevel::WARN:
                ofs << ":warning: ";
                break;
            case RulesCheckErrorLevel::FAIL:
                ofs << ":x: ";
                break;
            case RulesCheckErrorLevel::PASS:
                ofs << ":heavy_check_mark: ";
                break;
            default:
                ofs << rules_check_error_level_to_string(error.level) << " ";
            }
            ofs << error.comment << "\n";
        }
    }
    else {
        ofs << ":heavy_check_mark: " << name << " passed\n";
        for (const auto &error : r.errors) {
            ofs << " - " << error.comment << "\n";
        }
    }
    ofs << "\n";
}

void Reviewer::review_symbol(const UUID &sym_uu, const std::string &title_prefix)
{
    const auto &pool_sym = *pool->get_symbol(sym_uu);
    Symbol sym = pool_sym;
    sym.expand();
    sym.apply_placement(Placement());
    ofs << title_prefix << " " << sym.name << "\n";
    if (sym.can_expand) {
        ofs << "Is expandable\n\n";
    }
    {
        auto r = sym.rules.check(RuleID::SYMBOL_CHECKS, sym);
        print_rules_check_result(r);
        ofs << "\n";
    }
    for (auto &[uu, txt] : sym.texts) {
        if (txt.text == "$VALUE") {
            txt.text += "\nGroup\nTag";
        }
    }

    if (sym.text_placements.size() == 0) {
        CanvasCairo2 ca;
        ca.load(sym);
        const std::string img_filename = "sym_" + static_cast<std::string>(sym.uuid) + ".png";
        ca.get_image_surface(1, 1.25_mm)->write_to_png(Glib::build_filename(images_dir, img_filename));
        ofs << "![Symbol](" << images_prefix << img_filename << ")\n";
    }
    else {
        ofs << "| Angle | Normal | Mirrored |\n";
        ofs << "| --- | --- | --- |\n";
        for (const auto angle : {0, 90, 180, 270}) {
            ofs << "| " << angle << "° ";
            for (const auto mirror : {false, true}) {
                Placement pl;
                pl.set_angle_deg(angle);
                pl.mirror = mirror;
                sym.apply_placement(pl);
                CanvasCairo2 ca;
                ca.load(sym, pl);
                const std::string img_filename = "sym_" + static_cast<std::string>(sym.uuid) + "_"
                                                 + (mirror ? "m" : "n") + std::to_string(angle) + ".png";
                ca.get_image_surface(1, 1.25_mm)->write_to_png(Glib::build_filename(images_dir, img_filename));
                ofs << "| ![Symbol](" << images_prefix << img_filename << ") ";
            }
            ofs << "\n";
        }
    }
    sym.apply_placement(Placement());
    if (sym.can_expand) {
        ofs << "<details>\n<summary>Expanded by 5</summary>\n\n";
        sym.apply_expand(pool_sym, 5);
        CanvasCairo2 ca;
        ca.load(sym);
        const std::string img_filename = "sym_" + static_cast<std::string>(sym.uuid) + "_expanded.png";
        ca.get_image_surface(1, 1.25_mm)->write_to_png(Glib::build_filename(images_dir, img_filename));
        ofs << "![Symbol](" << images_prefix << img_filename << ")\n";
        ofs << "</details>\n\n";
    }
}

int Reviewer::parse_options(int c_argc, char *c_argv[])
{
    Glib::OptionContext options;
    options.set_summary("horizon pr review");
    options.set_help_enabled();

    Glib::OptionGroup group("pr-review", "pr-review");

    Glib::OptionEntry entry;
    entry.set_long_name("output");
    entry.set_short_name('o');
    entry.set_description("output filename");
    group.add_entry_filename(entry, output_filename);

    Glib::OptionEntry entry_pool_update;
    entry_pool_update.set_long_name("pool-update");
    entry_pool_update.set_short_name('u');
    entry_pool_update.set_description("update pool before generating review");
    group.add_entry(entry_pool_update, do_pool_update);

    Glib::OptionEntry entry_images_dir;
    entry_images_dir.set_long_name("img-dir");
    entry_images_dir.set_short_name('i');
    entry_images_dir.set_description("images directory");
    group.add_entry_filename(entry_images_dir, images_dir);

    Glib::OptionEntry entry_images_prefix;
    entry_images_prefix.set_long_name("img-prefix");
    entry_images_prefix.set_short_name('p');
    entry_images_prefix.set_description("images prefix");
    group.add_entry_filename(entry_images_prefix, images_prefix);

    Glib::OptionEntry entry_labels_filename;
    entry_labels_filename.set_long_name("labels-file");
    entry_labels_filename.set_short_name('l');
    entry_labels_filename.set_description("labels filename");
    group.add_entry_filename(entry_labels_filename, labels_filename);

    Glib::OptionEntry entry_versions_filename;
    entry_versions_filename.set_long_name("versions-file");
    entry_versions_filename.set_short_name('v');
    entry_versions_filename.set_description("versions filename");
    group.add_entry_filename(entry_versions_filename, versions_filename);


    std::vector<std::string> filenames;
    Glib::OptionEntry entry_f;
    entry_f.set_long_name(G_OPTION_REMAINING);
    entry_f.set_short_name(' ');
    entry_f.set_description("Pool directory");
    group.add_entry_filename(entry_f, filenames);

    options.set_main_group(group);
    options.parse(c_argc, c_argv);

    if (filenames.size() != 1) {
        std::cerr << "pool path not specified" << std::endl;
        return 1;
    }

    if (output_filename.size() == 0) {
        std::cerr << "output filename not specified" << std::endl;
        return 1;
    }

    if (images_dir.size() == 0) {
        std::cerr << "image directory not specified" << std::endl;
        return 1;
    }
    pool_base_path = filenames.at(0);
    return 0;
}

int Reviewer::main(int c_argc, char *c_argv[])
{
    if (auto r = parse_options(c_argc, c_argv))
        return r;


    ofs = make_ofstream(output_filename);

    ofs << "This review is brought to you by the Horizon EDA Poolbot commit [" << Version::commit_hash
        << "](https://github.com/horizon-eda/horizon/commit/" << Version::commit_hash << ").\n\n";

    if (do_pool_update) {
        std::list<std::pair<std::string, std::string>> errors;
        pool_update(
                pool_base_path,
                [&errors](PoolUpdateStatus status, std::string filename, std::string detail) {
                    if (status == PoolUpdateStatus::FILE_ERROR) {
                        errors.emplace_back(filename, detail);
                    }
                },
                true);
        if (errors.size()) {
            ofs << "# :bomb: Pool update encountered errors\n";
            ofs << "| File | Detail |\n";
            ofs << "| --- | --- |\n";
            for (const auto &[filename, detail] : errors) {
                ofs << "| " << filename << " | " << detail << " |\n";
            }
            ofs << "\n\n";
            return 0;
        }
    }


    pool.emplace(pool_base_path);


    autofree_ptr<git_repository> repo(git_repository_free);
    if (git_repository_open(&repo.ptr, pool_base_path.c_str()) != 0) {
        throw std::runtime_error("error opening repo");
    }

    autofree_ptr<git_object> treeish_master(git_object_free);
    if (git_revparse_single(&treeish_master.ptr, repo, "master") != 0) {
        throw std::runtime_error("error finding master branch");
    }

    autofree_ptr<git_object> otree_master(git_object_free);
    if (git_object_peel(&otree_master.ptr, treeish_master, GIT_OBJ_TREE) != 0) {
        throw std::runtime_error("error peeling master");
    }

    autofree_ptr<git_tree> tree_master(git_tree_free);


    if (git_tree_lookup(&tree_master.ptr, repo, git_object_id(otree_master)) != 0) {
        throw std::runtime_error("error finding master tree");
    }

    pool->db.execute("CREATE TEMP TABLE 'git_files' ('git_filename' TEXT NOT NULL, 'status' INT NOT NULL);");
    pool->db.execute("BEGIN");
    {
        autofree_ptr<git_diff> diff(git_diff_free);
        git_diff_tree_to_workdir_with_index(&diff.ptr, repo, tree_master, nullptr);
        git_diff_foreach(diff, &diff_file_cb_c, nullptr, nullptr, nullptr, &pool->db);
    }
    pool->db.execute("COMMIT");
    pool->db.execute(
            "CREATE TEMP VIEW git_files_view AS "
            "SELECT type, uuid, name, filename, status FROM git_files INNER JOIN "
            "(SELECT type, uuid, name, filename FROM all_items_view UNION ALL SELECT DISTINCT 'model_3d' AS type, "
            "'00000000-0000-0000-0000-000000000000' as uuid, '' as name, model_filename as filename FROM models) "
            "ON filename=git_filename");
    {
        if (versions_filename.size()) {
            const auto j = load_json_from_file(versions_filename);
            for (const auto &[k, v] : j.at("versions").items()) {
                const auto type = object_type_lut.lookup(k);
                const auto version = v.get<unsigned int>();
                max_versions.emplace(type, version);
            }
        }
    }
    {
        RulesCheckErrorLevel overall_result = RulesCheckErrorLevel::PASS;
        ofs << "# Items in this PR\n";
        ofs << "| State | Type | Name | Checks | Version | Filename |\n";
        ofs << "| --- | --- | --- | --- | --- | --- |\n";
        SQLite::Query q(pool->db, "SELECT type, uuid, name, filename, status FROM git_files_view");
        while (q.step()) {
            const auto type = q.get<ObjectType>(0);
            const auto name = q.get<std::string>(2);
            const auto filename = q.get<std::string>(3);
            ofs << "|" << delta_to_string(static_cast<git_delta_t>(q.get<int>(4))) << " | "
                << object_descriptions.at(type).name << " | " << name;
            if (needs_trim(name))
                ofs << " " << whitespace_warning;
            ofs << " | ";
            const auto r = check_item(*pool, type, UUID(q.get<std::string>(1)));
            accumulate_level(overall_result, r.level);
            switch (r.level) {
            case RulesCheckErrorLevel::WARN:
                ofs << ":warning: ";
                break;
            case RulesCheckErrorLevel::FAIL:
                ofs << ":x: ";
                break;
            case RulesCheckErrorLevel::PASS:
                ofs << ":heavy_check_mark: ";
                break;
            default:;
            }
            ofs << rules_check_error_level_to_string(r.level) << " | ";
            switch (type) {
            case ObjectType::UNIT:
            case ObjectType::SYMBOL:
            case ObjectType::ENTITY:
            case ObjectType::PADSTACK:
            case ObjectType::PACKAGE:
            case ObjectType::PART:
            case ObjectType::DECAL:
            case ObjectType::FRAME: {
                const auto item_j = load_json_from_file(Glib::build_filename(pool_base_path, filename));
                const auto version = item_j.value("version", 0U);
                unsigned int max_version = 0;
                if (max_versions.count(type))
                    max_version = max_versions.at(type);

                ofs << version << " ";
                if (version > max_version)
                    ofs << ":x: (>" << max_version << ")";
                else
                    ofs << ":heavy_check_mark:";
            } break;

            default:
                ofs << "N/A";
            }
            ofs << " | " << filename << "\n";
        }
        ofs << "\n";
        if (labels_filename.size()) {
            std::string label;
            switch (overall_result) {
            case RulesCheckErrorLevel::PASS:
                label = "pass";
                break;
            case RulesCheckErrorLevel::WARN:
                label = "warn";
                break;
            case RulesCheckErrorLevel::FAIL:
                label = "fail";
                break;
            default:;
            }
            if (label.size()) {
                const json j = {"bot: " + label};
                save_json_to_file(labels_filename, j);
            }
        }
    }

    {
        SQLite::Query q(pool->db,
                        "SELECT git_filename FROM git_files LEFT JOIN "
                        "(SELECT filename FROM all_items_view "
                        "UNION ALL SELECT DISTINCT model_filename as filename FROM models) "
                        "ON filename=git_filename WHERE filename is NULL");
        Once once;
        while (q.step()) {
            if (once()) {
                ofs << "# Non-items\n";
            }
            ofs << " - " << q.get<std::string>(0) << "\n";
        }
        ofs << "\n\n";
    }

    {
        SQLite::Query q(pool->db, "SELECT status, git_filename FROM git_files WHERE status != ?");
        q.bind(1, static_cast<int>(GIT_DELTA_ADDED));
        Once once;
        while (q.step()) {
            if (once()) {
                ofs << "# Non-new files\n";
                ofs << "| Status | File |\n";
                ofs << "| --- | --- |\n";
            }
            ofs << "| " << delta_to_string(static_cast<git_delta_t>(q.get<int>(0))) << " | " << q.get<std::string>(1)
                << "|\n";
        }
        ofs << "\n\n";
    }

    pool->db.execute(
            "CREATE TEMP VIEW top_parts AS "
            "SELECT git_files_view.uuid AS part_uuid FROM git_files_view "
            "LEFT JOIN parts ON git_files_view.uuid = parts.uuid "
            "LEFT JOIN git_files_view AS gfv ON gfv.uuid = parts.base "
            "WHERE (gfv.uuid IS NULL OR parts.base = '00000000-0000-0000-0000-000000000000') "
            "AND git_files_view.type = 'part'");
    pool->db.execute(
            "CREATE TEMP VIEW parts_tree AS "
            "WITH RECURSIVE where_used(typex, uuidx, level, path) AS ( SELECT 'part', part_uuid, 0, "
            "part_uuid from top_parts UNION "
            "SELECT dep_type, dep_uuid, level+1, path || '/' || dep_uuid FROM dependencies, where_used "
            "WHERE dependencies.type = where_used.typex "
            "AND dependencies.uuid = where_used.uuidx) "
            "SELECT where_used.typex AS type, all_items_view.name, level, "
            "(SELECT COUNT(*) from git_files_view "
            "WHERE git_files_view.uuid = where_used.uuidx AND "
            "git_files_view.type = where_used.typex) AS in_pr, "
            "where_used.uuidx AS uuid, path "
            "FROM where_used "
            "LEFT JOIN all_items_view "
            "ON where_used.typex = all_items_view.type "
            "AND where_used.uuidx = all_items_view.uuid");
    pool->db.execute(
            "CREATE TEMP VIEW derived_parts_tree AS "
            "WITH RECURSIVE where_used(uuidx, level, root) AS ( SELECT part_uuid, 0, part_uuid "
            "FROM top_parts UNION "
            "SELECT parts.uuid, level+1, root FROM parts, where_used "
            "WHERE parts.base = where_used.uuidx) "
            "SELECT parts.MPN, level, "
            "(SELECT COUNT(*) from git_files_view "
            "WHERE git_files_view.uuid = where_used.uuidx AND "
            "git_files_view.type = 'part') AS in_pr, "
            "where_used.uuidx AS uuid, root "
            "FROM where_used "
            "LEFT JOIN parts ON where_used.uuidx = parts.uuid "
            "ORDER BY root, level");
    pool->db.execute(
            "CREATE TEMP VIEW all_parts_tree AS "
            "SELECT * FROM ("
            "SELECT * FROM parts_tree "
            "UNION SELECT 'model_3d', model_filename, level+1, in_pr, '', path || '/model' FROM parts_tree "
            "INNER JOIN models ON (models.package_uuid = uuid and type = 'package') "
            "UNION SELECT 'symbol', symbols.name, level+1, in_pr, symbols.uuid, path || '/' || symbols.uuid FROM "
            "parts_tree "
            "INNER JOIN symbols ON (symbols.unit = parts_tree.uuid AND type = 'unit')) "
            "ORDER BY path");

    {
        ofs << "# Parts overview (excluding derived)\n";
        ofs << "Bold items are from this PR\n";
        SQLite::Query q(pool->db, "SELECT * FROM all_parts_tree");
        while (q.step()) {
            const auto type = q.get<ObjectType>(0);
            const auto name = q.get<std::string>(1);
            const auto level = q.get<int>(2);
            const auto from_pr = q.get<int>(3);
            for (int i = 0; i < level; i++) {
                ofs << "  ";
            }
            ofs << "- " << surround_if("**", "**", object_descriptions.at(type).name + " " + name, from_pr) << "\n";
        }
    }
    {
        SQLite::Query q(pool->db,
                        "SELECT git_files_view.type, git_files_view.name FROM git_files_view "
                        "LEFT JOIN all_parts_tree ON git_files_view.uuid = all_parts_tree.uuid "
                        "AND git_files_view.type = all_parts_tree.type "
                        "LEFT JOIN derived_parts_tree ON git_files_view.uuid = derived_parts_tree.uuid "
                        "AND git_files_view.type = 'part' "
                        "WHERE all_parts_tree.uuid IS NULL AND derived_parts_tree.uuid IS NULL "
                        "AND git_files_view.type != 'model_3d'");
        Once once;
        while (q.step()) {
            if (once())
                ofs << "# Items not associated with any part\n";

            const auto type = q.get<ObjectType>(0);
            ofs << " - " << object_descriptions.at(type).name << " " << q.get<std::string>(1) << "\n";
        }
    }


    {
        int n_derived = 0;
        {
            SQLite::Query q(pool->db,
                            "SELECT COUNT(*) FROM git_files_view "
                            "LEFT JOIN parts ON git_files_view.uuid = parts.uuid AND git_files_view.type = 'part' "
                            "WHERE parts.base != '00000000-0000-0000-0000-000000000000'");
            if (q.step()) {
                n_derived = q.get<int>(0);
            }
        }

        if (n_derived) {
            {
                ofs << "# Derived parts\n";
                ofs << "Bold items are from this PR\n";
                SQLite::Query q(pool->db, "SELECT * FROM derived_parts_tree");
                while (q.step()) {
                    const auto name = q.get<std::string>(0);
                    const auto level = q.get<int>(1);
                    const auto from_pr = q.get<int>(2);
                    for (int i = 0; i < level; i++) {
                        ofs << "  ";
                    }
                    ofs << "- " << surround_if("**", "**", name, from_pr) << "\n";
                }
            }
            {
                ofs << "# Parts table\n";
                ofs << "Values in italic are inherited\n\n";
                ofs << "| MPN | Value | Manufacturer | Datasheet | Description | Tags |\n";
                ofs << "| --- | ----- | ------------ | --------- | ----------- | ---- |\n";
                SQLite::Query q(pool->db, "SELECT uuid FROM derived_parts_tree");
                while (q.step()) {
                    const auto &part = *pool->get_part(q.get<std::string>(0));

                    auto get_attr = [&part](Part::Attribute attr) {
                        return surround_if("*", "*", part.get_attribute(attr), part.attributes.at(attr).first);
                    };

                    ofs << "| " << get_attr(Part::Attribute::MPN);
                    ofs << "| " << get_attr(Part::Attribute::VALUE);
                    ofs << "| " << get_attr(Part::Attribute::MANUFACTURER);
                    ofs << "| " << get_attr(Part::Attribute::DATASHEET);
                    ofs << "| " << get_attr(Part::Attribute::DESCRIPTION);
                    {
                        SQLite::Query qtags(pool->db, "SELECT tags FROM tags_view WHERE type = 'part' AND uuid = ?");
                        qtags.bind(1, part.uuid);
                        if (qtags.step()) {
                            ofs << "| " << surround_if("*", "*", qtags.get<std::string>(0), part.inherit_tags);
                        }
                    }
                    ofs << "\n";
                }
                ofs << "\n";
            }
        }
    }

    ofs << "# Details\n";
    {
        SQLite::Query q(pool->db, "SELECT uuid FROM derived_parts_tree");
        Once once;
        while (q.step()) {
            if (once())
                ofs << "## Parts\n";
            const auto &part = *pool->get_part(q.get<std::string>(0));
            ofs << "### " << part.get_MPN() << "\n";
            if (part.base)
                ofs << "Inerhits from " << part.base->get_MPN() << "\n\n";
            print_rules_check_result(check_part(part));

            ofs << "| Attribute | Value |\n";
            ofs << "| --- | --- |\n";
            static const std::vector<std::pair<Part::Attribute, std::string>> attrs = {
                    {Part::Attribute::MPN, "MPN"},
                    {Part::Attribute::VALUE, "Value"},
                    {Part::Attribute::MANUFACTURER, "Manufacturer"},
                    {Part::Attribute::DATASHEET, "Datasheet"},
                    {Part::Attribute::DESCRIPTION, "Description"},
            };
            for (const auto &[attr, attr_name] : attrs) {
                const auto val = part.get_attribute(attr);
                ofs << "|" << attr_name << " | " << val;
                if (needs_trim(val))
                    ofs << " " << whitespace_warning;
                if (attr == Part::Attribute::MANUFACTURER) {
                    ofs << " (" << count_manufactuer(val) << " other parts)";
                }
                if (part.attributes.at(attr).first) {
                    ofs << " (inherited)";
                }
                ofs << "\n";
            }
            {
                SQLite::Query qtags(pool->db, "SELECT tags FROM tags_view WHERE type = 'part' AND uuid = ?");
                qtags.bind(1, part.uuid);
                if (qtags.step()) {
                    ofs << "|Tags | " << qtags.get<std::string>(0);
                }
                if (part.inherit_tags)
                    ofs << " (inherited)";
                ofs << "\n";
            }
            if (part.override_prefix != Part::OverridePrefix::NO) {
                ofs << "| Prefix | " << part.get_prefix();
                if (part.override_prefix == Part::OverridePrefix::INHERIT)
                    ofs << " (inherited)";
                ofs << "\n";
            }
            ofs << "\n\n";

            if (part.orderable_MPNs.size()) {
                ofs << "Orderable MPNs\n";
                for (const auto &[uu, MPN] : part.orderable_MPNs) {
                    ofs << " - " << MPN << "\n";
                }
                ofs << "\n\n";
            }

            if (std::count_if(part.flags.begin(), part.flags.end(),
                              [](const auto &x) { return x.second != Part::FlagState::CLEAR; })) {
                static const std::map<Part::Flag, std::string> flag_names = {
                        {Part::Flag::BASE_PART, "Base part"},
                        {Part::Flag::EXCLUDE_BOM, "Exclude from BOM"},
                        {Part::Flag::EXCLUDE_PNP, "Exclude from Pick&Place"},
                };
                ofs << "Flags\n";
                for (const auto &[fl, name] : flag_names) {
                    if (part.get_flag(fl)) {
                        ofs << " - " << name;
                        if (part.flags.at(fl) == Part::FlagState::INHERIT) {
                            ofs << " (inherited)";
                        }
                        ofs << "\n";
                    }
                }
                ofs << "\n\n";
            }

            std::set<std::pair<UUID, UUID>> all_pins;
            for (const auto &[gate_uu, gate] : part.entity->gates) {
                for (const auto &[pin_uu, pin] : gate.unit->pins) {
                    all_pins.emplace(gate_uu, pin_uu);
                }
            }
            if (!part.base) {
                ofs << "| Pad | Gate | Pin |\n";
                ofs << "| --- | --- | --- |\n";
                auto pads_sorted = part.package->get_pads_sorted();
                for (const auto pad : pads_sorted) {
                    if (pad->pool_padstack->type != Padstack::Type::MECHANICAL) {
                        ofs << "| " << pad->name << " | ";
                        if (part.pad_map.count(pad->uuid)) {
                            const auto &it = part.pad_map.at(pad->uuid);
                            ofs << it.gate->name << " | " << it.pin->primary_name << " |\n";
                            all_pins.erase(std::make_pair(it.gate->uuid, it.pin->uuid));
                        }
                        else {
                            ofs << " - | - |\n";
                        }
                    }
                }
                ofs << "\n";
                if (all_pins.size()) {
                    ofs << ":x: unmapped pins:\n";
                    for (const auto &[gate, pin] : all_pins) {
                        ofs << " - " << part.entity->gates.at(gate).name << "."
                            << part.entity->gates.at(gate).unit->pins.at(pin).primary_name << "\n";
                    }
                }
            }
        }
    }

    {
        SQLite::Query q(pool->db, "SELECT uuid from git_files_view where type = 'entity'");
        Once once;
        while (q.step()) {
            if (once())
                ofs << "## Entities\n";
            const auto &entity = *pool->get_entity(q.get<std::string>(0));
            ofs << "### " << entity.name << "\n";

            print_rules_check_result(check_entity(entity));

            ofs << "| Attribute | Value |\n";
            ofs << "| --- | --- |\n";
            ofs << "|Manufacturer | " << entity.manufacturer << " (" << count_manufactuer(entity.manufacturer)
                << " other parts)\n";
            ofs << "|Prefix | " << entity.prefix << "\n";
            {
                SQLite::Query qtags(pool->db, "SELECT tags FROM tags_view WHERE type = 'entity' AND uuid = ?");
                qtags.bind(1, entity.uuid);
                if (qtags.step()) {
                    ofs << "|Tags | " << qtags.get<std::string>(0) << "\n";
                }
            }
            ofs << "\n";

            std::vector<const Gate *> gates_sorted;
            for (const auto &it : entity.gates) {
                gates_sorted.emplace_back(&it.second);
            }
            if (gates_sorted.size()) {
                ofs << "| Gate | Suffix | Swap group | Unit |\n";
                ofs << "| --- | --- | --- | --- |\n";
                std::sort(gates_sorted.begin(), gates_sorted.end(),
                          [](const auto a, const auto b) { return strcmp_natural(a->name, b->name) < 0; });
                for (auto gate : gates_sorted) {
                    ofs << "|" << gate->name << " | " << gate->suffix << " | " << gate->swap_group << " | "
                        << gate->unit->name << "\n";
                }
            }
            else {
                ofs << ":warning: Entity has no gates!\n";
            }
        }
    }
    {
        SQLite::Query q(pool->db, "SELECT DISTINCT uuid from git_files_view where type = 'unit'");
        Once once;
        while (q.step()) {
            if (once())
                ofs << "## Units\n";
            const auto &unit = *pool->get_unit(q.get<std::string>(0));
            ofs << "### " << unit.name << "\n";

            print_rules_check_result(check_unit(unit));

            ofs << "| Attribute | Value |\n";
            ofs << "| --- | --- |\n";
            ofs << "|Manufacturer | " << unit.manufacturer << " (" << count_manufactuer(unit.manufacturer)
                << " other parts)\n";

            ofs << "\n\n";

            std::vector<const Pin *> pins_sorted;
            for (const auto &it : unit.pins) {
                pins_sorted.emplace_back(&it.second);
            }
            if (pins_sorted.size()) {
                const auto has_alts =
                        std::any_of(pins_sorted.begin(), pins_sorted.end(), [](auto x) { return x->names.size(); });

                ofs << "| Pin | Direction ";
                if (has_alts)
                    ofs << "| Alternate names ";
                ofs << "|\n";
                ofs << "| --- | --- ";
                if (has_alts)
                    ofs << "| --- ";
                ofs << "|\n";
                std::sort(pins_sorted.begin(), pins_sorted.end(), [](const auto a, const auto b) {
                    return strcmp_natural(a->primary_name, b->primary_name) < 0;
                });
                for (auto pin : pins_sorted) {
                    std::string alts;
                    for (const auto &[uu, alt] : pin->names) {
                        alts += alt.name + " (" + Pin::direction_abbreviations.at(alt.direction) + "), ";
                    }
                    if (alts.size()) {
                        alts.pop_back();
                        alts.pop_back();
                    }
                    ofs << "|" << pin->primary_name << " | " << pin_direction_map.get().at(pin->direction); //| "
                    if (has_alts)
                        ofs << " | " << alts;
                    ofs << " |\n";
                }
            }
            else {
                ofs << ":x: Unit has no pins!\n";
            }

            {
                bool has_sym = false;
                SQLite::Query q_symbol(pool->db, "SELECT uuid FROM symbols WHERE unit = ?");
                q_symbol.bind(1, unit.uuid);
                while (q_symbol.step()) {
                    has_sym = true;
                    review_symbol(q_symbol.get<std::string>(0), "#### Symbol:");
                }

                if (!has_sym) {
                    ofs << ":x: Unit has no symbols!\n";
                }
            }
        }
    }

    {
        SQLite::Query q(pool->db,
                        "SELECT git_files_view.uuid FROM git_files_view "
                        "LEFT JOIN symbols ON (symbols.uuid = git_files_view.uuid AND git_files_view.type = 'symbol') "
                        "LEFT JOIN git_files_view AS gfv2 ON (symbols.unit = gfv2.uuid AND gfv2.type = 'unit') "
                        "WHERE git_files_view.type = 'symbol' AND gfv2.uuid IS NULL");
        Once once;
        while (q.step()) {
            if (once()) {
                ofs << "## Symbols\n";
            }
            review_symbol(q.get<std::string>(0), "###");
        }
    }

    {
        SQLite::Query q(pool->db, "SELECT DISTINCT uuid from git_files_view where type = 'package'");
        Once once;
        while (q.step()) {
            if (once())
                ofs << "## Packages\n";
            Package pkg = *pool->get_package(q.get<std::string>(0));
            pkg.expand();
            ofs << "### " << pkg.name << "\n";
            ofs << "| Attribute | Value |\n";
            ofs << "| --- | --- |\n";
            ofs << "|Manufacturer | " << pkg.manufacturer << " (" << count_manufactuer(pkg.manufacturer)
                << " other parts)\n";
            if (pkg.alternate_for) {
                ofs << "|Alt. for | " << pkg.alternate_for->name << "\n";
            }
            {
                SQLite::Query qtags(pool->db, "SELECT tags FROM tags_view WHERE type = 'package' AND uuid = ?");
                qtags.bind(1, pkg.uuid);
                if (qtags.step()) {
                    ofs << "|Tags | " << qtags.get<std::string>(0) << "\n";
                }
            }
            ofs << "\n";

            if (auto r = pkg.apply_parameter_set({})) {
                ofs << ":x: Error applying parameter set: " << *r << "\n\n";
            }
            {
                auto r = pkg.rules.check(RuleID::PACKAGE_CHECKS, pkg);
                print_rules_check_result(r, "Package checks");
                ofs << "\n";
            }
            {
                auto &rule_cl = pkg.rules.get_rule_t<RuleClearancePackage>();
                rule_cl.clearance_silkscreen_cu = 0.2_mm;
                rule_cl.clearance_silkscreen_pkg = 0.2_mm;
                auto r = pkg.rules.check(RuleID::CLEARANCE_PACKAGE, pkg);
                print_rules_check_result(r, "Clearance checks");
                ofs << "\n";
            }
            for (auto &[uu, txt] : pkg.texts) {
                if (txt.text == "$RD") {
                    txt.text = "M1234";
                }
            }

            ofs << "\n";
            {
                CanvasCairo2 ca;
                ca.load(pkg);
                const std::string img_filename = "pkg_" + static_cast<std::string>(pkg.uuid) + ".png";
                ca.get_image_surface(5)->write_to_png(Glib::build_filename(images_dir, img_filename));
                ofs << "![Package](" << images_prefix << img_filename << ")\n";
            }
            ofs << "<details>\n<summary>Parameters</summary>\n\n";
            ofs << "| Parameter | Value |\n";
            ofs << "| --- | --- |\n";
            for (const auto &[param, value] : pkg.parameter_set) {
                ofs << "| " << parameter_id_to_name(param) << " | " << dim_to_string(value, false) << "|\n";
            }
            ofs << "\n\n";
            ofs << "```\n";
            ofs << pkg.parameter_program.get_code();
            ofs << "\n```\n\n";
            ofs << "</details>\n\n";

            ofs << "<details>\n<summary>Pads</summary>\n\n";
            {
                std::set<ParameterID> params;
                auto pads_sorted = pkg.get_pads_sorted();
                for (auto pad : pads_sorted) {
                    for (const auto &[param, v] : pad->parameter_set) {
                        params.insert(param);
                    }
                }
                ofs << "| Pad | Padstack";
                for (const auto param : params) {
                    ofs << " | " << parameter_id_to_name(param);
                }
                ofs << " |\n";
                ofs << "| --- | ---";
                for (const auto param : params) {
                    (void)param;
                    ofs << " | ---";
                }
                ofs << " |\n";
                for (auto pad : pads_sorted) {
                    ofs << "| " << pad->name << " | " << pad->padstack.name;
                    for (const auto param : params) {
                        (void)param;
                        if (pad->parameter_set.count(param))
                            ofs << " | " << dim_to_string(pad->parameter_set.at(param), false);
                        else
                            ofs << " | -";
                    }
                    ofs << " |\n";
                }
            }
            ofs << "</details>\n\n";


            ofs << "<details>\n<summary>3D views (";
            {
                const auto n_models = pkg.models.size();
                if (n_models == 0) {
                    ofs << "no models";
                }
                else if (n_models == 1) {
                    ofs << "one model";
                }
                else {
                    ofs << n_models << " models";
                }
            }
            ofs << ")</summary>\n\n";
            {
                Block fake_block(UUID::random());
                Board fake_board(UUID::random(), fake_block);
                fake_board.set_n_inner_layers(0);
                {
                    auto uu = UUID::random();
                    auto &poly = fake_board.polygons.emplace(uu, uu).first->second;
                    poly.layer = BoardLayers::L_OUTLINE;

                    auto bb = pkg.get_bbox();
                    bb.first -= Coordi(5_mm, 5_mm);
                    bb.second += Coordi(5_mm, 5_mm);

                    poly.vertices.emplace_back(bb.first);
                    poly.vertices.emplace_back(Coordi(bb.first.x, bb.second.y));
                    poly.vertices.emplace_back(bb.second);
                    poly.vertices.emplace_back(Coordi(bb.second.x, bb.first.y));
                }

                fake_board.packages.clear();
                auto fake_pkg_uu = UUID::random();
                auto &fake_package = fake_board.packages.emplace(fake_pkg_uu, fake_pkg_uu).first->second;
                fake_package.package = pkg;
                fake_package.pool_package = std::make_shared<Package>(pkg);

                Image3DExporter ex(fake_board, *pool, 1024, 1024);
                ex.view_all();
                ex.set_show_models(false);
                ex.set_projection(Canvas3DBase::Projection::ORTHO);

                ofs << "#### Without model \n";
                ofs << "| Top | Bottom |\n";
                ofs << "| --- | --- |\n";

                {
                    const std::string img_filename =
                            "pkg_" + static_cast<std::string>(pkg.uuid) + "_3d_top_no_model.png";
                    ex.render_to_surface()->write_to_png(Glib::build_filename(images_dir, img_filename));
                    ofs << "| ![3D](" << images_prefix << img_filename << ") ";
                }
                {
                    const std::string img_filename =
                            "pkg_" + static_cast<std::string>(pkg.uuid) + "_3d_bottom_no_model.png";
                    ex.set_cam_elevation(-89.99);
                    ex.set_cam_azimuth(90);
                    ex.render_to_surface()->write_to_png(Glib::build_filename(images_dir, img_filename));
                    ofs << "| ![3D](" << images_prefix << img_filename << ") ";
                }

                ofs << "\n\n";
                ex.set_show_models(true);
                ex.view_all();
                for (const auto &[model_uu, model] : pkg.models) {
                    fake_package.model = model_uu;
                    ex.load_3d_models();
                    ex.view_all();
                    ofs << "#### " << Glib::path_get_basename(model.filename) << "\n";
                    ofs << "| Top | Bottom |\n";
                    ofs << "| --- | --- |\n";

                    {
                        const std::string img_filename = "pkg_" + static_cast<std::string>(pkg.uuid) + "_3d_top_"
                                                         + static_cast<std::string>(model_uu) + ".png";
                        ex.render_to_surface()->write_to_png(Glib::build_filename(images_dir, img_filename));
                        ofs << "| ![3D](" << images_prefix << img_filename << ") ";
                    }
                    {
                        const std::string img_filename = "pkg_" + static_cast<std::string>(pkg.uuid) + "_3d_bottom_"
                                                         + static_cast<std::string>(model_uu) + ".png";
                        ex.set_cam_elevation(-89.99);
                        ex.set_cam_azimuth(90);
                        ex.render_to_surface()->write_to_png(Glib::build_filename(images_dir, img_filename));
                        ofs << "| ![3D](" << images_prefix << img_filename << ") ";
                    }

                    ofs << "\n\n";

                    ofs << "| South  | East | North | West |\n";
                    ofs << "| --- | --- | --- | --- |\n";
                    ex.view_all();
                    ex.set_cam_elevation(45);
                    ex.set_projection(Canvas3DBase::Projection::PERSP);
                    for (const int az : {270, 0, 90, 180}) {
                        ex.set_cam_azimuth(az);
                        const std::string img_filename = "pkg_" + static_cast<std::string>(pkg.uuid) + "_3d_"
                                                         + std::to_string(az) + "_" + static_cast<std::string>(model_uu)
                                                         + ".png";
                        ex.render_to_surface()->write_to_png(Glib::build_filename(images_dir, img_filename));
                        ofs << "| ![3D](" << images_prefix << img_filename << ") ";
                    }
                    ofs << "\n\n";
                }
            }
            ofs << "</details>\n\n";

            if (pkg.pads.size() > 1) {
                ofs << "<details>\n<summary>Pitch analysis</summary>\n\n";
                auto all_pads_sorted = pkg.get_pads_sorted();
                std::vector<const Pad *> pads_sorted;
                pads_sorted.reserve(all_pads_sorted.size());
                for (auto it : all_pads_sorted) {
                    if (it->pool_padstack->type != Padstack::Type::MECHANICAL)
                        pads_sorted.push_back(it);
                }
                std::map<Coordi, unsigned int> pitches;
                for (size_t i = 0; i < pads_sorted.size(); i++) {
                    const auto i_next = (i + 1) % pads_sorted.size();
                    const auto d = pads_sorted.at(i_next)->placement.shift - pads_sorted.at(i)->placement.shift;
                    const Coordi da(std::abs(d.x), std::abs(d.y));
                    if (pitches.count(da))
                        pitches.at(da)++;
                    else
                        pitches.emplace(da, 1);
                }
                ofs << "| X | Y | Count |\n";
                ofs << "| --- | --- | --- |\n";
                {
                    std::vector<std::pair<Coordi, unsigned int>> pitches_sorted;
                    for (const auto &it : pitches) {
                        pitches_sorted.push_back(it);
                    }
                    std::sort(pitches_sorted.begin(), pitches_sorted.end(),
                              [](const auto &a, const auto &b) { return a.second > b.second; });
                    for (const auto &[delta, count] : pitches_sorted) {
                        ofs << "| " << dim_to_string(delta.x, false) << " | " << dim_to_string(delta.y, false) << " | "
                            << count << "\n";
                    }
                }
                ofs << "\n\n";
                ofs << "</details>\n\n";
            }
        }
    }

    {
        SQLite::Query q(pool->db, "SELECT DISTINCT uuid from git_files_view where type = 'decal'");
        Once once;
        while (q.step()) {
            if (once()) {
                ofs << "## Decals\n";
            }
            const auto uu = q.get<UUID>(0);
            const auto &decal = *pool->get_decal(uu);

            ofs << "### " << decal.name << "\n";
            {
                CanvasCairo2 ca;
                ca.load(decal);
                const std::string img_filename = "dec_" + static_cast<std::string>(decal.uuid) + ".png";
                ca.get_image_surface(5)->write_to_png(Glib::build_filename(images_dir, img_filename));
                ofs << "![Decal](" << images_prefix << img_filename << ")\n";
            }
        }
    }

    {
        SQLite::Query q(pool->db, "SELECT DISTINCT uuid from git_files_view where type = 'frame'");
        Once once;
        while (q.step()) {
            if (once()) {
                ofs << "## Frames\n";
            }
            const auto uu = q.get<UUID>(0);
            const auto &frame = *pool->get_frame(uu);

            ofs << "### " << frame.name << "\n";
            ofs << dim_to_string(frame.width, false) << "×" << dim_to_string(frame.height, false) << "\n";
            {
                CanvasCairo2 ca;
                ca.load(frame);
                const std::string img_filename = "frame_" + static_cast<std::string>(frame.uuid) + ".png";
                ca.get_image_surface(0.5)->write_to_png(Glib::build_filename(images_dir, img_filename));
                ofs << "![Frame](" << images_prefix << img_filename << ")\n";
            }
        }
    }
    return 0;
}


int main(int c_argc, char *c_argv[])
{
    Gio::init();
    PoolManager::init();
    git_libgit2_init();
    create_cache_and_config_dir();

    Reviewer rev;
    return rev.main(c_argc, c_argv);


    return 0;
}
