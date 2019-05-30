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
        .type   = FOOTAG_TYPE_SO,
        .name   = "SO",
        .brief  = "Small outline package",
        .desc   = "SOIC, SO, SOM, SOL, SOP, VSOP, SSOP, QSOP, TSOP, etc.",
};

/* SOIC JEDEC MS-012 */
static const struct footag_param temp[] = {
        PARAM_TOPIC("Body"),
        PARAM_L(BODY_L,  "Length",     "D",  9.90),
        PARAM_L(BODY_W,  "Width",      "E1", 3.30),
        /*
         * A:  from PCB to top of body
         * A1: from PCB to bottom of body
         * A2: from bottom of body to top of body
         */
        PARAM_I(BODY_N,  "Pins",        "N", 16, 2, 2, 512),
        PARAM_TOPIC("Lead"),
        PARAM_L(LEAD_P,  "Pitch",      "e",  1.27),
        PARAM_T(LEAD_L,  "Length",     "L",  0.40, 1.27),
        PARAM_T(LEAD_W,  "Width",      "b",  0.31, 0.51),
        PARAM_T(LEAD_S,  "Span",       "E",  6.00, 0.20),
        PARAM_CALC_IPC7351B,
        PARAM_PADSTACK_SMD_RECTS,
        PARAM_TERM,
};

static int calc(struct footag_ctx *ctx)
{
        struct footag_spec *const s = &ctx->spec;
        struct ipcb_twospec spec;
        const struct ipcb_comp comp = {
                .l = GETID(ctx, LEAD_S)->t,
                .t = GETID(ctx, LEAD_L)->t,
                .w = GETID(ctx, LEAD_W)->t,
        };
        const struct ipcb_attr attr = {
                .density = footag_get_density(&GETID(ctx, CALC_D)->e),
                .f = GETID(ctx, CALC_F)->l,
                .p = GETID(ctx, CALC_P)->l,
        };
        const double pitch      = GETID(ctx, LEAD_P)->l;
        const int npads         = intmax(2, GETID(ctx, BODY_N)->i.val);
        int ret;

        ret = footag_realloc_pads(ctx, npads);
        if (ret) { return ret; }

        ret = ipcb_get_gullwing(&spec, &comp, &attr, pitch);
        if (ret) { return ret; }

        footag_gentworow(
                s->pads,
                spec.land.dist,
                spec.land.w, spec.land.h,
                pitch,
                npads,
                GETID(ctx, CALC_STACK)->e.val
        );
        s->body = footag_crlimit(
                GETID(ctx, BODY_W)->l,
                GETID(ctx, BODY_L)->l
        );
        footag_ipc7351b_setrrectpads(&ctx->spec);
        footag_snap(s, ROUNDOFF_TO_GRID[GETID(ctx, CALC_ROUND)->e.val]);
        footag_setcourtyard(s, spec.cyexc);
        footag_setref_ipc7351b(s, &spec.ref);

        return 0;
}

const struct footag_op footag_op_so = {
        .size   = sizeof (struct footag_ctx),
        .init   = footag_init_default,
        .fini   = footag_fini_default,
        .calc   = calc,
        .info   = &info,
        .temp   = &temp[0],
};

