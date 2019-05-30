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

#include "priv.h"
#include <footag/ipc7351b.h>

static const struct footag_typeinfo info = {
        .type   = FOOTAG_TYPE_QFN,
        .name   = "QFN",
        .brief  = "Quad flat no-lead",
};

/* Silicon Labs Si5332A/B/C/D (JEDEC MO-220, Variation VKKD-4) */
static const struct footag_param temp[] = {
        PARAM_TOPIC("Body"),
        PARAM_T(BODY_L,  "Length",      "D",  4.90, 5.10),
        PARAM_T(BODY_W,  "Width",       "E",  4.90, 5.10),
        PARAM_I(BODY_N,  "Pins (1...)", "-",  8, 1, 1, 256),
        PARAM_I(BODY_Nx, "Pins (vert)", "-",  8, 1, 1, 256),

        PARAM_TOPIC("Lead"),
        PARAM_L(LEAD_P,  "Pitch",       "e",  0.50),
        PARAM_T(LEAD_L,  "Length",      "L",  0.30, 0.50),
        PARAM_T(LEAD_W,  "Width",       "b",  0.18, 0.30),
        PARAM_CALC_IPC7351B_HIRES,
        PARAM_PADSTACK_SMD_RECTS,
        PARAM_TERM,
};

static int calc(struct footag_ctx *ctx)
{
        struct footag_spec *const s = &ctx->spec;
        struct ipcb_twospec spec_row;
        struct ipcb_twospec spec_col;
        const struct ipcb_comp comp_row = {
                .l = GETID(ctx, BODY_W)->t,
                .t = GETID(ctx, LEAD_L)->t,
                .w = GETID(ctx, LEAD_W)->t,
        };
        const struct ipcb_comp comp_col = {
                .l = GETID(ctx, BODY_L)->t,
                .t = GETID(ctx, LEAD_L)->t,
                .w = GETID(ctx, LEAD_W)->t,
        };
        const struct ipcb_attr attr = {
                .density = footag_get_density(&GETID(ctx, CALC_D)->e),
                .f = GETID(ctx, CALC_F)->l,
                .p = GETID(ctx, CALC_P)->l,
        };
        const int rows  = intmax(1, GETID(ctx, BODY_N)->i.val);
        const int cols  = intmax(1, GETID(ctx, BODY_Nx)->i.val);
        const int npads = 2 * (rows + cols);
        int ret;

        ret = footag_realloc_pads(ctx, npads);
        if (ret) { return ret; }

        ret = ipcb_get_qfn(&spec_row, &comp_row, &attr);
        if (ret) { return ret; }
        ret = ipcb_get_qfn(&spec_col, &comp_col, &attr);
        if (ret) { return ret; }

        footag_genquad(
                s->pads,
                spec_row.land.dist,
                spec_row.land.w, spec_row.land.h,
                spec_col.land.dist,
                spec_col.land.w, spec_col.land.h,
                GETID(ctx, LEAD_P)->l,
                rows, cols,
                GETID(ctx, CALC_STACK)->e.val
        );
        s->body = footag_crlimit(
                GETID(ctx, BODY_W)->t.nom,
                GETID(ctx, BODY_L)->t.nom
        );
        footag_ipc7351b_setrrectpads(&ctx->spec);
        footag_snap(s, ROUNDOFF_TO_GRID[GETID(ctx, CALC_ROUND)->e.val]);
        footag_setcourtyard(s, spec_row.cyexc);
        footag_setref_ipc7351b(s, &spec_row.ref);

        return 0;
}

const struct footag_op footag_op_qfn = {
        .size   = sizeof (struct footag_ctx),
        .init   = footag_init_default,
        .fini   = footag_fini_default,
        .calc   = calc,
        .info   = &info,
        .temp   = &temp[0],
};

