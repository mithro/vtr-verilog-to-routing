#include <iostream>
#include <set>
#include <sstream>

#include "atom_netlist_utils.h"
#include "globals.h"
#include "vtr_assert.h"
#include "vtr_logic.h"

#include "ice40_hlc.h"

// #define DEBUG_NETS
// #define DEBUG_NO_IGNORE

/** Get the "block type" from a pb_type.
 */
typedef enum e_ptype {
    NULL_TYPE = 0,

    BEL_TYPE,    // Basic Logic Unit
    BLK_TYPE,    // Block, contains BELs and other blocks.
    PAD_TYPE,    // Input/output pad.

    // End sentinel
    NUM_TYPES
} t_ptype;

constexpr std::array<t_ptype, NUM_TYPES> TYPES = { {
    NULL_TYPE, BEL_TYPE, BLK_TYPE, PAD_TYPE
} };

constexpr std::array<const char*, NUM_TYPES> ptypename = { {
    "   ", "BEL", "BLK", "PAD"
} };

/** BEL / BLE - **B**asic **EL**ement / **B**asic **L**ogic **E**lement.
 * Fundamental building blocks of the FPGA architecture.
 */
typedef enum e_bel_type {
    BEL_NULL_TYPE = 0,

    BEL_XR_TYPE, // Routing MUX -- Multiplexer configured at Place-and-Route time.
    BEL_XL_TYPE, // Logic MUX   -- Multiplexer with select signals under logic control.
    BEL_LT_TYPE, // LUT         -- Lookup table.
    BEL_FF_TYPE, // Flip Flop   --
    BEL_LL_TYPE, // Latch       --
    BEL_BB_TYPE, // Black Box   --

    // End sentinel
    NUM_BEL_TYPES
} t_bel_type;

constexpr std::array<t_bel_type, NUM_BEL_TYPES> BEL_TYPES = { {
    BEL_NULL_TYPE, BEL_XR_TYPE, BEL_XL_TYPE, BEL_LT_TYPE,
    BEL_FF_TYPE, BEL_LL_TYPE, BEL_BB_TYPE
} };

constexpr std::array<const char*, NUM_BEL_TYPES> bel_typename { {
      "  ", "RX", "MX", "LT", "FF", "LL", "BB"
} };

/** Block -- A structure which contains BELs and other blocks.
 */
typedef enum e_blk_type {
    BLK_NULL_TYPE = 0,

    BLK_XX_TYPE, // A block with no special meaning.
                      // XX == SI, then `SITE` in Xilinx Terminology.
                      // XX == TI, then `TILE` in Xilinx Terminology
    BLK_IG_TYPE, // A block which is ignored -- They don't appear in
                      // the output hierarchy and are normally used when
                      // something is needed in the description which
                      // doesn't match actual architecture.
    BLK_TL_TYPE, // A top-level block

    // End sentinel
    NUM_BLK_TYPES,
} t_blk_type;

constexpr std::array<t_blk_type, NUM_BLK_TYPES> BLK_TYPES = { {
    BLK_NULL_TYPE, BLK_XX_TYPE, BLK_IG_TYPE, BLK_TL_TYPE
} };

constexpr std::array<const char*, NUM_BLK_TYPES> blk_typename { {
    "  ", "XX", "IG", "TL"
} };

/** PAD -- Where signals start / end...
 */
typedef enum e_pad_type {
    PAD_NULL_TYPE = 0,

    PAD_IN_TYPE, // A signal input location.
    PAD_OT_TYPE, // A signal output location.

    // End sentinel
    NUM_PAD_TYPES,
} t_pad_type;

constexpr std::array<t_pad_type, NUM_PAD_TYPES> PAD_TYPES = { {
    PAD_NULL_TYPE, PAD_IN_TYPE, PAD_OT_TYPE
} };

constexpr std::array<const char*, NUM_PAD_TYPES> pad_typename { {
    "  ", "IN", "OT",
} };

/**
 */
