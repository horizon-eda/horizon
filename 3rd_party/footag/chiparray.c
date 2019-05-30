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
        .type   = FOOTAG_TYPE_CHIPARRAY,
        .name   = "CHIPARRAY",
        .brief  = "Chip array components",
        .desc   = "Resistor, capacitor and inductor arrays etc., Side concave oscillator",
};

/* BOURNS CAT16-J4 chip resistor array with concave terminations */
static const struct footag_param temp[] = {
        PARAM_TOPIC("Body"),
        PARAM_L(BODY_L,  "Length",    "D",  3.20),
        PARAM_T(BODY_W,  "Width",     "E",  1.55, 0.25),
        PARAM_I(BODY_N,  "Pins",      "N",  8, 2, 2, 512),
        PARAM_E(BODY_Nx, "Terminals", "-",  0, 3, "Concave", "Convex", "Flat"),

        PARAM_TOPIC("Lead"),
        PARAM_L(LEAD_P,  "Pitch",     "e",  0.80),
        PARAM_T(LEAD_L,  "Length",    "L",  0.30, 0.20),
        PARAM_T(LEAD_W,  "Width",     "b",  0.40, 0.15),
        PARAM_CALC_IPC7351B,
        PARAM_PADSTACK_SMD_RECTS,
        PARAM_TERM,
};

static int calc(struct footag_ctx *ctx)
{
        struct footag_spec *const s = &ctx->spec;
        struct ipcb_twospec spec;
        const struct ipcb_comp comp = {
                .l = GETID(ctx, BODY_W)->t,
                .t = GETID(ctx, LEAD_L)->t,
                .w = GETID(ctx, LEAD_W)->t,
        };
        const struct ipcb_attr attr = {
                .density = footag_get_density(&GETID(ctx, CALC_D)->e),
                .f = GETID(ctx, CALC_F)->l,
                .p = GETID(ctx, CALC_P)->l,
        };
        const int npads = intmax(2, GETID(ctx, BODY_N)->i.val);
        int ret;

        ret = footag_realloc_pads(ctx, npads);
        if (ret) { return ret; }

        {
                const int style = GETID(ctx, BODY_Nx)->e.val;
                if (style == 0) {
                        ret = ipcb_get_concavearray(&spec, &comp, &attr);
                } else if (style == 1) {
                        ret = ipcb_get_convexarray(&spec, &comp, &attr);
                } else {
                        ret = ipcb_get_flatarray(&spec, &comp, &attr);
                }
        }
        if (ret) { return ret; }

        footag_gentworow(
                s->pads,
                spec.land.dist,
                spec.land.w, spec.land.h,
                GETID(ctx, LEAD_P)->l,
                npads,
                GETID(ctx, CALC_STACK)->e.val
        );
        s->body = footag_crlimit(
                GETID(ctx, BODY_W)->t.nom,
                GETID(ctx, BODY_L)->l
        );
        footag_ipc7351b_setrrectpads(&ctx->spec);
        footag_snap(s, ROUNDOFF_TO_GRID[GETID(ctx, CALC_ROUND)->e.val]);
        footag_setcourtyard(s, spec.cyexc);
        footag_setref_ipc7351b(s, &spec.ref);

        return 0;
}

const struct footag_op footag_op_chiparray = {
        .size   = sizeof (struct footag_ctx),
        .init   = footag_init_default,
        .fini   = footag_fini_default,
        .calc   = calc,
        .info   = &info,
        .temp   = &temp[0],
};

