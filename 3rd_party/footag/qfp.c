/*
 * Copyright 2019 Martin Åberg
 *
 * This file is part of Footag.
 *
 * Footag is free software: you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * Footag is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program. If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * Orientation according to JEDEC
 */

#include "priv.h"
#include <footag/ipc7351b.h>

static const struct footag_typeinfo info_qfp = {
        .type   = FOOTAG_TYPE_QFP,
        .name   = "QFP",
        .brief  = "Quad Flat Package (plastic, ceramic, etc.)",
};

static const struct footag_param temp[] = {
        PARAM_TOPIC("Body"),
        PARAM_L(BODY_L,  "Length",     "D1",  7.00),
        PARAM_L(BODY_W,  "Width",      "E1",  7.00),
        /*
         * A:  from PCB to top of body
         * A1: from PCB to bottom of body
         * A2: from bottom of body to top of body
         */
#if 0
        PARAM_T(BODY_H,  "Height",     "A2", 1.25, 1.65),
        PARAM_T(BODY_S,  "Seating plane", "A1", 0.10, 0.25),
#endif
        PARAM_I(BODY_N,  "Pins (1...)", "-",   8, 1, 1, 256),
        PARAM_I(BODY_Nx, "Pins (vert)", "-",   8, 1, 1, 256),

        PARAM_TOPIC("Lead"),
        PARAM_L(LEAD_P,  "Pitch",      "e",  0.80),
        PARAM_T(LEAD_L,  "Length",     "L",  0.45, 0.75),
        PARAM_T(LEAD_W,  "Width",      "b",  0.30, 0.45),
        PARAM_T(LEAD_S,  "Span (1...)", "D",  9.00, 0.25),
        PARAM_T(LEAD_Sx, "Span (vert)", "E",  9.00, 0.25),
        PARAM_CALC_IPC7351B,
        PARAM_PADSTACK_SMD_RECTS,
        PARAM_TERM,
};

static int calc(struct footag_ctx *ctx)
{
        struct footag_spec *const s = &ctx->spec;
        struct ipcb_twospec spec_row;
        struct ipcb_twospec spec_col;
        const struct ipcb_comp comp_row = {
                .l = GETID(ctx, LEAD_S)->t,
                .t = GETID(ctx, LEAD_L)->t,
                .w = GETID(ctx, LEAD_W)->t,
        };
        const struct ipcb_comp comp_col = {
                .l = GETID(ctx, LEAD_Sx)->t,
                .t = GETID(ctx, LEAD_L)->t,
                .w = GETID(ctx, LEAD_W)->t,
        };
        const struct ipcb_attr attr = {
                .density = footag_get_density(&GETID(ctx, CALC_D)->e),
                .f = GETID(ctx, CALC_F)->l,
                .p = GETID(ctx, CALC_P)->l,
        };
        const double pitch      = GETID(ctx, LEAD_P)->l;
        const int rows          = intmax(1, GETID(ctx, BODY_N)->i.val);
        const int cols          = intmax(1, GETID(ctx, BODY_Nx)->i.val);
        const int npads         = 2 * (rows + cols);
        int ret;

        ret = footag_realloc_pads(ctx, npads);
        if (ret) { return ret; }

        ret = ipcb_get_gullwing(&spec_row, &comp_row, &attr, pitch);
        if (ret) { return ret; }
        ret = ipcb_get_gullwing(&spec_col, &comp_col, &attr, pitch);
        if (ret) { return ret; }

        footag_genquad(
                s->pads,
                spec_row.land.dist,
                spec_row.land.w, spec_row.land.h,
                spec_col.land.dist,
                spec_col.land.w, spec_col.land.h,
                pitch,
                rows, cols,
                GETID(ctx, CALC_STACK)->e.val
        );
        s->body = footag_crlimit(
                GETID(ctx, BODY_W)->l,
                GETID(ctx, BODY_L)->l
        );
        footag_ipc7351b_setrrectpads(&ctx->spec);
        footag_snap(s, ROUNDOFF_TO_GRID[GETID(ctx, CALC_ROUND)->e.val]);
        footag_setcourtyard(s, spec_row.cyexc);
        footag_setref_ipc7351b(s, &spec_row.ref);

        return 0;
}

const struct footag_op footag_op_qfp = {
        .size   = sizeof (struct footag_ctx),
        .init   = footag_init_default,
        .fini   = footag_fini_default,
        .calc   = calc,
        .info   = &info_qfp,
        .temp   = &temp[0],
};