struct t_block_type {
    t_ptype type;
    union {
        t_bel_type type_bel;
        t_blk_type type_blk;
        t_pad_type type_pad;
        int type_raw;
    };
    std::string name;
};

static t_block_type _get_type(std::string pb_name) {
    VTR_ASSERT(pb_name.size() > 0);

    if (pb_name.size() < 7 || pb_name[3] != '_' || pb_name[6] != '-') {
        t_block_type ftype;
        ftype.type = NULL_TYPE;
        ftype.type_raw = 0;
        return ftype;
    }

    const std::string ptype_str = pb_name.substr(0, 3);
    const std::string stype_str = pb_name.substr(4, 2);

    t_block_type ftype;
    ftype.type = NULL_TYPE;
    ftype.type_raw = 0;
    ftype.name = pb_name.substr(7);

    // Trim the edge specification off
    if (ftype.name[ftype.name.size() - 2] == '_') {
        const auto c = ftype.name[ftype.name.size() - 1];
        if (c == 'L' || c == 'T' || c == 'R' || c == 'B')
            ftype.name = ftype.name.substr(0, ftype.name.size() - 2);
    }

    for (unsigned int i = 0; i < ptypename.size(); i++) {
        if (ptype_str == ptypename[i]) {
            ftype.type = TYPES[i];
            break;
        }
    }

    if (ftype.type == NULL_TYPE)
        return ftype;

    switch (ftype.type) {
    case BEL_TYPE:
        for (unsigned int i = 0; i < bel_typename.size(); i++) {
            if (stype_str == bel_typename[i]) {
                ftype.type_bel = BEL_TYPES[i];
                break;
            }
        }
        break;
    case BLK_TYPE:
        for (unsigned int i = 0; i < blk_typename.size(); i++) {
            if (stype_str == blk_typename[i]) {
                ftype.type_blk = BLK_TYPES[i];
                break;
            }
        }
        break;
    case PAD_TYPE:
        for (unsigned int i = 0; i < pad_typename.size(); i++) {
            if (stype_str == pad_typename[i]) {
                ftype.type_pad = PAD_TYPES[i];
                break;
            }
        }
        break;
    default:
        VTR_ASSERT(false);
    }

    return ftype;
}

static bool _ignore_pin(const t_pb_graph_pin *pin) {
#ifdef DEBUG_NO_IGNORE
    return false;
#endif
    const t_block_type t = _get_type(pin->parent_node->pb_type->name);
    return t.type == NULL_TYPE || (t.type == BLK_TYPE && t.type_blk == BLK_IG_TYPE);
}

static int _find_pb_index(const t_pb_graph_node *n) {
    for (; n; n = n->parent_pb_graph_node)
        if (n->pb_type->num_pb > 1)
            return n->placement_index;
    return -1;
}

static int _find_cell_index(const t_pb_graph_node *n) {
    for (; n; n = n->parent_pb_graph_node) {
        const t_block_type t = _get_type(n->pb_type->name);
        if (n->pb_type->num_pb > 1 && (t.type != BLK_TYPE || t.type_blk != BLK_XX_TYPE))
            return n->placement_index;
    }

    return -1;
}

#ifdef DEBUG_NETS
static void _write_pin_name(std::ostream &o, const t_pb_graph_pin *pin) {
    const int cell = _find_cell_index(pin->parent_node);
    o << pin->parent_node->pb_type->name;
    if (cell >= 0)
        o << '[' << cell << ']';
    o << '.' << pin->port->name;
    if (pin->port->num_pins > 1)
        o << "[" << pin->pin_number << "]";
}
#endif

