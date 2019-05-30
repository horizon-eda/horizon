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

static const struct footag_typeinfo info_sod = {
        .type   = FOOTAG_TYPE_SOD,
        .name   = "SOD",
        .brief  = "Small outline diode",
        .desc   = "SOD123, SOD323, SOD523, SOD923, etc.",
};

/* SOD323 according to Diodes Incorporated */
static const struct footag_param temp_sod[] = {
        PARAM_TOPIC("Body"),
        PARAM_L(BODY_L,  "Length",     "D",  1.30),
        PARAM_L(BODY_W,  "Width",      "E1", 1.70),
        PARAM_TOPIC("Lead"),
        PARAM_T(LEAD_L,  "Length",     "L",  0.20, 0.40),
        PARAM_T(LEAD_W,  "Width",      "b",  0.25, 0.35),
        PARAM_T(LEAD_S,  "Span",       "E",  2.30, 2.70),
        PARAM_CALC_IPC7351B,
        PARAM_PADSTACK_SMD_RECTS,
        PARAM_TERM,
};

static const struct footag_typeinfo info_sodfl = {
        .type   = FOOTAG_TYPE_SODFL,
        .name   = "SODFL",
        .brief  = "Small outline diode, flat lead",
};

/* SOD323F according to Diodes Incorporated */
static const struct footag_param temp_sodfl[] = {
        PARAM_TOPIC("Body"),
        PARAM_L(BODY_L,  "Length",     "D",  1.25),
        PARAM_L(BODY_W,  "Width",      "E1", 1.70),
        PARAM_TOPIC("Lead"),
        PARAM_T(LEAD_L,  "Length",     "L",  0.41, 0.61),
        PARAM_T(LEAD_W,  "Width",      "b",  0.25, 0.35),
        PARAM_T(LEAD_S,  "Span",       "E",  2.30, 2.70),
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
        int ret;

        if (ctx->op->info->type == FOOTAG_TYPE_SODFL) {
                ret = ipcb_get_sodfl(&spec, &comp, &attr);
        } else {
                ret = ipcb_get_gullwing(&spec, &comp, &attr, 12345);
                if (ret) { return ret; }
        }

        footag_gentwopin(
                s->pads,
                spec.land.dist,
                spec.land.w, spec.land.h,
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

const struct footag_op footag_op_sod = {
        .size   = sizeof (struct footag_ctx),
        .init   = footag_init_twopin,
        .fini   = footag_fini_default,
        .calc   = calc,
        .info   = &info_sod,
        .temp   = &temp_sod[0],
};

const struct footag_op footag_op_sodfl = {
        .size   = sizeof (struct footag_ctx),
        .init   = footag_init_twopin,
        .fini   = footag_fini_default,
        .calc   = calc,
        .info   = &info_sodfl,
        .temp   = &temp_sodfl[0],
};

