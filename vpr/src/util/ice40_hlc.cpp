#include <iostream>

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

    std::map<string, string> tile_types = {{"PLB", "logic"}, {"VPR_PAD", "io"}};

    os_ << endl <<
        tile_types[block_type.name] << "_tile " << (block_loc.x - 1) << ' ' <<
        (block_loc.y - 1) << " {" << endl;
    cur_clb_ = clb;
}

void ICE40HLCWriterVisitor::visit_all_impl(const t_pb_route *top_pb_route, const t_pb* pb,
    const t_pb_graph_node* pb_graph_node) {
    (void)top_pb_route;
    (void)pb;
    (void)pb_graph_node;
}

void ICE40HLCWriterVisitor::finish_impl() {
    if (cur_clb_)
        close_tile();
}

void ICE40HLCWriterVisitor::close_tile() {
    os_ << '}' << std::endl;
    cur_clb_ = nullptr;
}
