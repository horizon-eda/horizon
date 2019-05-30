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
        .type   = FOOTAG_TYPE_SOT23,
        .name   = "SOT23",
        .brief  = "Small outline transistor (JEDEC TO-236AB)",
};

static const struct footag_param temp[] = {
        PARAM_TOPIC("Body"),
        PARAM_L(BODY_L,  "Length",     "D",  2.90),
        PARAM_L(BODY_W,  "Width",      "E1", 1.30),
        PARAM_E(BODY_N,  "Style",      "-",  0, 2, "3 pins", "5 pins",),
        PARAM_TOPIC("Lead"),
        PARAM_L(LEAD_P,  "Pitch",      "e",  0.95),
        PARAM_T(LEAD_L,  "Length",     "L",  0.13, 0.60),
        PARAM_T(LEAD_W,  "Width",      "b",  0.30, 0.54),
        PARAM_T(LEAD_S,  "Span",       "E",  2.10, 2.64),
        PARAM_CALC_IPC7351B,
        PARAM_PADSTACK_SMD_RECTS,
        PARAM_TERM,
};

static int calc(
        struct footag_ctx *ctx
)
{
        struct footag_spec *const s = &ctx->spec;
        struct ipcb_twospec spec;
        struct ipcb_comp comp = {
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
        double lpitch, rpitch;
        int npads, lpads;
        int ret;

        if (0 == GETID(ctx, BODY_N)->e.val) {
                npads  = 2 + 1;
                lpads  = 2;
                lpitch = 2 * pitch;
                rpitch = 1234;
        } else {
                npads  = 3 + 2;
                lpads  = 3;
                lpitch = pitch;
                rpitch = 2 * pitch;
        }

        ret = footag_realloc_pads(ctx, npads);
        if (ret) { return ret; }

        ret = ipcb_get_gullwing(&spec, &comp, &attr, lpitch);
        if (ret) { return ret; }

        footag_genlrow(
                s->pads,
                spec.land.dist,
                spec.land.w, spec.land.h,
                lpitch,
                lpads,
                GETID(ctx, CALC_STACK)->e.val
        );

        /* re-calculate because different pitch */
        ret = ipcb_get_gullwing(&spec, &comp, &attr, rpitch);
        if (ret) { return ret; }

        footag_genrrow(
                &s->pads[lpads],
                spec.land.dist,
                spec.land.w, spec.land.h,
                rpitch,
                npads - lpads,
                GETID(ctx, CALC_STACK)->e.val
        );
        footag_gennames(s->pads, npads, 1, 1);

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

const struct footag_op footag_op_sot23 = {
        .size   = sizeof (struct footag_ctx),
        .init   = footag_init_default,
        .fini   = footag_fini_default,
        .calc   = calc,
        .info   = &info,
        .temp   = &temp[0],
};

