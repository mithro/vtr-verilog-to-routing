#include <iostream>
#include <set>
#include <sstream>

#include "globals.h"
#include "vtr_assert.h"

#include "ice40_hlc.h"

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
    const t_block_type t = _get_type(pin->parent_node->pb_type->name);
    return t.type == NULL_TYPE || (t.type == BLK_TYPE && t.type_blk == BLK_IG_TYPE);
}

static int _find_pb_index(const t_pb_graph_node *n) {
    for (; n; n = n->parent_pb_graph_node)
        if (n->pb_type->num_pb > 1)
            return n->placement_index;
    return -1;
}

static int _find_cell_index(const t_pb_graph_pin *pin) {
    // Find the first parent with a non-zero number of cells, but it can't be the direct parent
    const t_pb_graph_node *n = pin->parent_node;
    return n ? _find_pb_index(n->parent_pb_graph_node) : -1;
}

#if 0
static void _write_pin_name(std::ostream &o, const t_pb_graph_pin *pin) {
    const int cell = _find_cell_index(pin);
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
    const int cell = _find_cell_index(pin);
    const t_block_type t = _get_type(pin->parent_node->pb_type->name);

    if (t.type == BEL_TYPE) {
        if (this_cell != cell)
            o << "lutff_" << cell << '/';
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

#if 0
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

ICE40HLCWriterVisitor::ICE40HLCWriterVisitor(std::ostream& os)
        : os_(os)
        , cur_clb_(nullptr) {
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

    const std::map<string, string> tile_types = {{"PLB", "logic"}, {"VPR_PAD", "io"}};

    os_ << endl <<
        tile_types.at(block_type.name) << "_tile " << (block_loc.x - 1) << ' ' <<
        (block_loc.y - 1) << " {" << endl;

    const std::map<string, string> element_prefixes = {{"PLB", "lutff"}, {"VPR_PAD", "io"}};
    element_prefix_ = element_prefixes.at(block_type.name);

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
    const t_pb_route *driver = nullptr;
    int driver_id = pb_route->driver_pb_pin_id;
    while (driver_id != -1) {
        driver = &top_pb_route[driver_id];
        if (!_ignore_pin(driver->pb_graph_pin))
            break;
        driver_id = driver->driver_pb_pin_id;
    }

    // Found no non-ignored driver
    if (driver_id == -1 || !driver)
        return;

    links_.emplace_back(link{driver->pb_graph_pin, pin, pb});
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

void ICE40HLCWriterVisitor::close_tile() {
    using std::endl;
    using std::set;
    using std::string;

    // List the pins
    typedef std::pair<const t_pb_graph_pin*, const t_pb*> t_pin_atom;
    set<t_pin_atom> pins;
    for (const link &l : links_) {
        pins.insert({l.source_, l.pb_});
        pins.insert({l.sink_, l.pb_});
    }

    // Find which cells are present
    std::map<int, set<t_pin_atom>> cells;
    for (const t_pin_atom &p : pins) {
        const int cell = _find_cell_index(p.first);
        if (cell != -1)
            cells[cell].insert(p);
    }

    // Add the chains for each element
    for (const auto cell : cells) {
        std::ostringstream ss;
        ss << cell.first;
        auto &element_lines = (*elements_.insert(
            std::make_pair(ss.str(), std::vector<string>())).first).second;
        for (const t_pin_atom &pa : cell.second) {
            std::ostringstream ss2;
            const auto input_chain = collect_chain(pa.first, true);
            if (!input_chain.empty())
                _write_chain(ss2, input_chain, cell.first);
            else {
                const auto output_chain = collect_chain(pa.first, false);
                if (output_chain.empty())
                    continue;
                _write_chain(ss2, output_chain, cell.first);
            }

#if 0
            ss2 << "  # " << pa.second->name;
#endif
            element_lines.push_back(ss2.str());
        }
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
}
