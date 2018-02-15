#include "catch.hpp"

#include "fasm.h"
#include "vtr_math.h"

TEST_CASE("get_fasm_type", "[fasm]") {
    REQUIRE(_get_fasm_type("BLK_MB-hello").type == FASM_BLK_TYPE);
    REQUIRE(_get_fasm_type("BLK_MB-hello").type_blk == FASM_BLK_MB_TYPE);

    REQUIRE(_get_fasm_type("BLK_SI-hello").type == FASM_BLK_TYPE);
    REQUIRE(_get_fasm_type("BLK_SI-hello").type_blk == FASM_BLK_XX_TYPE);

    REQUIRE(_get_fasm_type("BLK_TI-hello").type == FASM_BLK_TYPE);
    REQUIRE(_get_fasm_type("BLK_TI-hello").type_blk == FASM_BLK_XX_TYPE);

    REQUIRE(_get_fasm_type("BLK_IG-hello").type == FASM_BLK_TYPE);
    REQUIRE(_get_fasm_type("BLK_IG-hello").type_blk == FASM_BLK_IG_TYPE);

    REQUIRE(_get_fasm_type("BEL_XR-hello").type == FASM_BEL_TYPE);
    REQUIRE(_get_fasm_type("BEL_XR-hello").type_bel == FASM_BEL_XR_TYPE);

    REQUIRE(_get_fasm_type("BEL_XL-hello").type == FASM_BEL_TYPE);
    REQUIRE(_get_fasm_type("BEL_XL-hello").type_bel == FASM_BEL_XL_TYPE);

    REQUIRE(_get_fasm_type("BEL_LT-hello").type == FASM_BEL_TYPE);
    REQUIRE(_get_fasm_type("BEL_LT-hello").type_bel == FASM_BEL_LT_TYPE);

    REQUIRE(_get_fasm_type("BEL_FF-hello").type == FASM_BEL_TYPE);
    REQUIRE(_get_fasm_type("BEL_FF-hello").type_bel == FASM_BEL_FF_TYPE);

    REQUIRE(_get_fasm_type("BEL_LL-hello").type == FASM_BEL_TYPE);
    REQUIRE(_get_fasm_type("BEL_LL-hello").type_bel == FASM_BEL_LL_TYPE);

    REQUIRE(_get_fasm_type("BEL_BB-hello").type == FASM_BEL_TYPE);
    REQUIRE(_get_fasm_type("BEL_BB-hello").type_bel == FASM_BEL_BB_TYPE);

    REQUIRE(_get_fasm_type("PAD_IN-hello").type == FASM_PAD_TYPE);
    REQUIRE(_get_fasm_type("PAD_IN-hello").type_pad == FASM_PAD_IN_TYPE);

    REQUIRE(_get_fasm_type("PAD_OT-hello").type == FASM_PAD_TYPE);
    REQUIRE(_get_fasm_type("PAD_OT-hello").type_pad == FASM_PAD_OT_TYPE);
}


