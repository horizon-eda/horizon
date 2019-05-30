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
 * SOT-223
 *
 * There seems to be some naming confusion on this one.
 * - "SOT-223" vs "SOT223"
 * - whether the prefix -x means number of leads incl. or excl. tab.
 *
 * SOT223-3 seems to be specified by
 * - "JEDEC TO-261AA"
 * - "JEITA SC-73"
 */

#include "priv.h"
#include <footag/ipc7351b.h>

static const struct footag_typeinfo info = {
        .type   = FOOTAG_TYPE_SOT223,
        .name   = "SOT223",
        .brief  = "Small outline transistor (JEDEC TO-261AA)",
};

static const struct footag_param temp[] = {
        PARAM_TOPIC("Body"),
        PARAM_L(BODY_L,  "Length",     "D",  6.50),
        PARAM_L(BODY_W,  "Width",      "E1", 3.50),
#if 0
        PARAM_T(BODY_H,  "Height",     "A2", 1.50, 1.70),
        PARAM_T(BODY_S,  "Standoff",   "A1", 0.02, 0.10),
#endif
        PARAM_E(BODY_N,  "Style",      "-",  0, 3,
                "3 pins, 1 tab",
                "4 pins, 1 tab",
                "5 pins, 1 tab",
        ),

        PARAM_TOPIC("Lead"),
        PARAM_L(LEAD_P,  "Pitch",      "e",  2.30),
        PARAM_T(LEAD_L,  "Length",     "L",  0.62, 1.02),
        PARAM_T(LEAD_W,  "Width",      "b",  0.65, 0.85),
        PARAM_T(LEAD_W1, "Tab width",  "bx", 2.95, 3.15),
        PARAM_T(LEAD_S,  "Span",       "E",  6.70, 7.30),
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
        const int npads         = 4 + GETID(ctx, BODY_N)->e.val;
        int ret;

        ret = footag_realloc_pads(ctx, npads);
        if (ret) { return ret; }

        ret = ipcb_get_gullwing(&spec, &comp, &attr, pitch);
        if (ret) { return ret; }

        footag_genlrow(
                s->pads,
                spec.land.dist,
                spec.land.w, spec.land.h,
                pitch,
                npads-1,
                GETID(ctx, CALC_STACK)->e.val
        );

        /* re-calculate land for the tab */
        comp.w = GETID(ctx, LEAD_W1)->t;
        ret = ipcb_get_gullwing(&spec, &comp, &attr, 1234);
        if (ret) { return ret; }

        s->pads[npads-1].stack = s->pads[0].stack;
        s->pads[npads-1].y = 0;
        s->pads[npads-1].x = spec.land.dist / 2.0;
        s->pads[npads-1].w = spec.land.w;
        s->pads[npads-1].h = spec.land.h;
        s->pads[npads-1].angle = FOOTAG_ANGLE_90;
        footag_gennames(&s->pads[npads-1], 1, npads, 1);

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

const struct footag_op footag_op_sot223 = {
        .size   = sizeof (struct footag_ctx),
        .init   = footag_init_default,
        .fini   = footag_fini_default,
        .calc   = calc,
        .info   = &info,
        .temp   = &temp[0],
};