static void _write_hlc_pin_name(std::ostream &o, const t_pb_graph_pin *pin, int this_cell) {
    const int pb_index = _find_pb_index(pin->parent_node);
    const int cell = _find_cell_index(pin->parent_node);
    const t_block_type t = _get_type(pin->parent_node->pb_type->name);

    if (t.type == BEL_TYPE || t.type == PAD_TYPE) {
        if (this_cell != cell) {
            const std::map<t_ptype, std::string> prefix = {{BEL_TYPE, "lutff"}, {PAD_TYPE, "io"}};
            o << prefix.at(t.type) << '_';
            if (cell >= 0)
                o << cell;
            else
                o << "global";
            o << '/';
        }
    } else if (t.type != BLK_TYPE || t.type_blk != BLK_TL_TYPE) {
        o << t.name;
        if (pb_index >= 0)
            o << pb_index;
        o << '_';
    }

    // Only print the pin name for blocks that are not part of the internal tile routing
    if (t.type != BLK_TYPE || t.type_blk != BLK_XX_TYPE) {
        // Trim off the i_ or o_ direction prefix
        const std::string n = pin->port->name;
        if ((n[0] == 'i' || n[0] == 'o') && n[1] == '_')
            o << n.substr(2);
        else
            o << n;

        if (pin->port->num_pins > 1)
            o << '_';
    }

    if (pin->port->num_pins > 1)
        o << pin->pin_number;

#ifdef DEBUG_NETS
    o << " (";
    _write_pin_name(o, pin);
    o << ')';
#endif
}

static void _write_chain(std::ostream &o, std::list<const t_pb_graph_pin*> chain, int cell) {
    for (auto i = chain.cbegin(); i != chain.cend();) {
        _write_hlc_pin_name(o, *i++, cell);
        if (i == chain.cend())
           break;
        o << " -> ";
    }
}

static const t_pb* _find_top_cb(const t_pb* curr) {
    //Walk up through the pb graph until curr
    //has no parent, at which point it will be the top pb
    const t_pb* parent = curr->parent_pb;
    while(parent != nullptr) {
        curr = parent;
        parent = curr->parent_pb;
    }
    return curr;
}

static const t_pb_route* _find_top_pb_route(const t_pb* curr) {
    const auto& cluster_ctx = g_vpr_ctx.clustering();

    const t_pb* top_pb = _find_top_cb(curr);

    const t_pb_route* top_pb_route = nullptr;
    for(auto blk_id : cluster_ctx.clb_nlist.blocks()) {
        if(cluster_ctx.clb_nlist.block_pb(blk_id) == top_pb) {
            top_pb_route = cluster_ctx.clb_nlist.block_pb(blk_id)->pb_route;
            break;
        }
    }
    VTR_ASSERT(top_pb_route);
    return top_pb_route;
}

static AtomNetId _find_atom_input_logical_net(const t_pb* atom, int atom_input_idx) {
    const t_pb_graph_node* pb_node = atom->pb_graph_node;
    const int cluster_pin_idx = pb_node->input_pins[0][atom_input_idx].pin_count_in_cluster;
    const t_pb_route* top_pb_route = _find_top_pb_route(atom);
    return top_pb_route[cluster_pin_idx].atom_net_id;
}

