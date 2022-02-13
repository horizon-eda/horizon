#pragma once
#include "bus.hpp"
#include "component.hpp"
#include "nlohmann/json_fwd.hpp"
#include "net.hpp"
#include "net_class.hpp"
#include "util/uuid.hpp"
#include "bom_export_settings.hpp"
#include "block_instance.hpp"
#include "net_tie.hpp"
#include <map>
#include <set>
#include <vector>
#include "util/item_set.hpp"
#include "util/uuid_vec.hpp"
#include "util/template_util.hpp"

namespace horizon {
using json = nlohmann::json;

/**
 * A block is one level of hierarchy in the netlist.
 * Right now, horizon doesn't support hierarchical designs, but provisions have
 * been made
 * where necessary.
 *
 * A block stores Components (instances of Entities), Buses and Nets.
 */
class Block {
public:
    Block(const UUID &uu, const json &, class IPool &pool, class IBlockProvider &prv);
    Block(const UUID &uu);
    static Block new_from_file(const std::string &filename, IPool &pool, class IBlockProvider &prv);
    static std::map<std::string, std::string> peek_project_meta(const std::string &filename);
    static std::set<UUID> peek_instantiated_blocks(const std::string &filename);
    Net *get_net(const UUID &uu);
    UUID uuid;
    std::string name;
    std::map<UUID, Net> nets;
    std::map<UUID, NetTie> net_ties;
    std::map<UUID, Bus> buses;
    std::map<UUID, Component> components;
    std::map<UUID, BlockInstance> block_instances;
    std::map<UUID, NetClass> net_classes;
    uuid_ptr<NetClass> net_class_default = nullptr;

    std::map<UUIDVec, BlockInstanceMapping> block_instance_mappings;

    std::map<UUID, std::string> group_names;
    std::map<UUID, std::string> tag_names;
    std::map<std::string, std::string> project_meta;
    std::string get_group_name(const UUID &uu) const;
    std::string get_tag_name(const UUID &uu) const;

    BOMExportSettings bom_export_settings;
    std::map<const class Part *, BOMRow> get_BOM(const BOMExportSettings &settings) const;

    bool can_swap_gates(const UUID &comp, const UUID &g1, const UUID &g2) const;
    void swap_gates(const UUID &comp, const UUID &g1, const UUID &g2);

    Block(const Block &block);
    void operator=(const Block &block);

    void merge_nets(Net *net, Net *into);

    /**
     * deletes unreferenced nets
     */
    void vacuum_nets();
    void vacuum_group_tag_names();

    /**
     * Takes pins specified by pins&ports and moves them over to net
     * \param pins UUIDPath of component, gate and pin UUID
     * \param net the destination Net or nullptr for new Net
     */
    struct NetPinsAndPorts {
        std::set<UUIDPath<3>> pins;  // component/gate/pin
        std::set<UUIDPath<2>> ports; // instance/port
    };
    Net *extract_pins(const NetPinsAndPorts &pins, Net *net = nullptr);

    void update_connection_count();

    void update_diffpairs();

    /**
     * creates new net
     * \returns pointer to new Net
     */
    Net *insert_net();

    std::string get_net_name(const UUID &uu) const;

    ItemSet get_pool_items_used() const;

    void update_non_top(Block &other) const;

    void create_instance_mappings();

    Block flatten() const;

    UUID get_uuid() const;

    BlockInstanceMapping::ComponentInfo get_component_info(const Component &comp, const UUIDVec &instance_path) const;
    std::string get_refdes(const Component &comp, const UUIDVec &instance_path) const;
    void set_refdes(Component &comp, const UUIDVec &instance_path, const std::string &rd);
    void set_nopopulate(Component &comp, const UUIDVec &instance_path, bool nopopulate);

    template <bool c> struct BlockItem {
        BlockItem(make_const_ref_t<c, Block> b, const UUIDVec &p) : block(b), instance_path(p)
        {
        }
        make_const_ref_t<c, Block> block;
        UUIDVec instance_path;
    };

    std::vector<BlockItem<false>> get_instantiated_blocks();
    std::vector<BlockItem<true>> get_instantiated_blocks() const;
    std::vector<BlockItem<false>> get_instantiated_blocks_and_top();
    std::vector<BlockItem<true>> get_instantiated_blocks_and_top() const;

    std::string instance_path_to_string(const UUIDVec &instance_path) const;

    bool can_delete_power_net(const UUID &uu) const;

    bool can_add_block_instance(const UUID &where, const UUID &block_inst) const;

    json serialize() const;

    static const size_t max_instance_path_len;
    static bool instance_path_too_long(const UUIDVec &path, const char *funcname);

private:
    void update_refs();
    std::vector<BlockItem<false>> get_instantiated_blocks(bool inc_top);
    std::vector<BlockItem<true>> get_instantiated_blocks(bool inc_top) const;
};
} // namespace horizon
