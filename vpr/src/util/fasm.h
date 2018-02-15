#ifndef FASM_H
#define FASM_H

#include <array>
#include <string>
#include <sstream>

#include "vpr_types.h"
#include "vpr_context.h"

#include "physical_types.h"

#include "netlist_walker.h"



/** Get the "block type" from a pb_type.
 */

typedef enum e_fasm_ptype {
	FASM_NULL_TYPE = 0,

	FASM_BEL_TYPE,	// Basic Logic Unit
	FASM_BLK_TYPE,	// Block, contains BELs and other blocks.
	FASM_PAD_TYPE,  // Input/output pad.

	// End sentinel
	NUM_FASM_TYPES
} t_fasm_ptype;

constexpr std::array<t_fasm_ptype, NUM_FASM_TYPES> FASM_TYPES = { {
	FASM_NULL_TYPE, FASM_BEL_TYPE, FASM_BLK_TYPE, FASM_PAD_TYPE
} };
constexpr std::array<const char*, NUM_FASM_TYPES> fasm_typename { {
	"   ", "BEL", "BLK", "PAD"
} };

/** BEL / BLE - **B**asic **EL**ement / **B**asic **L**ogic **E**lement.
 * Fundamental building blocks of the FPGA architecture.
 */
typedef enum e_fasm_bel_type {
	FASM_BEL_NULL_TYPE = 0,

	FASM_BEL_XR_TYPE, // Routing MUX -- Multiplexer configured at Place-and-Route time.
	FASM_BEL_XL_TYPE, // Logic MUX   -- Multiplexer with select signals under logic control.
	FASM_BEL_LT_TYPE, // LUT         -- Lookup table.
	FASM_BEL_FF_TYPE, // Flip Flop   --
	FASM_BEL_LL_TYPE, // Latch       --
	FASM_BEL_BB_TYPE, // Black Box   --

	// End sentinel
	NUM_FASM_BEL_TYPES
} t_fasm_bel_type;

constexpr std::array<t_fasm_bel_type, NUM_FASM_BEL_TYPES> FASM_BEL_TYPES = { {
	FASM_BEL_NULL_TYPE, FASM_BEL_XR_TYPE, FASM_BEL_XL_TYPE, FASM_BEL_LT_TYPE,
	FASM_BEL_FF_TYPE, FASM_BEL_LL_TYPE, FASM_BEL_BB_TYPE
} };
constexpr std::array<const char*, NUM_FASM_BEL_TYPES> fasm_bel_typename { {
//	"  ", "XR", "XL", "LT", "FF", "LL", "BB"
  	"  ", "RX", "MX", "LT", "FF", "LL", "BB"
} };

/** Block -- A structure which contains BELs and other blocks.
 */
typedef enum e_fasm_blk_type {
	FASM_BLK_NULL_TYPE = 0,

	FASM_BLK_MB_TYPE, // A mega-block which contains top level blocks.
	FASM_BLK_XX_TYPE, // A block with no special meaning.
	                  // XX == SI, then `SITE` in Xilinx Terminology.
		              // XX == TI, then `TILE` in Xilinx Terminology
	FASM_BLK_IG_TYPE, // A block which is ignored -- They don't appear in
                      // the output hierarchy and are normally used when
                      // something is needed in the description which
                      // doesn't match actual architecture.

	// End sentinel
	NUM_FASM_BLK_TYPES,
} t_fasm_blk_type;

constexpr std::array<t_fasm_blk_type, NUM_FASM_BLK_TYPES> FASM_BLK_TYPES = { {
	FASM_BLK_NULL_TYPE, FASM_BLK_XX_TYPE, FASM_BLK_IG_TYPE
} };
constexpr std::array<const char*, NUM_FASM_BLK_TYPES> fasm_blk_typename { {
	"  ", "MB", "XX", "IG",
} };


/** PAD -- Where signals start / end...
 */
typedef enum e_fasm_pad_type {
	FASM_PAD_NULL_TYPE = 0,

	FASM_PAD_IN_TYPE, // A signal input location.
	FASM_PAD_OT_TYPE, // A signal output location.

	// End sentinel
	NUM_FASM_PAD_TYPES,
} t_fasm_pad_type;

constexpr std::array<t_fasm_pad_type, NUM_FASM_PAD_TYPES> FASM_PAD_TYPES = { {
	FASM_PAD_NULL_TYPE, FASM_PAD_IN_TYPE, FASM_PAD_OT_TYPE
} };
constexpr std::array<const char*, NUM_FASM_PAD_TYPES> fasm_pad_typename { {
	"  ", "IN", "OT",
} };

/**
 */
struct t_fasm_type {
    t_fasm_ptype type;
    union {
        t_fasm_bel_type type_bel;
        t_fasm_blk_type type_blk;
        t_fasm_pad_type type_pad;
        int type_raw;
    };
};

void fasm_assert(const t_pb_type *pb, t_fasm_type type);
t_fasm_type get_fasm_type(const t_pb_type *);
t_fasm_type _get_fasm_type(std::string pb_name);

std::string get_fasm_name(const t_pb *pb);

class FASMWriterVisitor : public NetlistVisitor {
    public:
        FASMWriterVisitor(std::ostream& f);
    private: //NetlistVisitor interface functions
        void visit_top_impl(const char* top_level_name) override;
		void visit_clb_impl(ClusterBlockId blk_id, const t_pb* clb) override;
		void visit_atom_impl(const t_pb* atom) override;
        void visit_all_impl(const t_pb_route *top_pb_route, const t_pb* pb, const t_pb_graph_node* pb_graph_node) override;
        void finish_impl() override;

        std::ostream& os_;
};

#endif