//Helper function for load_lut_mask() which determines how the LUT inputs were
//permuted compared to the input BLIF
//
//  Since the LUT inputs may have been rotated from the input blif specification we need to
//  figure out this permutation to reflect the physical implementation connectivity.
//
//  We return a permutation map (which is a list of swaps from index to index)
//  which is then applied to do the rotation of the lutmask.
//
//  The net in the atom netlist which was originally connected to pin i, is connected
//  to pin permute[i] in the implementation.
static std::vector<int> _determine_lut_permutation(size_t num_inputs, const t_pb* atom_pb) {
    auto& atom_ctx = g_vpr_ctx.atom();

    std::vector<int> permute(num_inputs, OPEN);

    //Determine the permutation
    //
    //We walk through the logical inputs to this atom (i.e. in the original truth table/netlist)
    //and find the corresponding input in the implementation atom (i.e. in the current netlist)
    auto ports = atom_ctx.nlist.block_input_ports(atom_ctx.lookup.pb_atom(atom_pb));
    if(ports.size() == 1) {
        const t_pb_graph_node* gnode = atom_pb->pb_graph_node;
        VTR_ASSERT(gnode->num_input_ports == 1);
        VTR_ASSERT(gnode->num_input_pins[0] >= (int) num_inputs);

        AtomPortId port_id = *ports.begin();

        for(size_t ipin = 0; ipin < num_inputs; ++ipin) {
            //The net currently connected to input j
            const AtomNetId impl_input_net_id = _find_atom_input_logical_net(atom_pb, ipin);

            //Find the original pin index
            const t_pb_graph_pin* gpin = &gnode->input_pins[0][ipin];
            const BitIndex orig_index = atom_pb->atom_pin_bit_index(gpin);

            if(impl_input_net_id) {
                //If there is a valid net connected in the implementation
                AtomNetId logical_net_id = atom_ctx.nlist.port_net(port_id, orig_index);
                VTR_ASSERT(impl_input_net_id == logical_net_id);

                //Mark the permutation.
                //  The net originally located at orig_index in the atom netlist
                //  was moved to ipin in the implementation
                permute[orig_index] = ipin;
            }
        }
    } else {
        //May have no inputs on a constant generator
        VTR_ASSERT(ports.size() == 0);
    }

    //Fill in any missing values in the permutation (i.e. zeros)
    std::set<int> perm_indicies(permute.begin(), permute.end());
    size_t unused_index = 0;
    for(size_t i = 0; i < permute.size(); i++) {
        if(permute[i] == OPEN) {
            while(perm_indicies.count(unused_index)) {
                unused_index++;
            }
            permute[i] = unused_index;
            perm_indicies.insert(unused_index);
        }
    }

    return permute;
}

ICE40HLCWriterVisitor::ICE40HLCWriterVisitor(std::ostream& os)
        : os_(os)
        , cur_clb_(nullptr)
        , cur_clb_type_(clb_type::UNKNOWN) {
}

void ICE40HLCWriterVisitor::visit_top_impl(const char* top_level_name) {
    using std::endl;

    (void)top_level_name;

    const auto& device_ctx = g_vpr_ctx.device();
    os_ << "device \"" << device_ctx.grid.name() << "\" " <<
        (device_ctx.grid.width() - 2) << ' ' <<
        (device_ctx.grid.height() - 2) << endl << endl <<
        "warmboot = on" << endl;
}

void ICE40HLCWriterVisitor::visit_atom_impl(const t_pb* atom) {
    const auto& atom_ctx = g_vpr_ctx.atom();
    const t_model *const model = atom_ctx.nlist.block_model(atom_ctx.lookup.pb_atom(atom));

    if(model->name != std::string(MODEL_NAMES))
        return;

    // Write the LUT config
    const int num_inputs = atom->pb_graph_node->total_input_pins();
    const std::vector<int> permute = _determine_lut_permutation(num_inputs, atom);
    const auto& truth_table = atom_ctx.nlist.block_truth_table(atom_ctx.lookup.pb_atom(atom));
    const auto permuted_truth_table = permute_truth_table(truth_table, num_inputs, permute);
    const auto mask = truth_table_to_lut_mask(permuted_truth_table, num_inputs);

    const int cell = _find_cell_index(atom->pb_graph_node);
    std::ostringstream ss;
    ss << cell;
    auto &element_lines = (*elements_.insert(
        std::make_pair(ss.str(), std::vector<std::string>())).first).second;

    std::ostringstream ss2;
    ss2 << "out = " << mask.size() << "'b";
    for (const vtr::LogicValue &v : mask)
        ss2 << ((v == vtr::LogicValue::TRUE) ? '1' : '0');
    element_lines.push_back(ss2.str());
}

