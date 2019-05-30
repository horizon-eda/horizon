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
        .type   = FOOTAG_TYPE_SOJ,
        .name   = "SOJ",
        .brief  = "Small Outline IC, J-leaded",
};

/* ISSI IS63LV1024 */
#define inch (25.4)
static const struct footag_param temp[] = {
        PARAM_TOPIC("Body"),
        PARAM_L(BODY_L,  "Length",     "D",  0.825 * inch),
        PARAM_L(BODY_W,  "Width",      "E1", 0.3 * 25.4),
        PARAM_I(BODY_N,  "Pins",       "N",  32, 2, 2, 512),

        PARAM_TOPIC("Lead"),
        PARAM_L(LEAD_P,  "Pitch",      "e",  0.05 * inch),
        PARAM_T(LEAD_L,  "Length",     "L", (0.325 - 0.287) * inch, (0.345 - 0.247) * inch),
        PARAM_T(LEAD_W,  "Width",      "b",  0.016 * inch, 0.020 * inch),
        PARAM_T(LEAD_S,  "Span",       "E",  0.325 * inch, 0.345 * inch),
        PARAM_CALC_IPC7351B,
        PARAM_PADSTACK_SMD_RECTS,
        PARAM_TERM,
};
#undef inch

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
        const int npads         = intmax(2, GETID(ctx, BODY_N)->i.val);
        int ret;

        ret = footag_realloc_pads(ctx, npads);
        if (ret) { return ret; }

        ret = ipcb_get_jlead(&spec, &comp, &attr);
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
                GETID(ctx, BODY_W)->l,
                GETID(ctx, BODY_L)->l
        );
        footag_ipc7351b_setrrectpads(&ctx->spec);
        footag_snap(s, ROUNDOFF_TO_GRID[GETID(ctx, CALC_ROUND)->e.val]);
        footag_setcourtyard(s, spec.cyexc);
        footag_setref_ipc7351b(s, &spec.ref);

        return 0;
}

const struct footag_op footag_op_soj = {
        .size   = sizeof (struct footag_ctx),
        .init   = footag_init_default,
        .fini   = footag_fini_default,
        .calc   = calc,
        .info   = &info,
        .temp   = &temp[0],
};