void ICE40HLCWriterVisitor::visit_clb_impl(ClusterBlockId blk_id, const t_pb* clb) {
    using std::endl;
    using std::string;

    (void)blk_id;

    const auto& place_ctx = g_vpr_ctx.placement();
    const t_pb_type *const t = clb->pb_graph_node->pb_type;

    if (cur_clb_)
        close_tile();
    const auto &block_loc = place_ctx.block_locs[blk_id];
    const t_block_type block_type = _get_type(string(t->name));
    VTR_ASSERT(block_type.type == BLK_TYPE);

    const std::map<string, clb_type> block_types = {{"PLB", clb_type::PLB}, {"PIO", clb_type::PIO}};
    try {
        cur_clb_type_ = block_types.at(block_type.name);
    } catch (const std::out_of_range&) {
        cur_clb_type_ = clb_type::UNKNOWN;
    }

    const std::map<clb_type, string> tile_types = {{clb_type::PLB, "logic"}, {clb_type::PIO, "io"}};

    os_ << endl;

    try {
        os_ << tile_types.at(cur_clb_type_);
    } catch (const std::out_of_range&) {
        os_ << block_type.name;
    }

    const auto& device_ctx = g_vpr_ctx.device();
    os_ << "_tile " << (block_loc.x - 1) << ' ' <<
        (device_ctx.grid.height() - block_loc.y - 2) << " {" << endl;

    const std::map<clb_type, string> element_prefixes = {{clb_type::PLB, "lutff"}, {clb_type::PIO, "io"}};
    try {
        element_prefix_ = element_prefixes.at(cur_clb_type_);
    } catch (const std::out_of_range&) {
        element_prefix_ = block_type.name;
    }

    cur_clb_ = clb;
}

void ICE40HLCWriterVisitor::visit_all_impl(const t_pb_route *top_pb_route, const t_pb* pb,
    const t_pb_graph_node* pb_graph_node) {
    using std::endl;
    using std::string;

    if (!pb)
        return;

    process_ports(top_pb_route, pb, pb_graph_node->num_input_ports,
        pb_graph_node->num_input_pins, pb_graph_node->input_pins);
    process_ports(top_pb_route, pb, pb_graph_node->num_clock_ports,
        pb_graph_node->num_clock_pins, pb_graph_node->clock_pins);
    process_ports(top_pb_route, pb, pb_graph_node->num_output_ports,
        pb_graph_node->num_output_pins, pb_graph_node->output_pins);
}

void ICE40HLCWriterVisitor::finish_impl() {
    if (cur_clb_)
        close_tile();
}

void ICE40HLCWriterVisitor::process_ports(const t_pb_route *top_pb_route,
    const t_pb* pb, const int num_ports, const int *num_pins, const t_pb_graph_pin *const *pins) {
    for (int port_index = 0; port_index < num_ports; ++port_index) {
        for (int pin_index = 0; pin_index < num_pins[port_index]; ++pin_index) {
            const t_pb_graph_pin *const pin = &pins[port_index][pin_index];
            const t_pb_route *const pb_route = &top_pb_route[pin->pin_count_in_cluster];
            if (pb_route->atom_net_id != AtomNetId::INVALID() &&
                !_ignore_pin(pin))
                process_route(top_pb_route, pb_route, pin, pb);
        }
    }
}

void ICE40HLCWriterVisitor::process_route(const t_pb_route *top_pb_route, const t_pb_route *pb_route,
    const t_pb_graph_pin *pin, const t_pb* pb) {
    using std::endl;

    // Iterate up the chain of drivers until a non-ignored pin is encountered
    const t_pb_graph_pin *const driven_pin = pin;
    const t_pb_route *driver = nullptr;
    int driver_id = pb_route->driver_pb_pin_id;
    bool any_mux_edges = false;
    while (driver_id != -1) {
        driver = &top_pb_route[driver_id];
        any_mux_edges |= std::any_of(
            pin->input_edges, pin->input_edges + pin->num_input_edges,
            [](const t_pb_graph_edge *e) { return e->interconnect->type == MUX_INTERC; });
        if (!_ignore_pin(driver->pb_graph_pin))
            break;
        driver_id = driver->driver_pb_pin_id;
        pin = driver->pb_graph_pin;
    }

    // Found a non-ignored driver, beyond a mux
    if (driver_id == -1 || !driver || !any_mux_edges)
        return;

    links_.emplace_back(link{driver->pb_graph_pin, driven_pin, pb});
}

std::list<const t_pb_graph_pin*> ICE40HLCWriterVisitor::collect_chain(
    const t_pb_graph_pin *tip, bool up) {
    typedef const t_pb_graph_pin* t_p;
    typedef std::list<t_p> t_chain;

    t_chain chain({tip});
    std::vector<link>::iterator iter;
    const auto from = up ? &link::sink_ : &link::source_;
    const auto to = up ? &link::source_ : &link::sink_;
    const auto push_head = up ? (void (t_chain::*)(const t_p&))&t_chain::push_front :
        (void (t_chain::*)(const t_p&))&t_chain::push_back;

    do {
        for (iter = links_.begin(); iter != links_.end(); iter++) {
            if ((*iter).*from == tip) {
                tip = (*iter).*to;
                (chain.*push_head)(tip);
                links_.erase(iter);
                break;
            }
        }
    } while (iter != links_.end());

    if (chain.size() <= 1)
        return t_chain();
    return chain;
}

bool ICE40HLCWriterVisitor::write_cell_chains(int cell, const std::set<t_pin_atom> &pin_atoms, bool up) {
    if (pin_atoms.empty())
        return false;

    auto &element_lines = (*elements_.insert(
        std::make_pair(std::to_string(cell), std::vector<std::string>())).first).second;
    bool found_chain = false;
    for (const t_pin_atom &pa : pin_atoms) {
        std::ostringstream ss2;
        const auto chain = collect_chain(pa.first, up);
        if (!chain.empty()) {
            _write_chain(ss2, chain, cell);
            element_lines.push_back(ss2.str());
            found_chain = true;
        }
    }
    return found_chain;
}

void ICE40HLCWriterVisitor::close_tile() {
    using std::endl;
    using std::set;
    using std::string;
    using std::to_string;

    set<int> input_cells, output_cells;

    // List the pins
    set<t_pin_atom> pins;
    for (const link &l : links_) {
        pins.insert({l.source_, l.pb_});
        pins.insert({l.sink_, l.pb_});
    }

    // Find which cells are present
    std::map<int, set<t_pin_atom>> cells;
    for (const t_pin_atom &p : pins) {
        const int cell = _find_cell_index(p.first->parent_node);
        if (cell != -1)
            cells[cell].insert(p);
    }

    // Add the inputs for each element
    for (const auto cell : cells)
        if (cell.first != -1)
            if (write_cell_chains(cell.first, cell.second, true))
                input_cells.insert(cell.first);

    // Add the outputs for each element
    for (const auto cell : cells)
        if (cell.first != -1)
            if (write_cell_chains(cell.first, cell.second, false))
                output_cells.insert(cell.first);

    // Add type-specific options
    switch (cur_clb_type_) {
    case clb_type::PIO:
        for (const auto cell : cells) {
            auto &element_lines = (*elements_.insert(
                std::make_pair(to_string(cell.first), std::vector<string>())).first).second;
            element_lines.push_back("disable_pull_up");

            if (input_cells.find(cell.first) != input_cells.end()) {
                element_lines.push_back("output_pin_type = simple_output_pin");
            }

            if (output_cells.find(cell.first) != output_cells.end()) {
                element_lines.push_back("enable_input");
                element_lines.push_back("input_pin_type = simple_input_pin");
            }
        }
        break;
    default:
        break;
    }

    // Print the element content
    for (auto i = elements_.cbegin(); i != elements_.cend(); i++) {
        os_ << "    " << element_prefix_ << '_' << (*i).first << " {" << endl;
        for (const string &line : (*i).second)
            os_ << "        " << line << endl;
        os_ << "    }" << endl;
    }

    os_ << '}' << std::endl;
    elements_.clear();
    links_.clear();
    cur_clb_ = nullptr;
    cur_clb_type_ = clb_type::UNKNOWN;
}
