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
        .type   = FOOTAG_TYPE_CAPAE,
        .name   = "CAPAE",
        .brief  = "Aluminium Electrolytic Capacitor and 2-pin Crystal",
};

/* A Panasonic aluminium electrolytic capacitor */
static const struct footag_param temp[] = {
        PARAM_TOPIC("Body"),
        PARAM_L(BODY_L,  "Length",      "-", 10.30),
        PARAM_L(BODY_W,  "Width",       "-", 10.30),
        PARAM_L(BODY_H,  "Height",      "A", 10.20),
        PARAM_TOPIC("Lead"),
        PARAM_T(LEAD_L,  "Length",      "-",  3.50, 0.20+0.20),
        PARAM_T(LEAD_W,  "Width",       "-",  0.90, 0.20),
        PARAM_T(LEAD_S,  "Span",        "-", 10.30+0.70, 0.20+0.20),
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
        const struct ipcb_comp comp = {
                .l = GETID(ctx, LEAD_S)->t,
                .t = GETID(ctx, LEAD_L)->t,
                .w = GETID(ctx, LEAD_W)->t,
        };
        struct ipcb_attr attr = {
                .density = footag_get_density(&GETID(ctx, CALC_D)->e),
                .f = GETID(ctx, CALC_F)->l,
                .p = GETID(ctx, CALC_P)->l,
        };
        int ret;

        ret = ipcb_get_capae(&spec, &comp, &attr, GETID(ctx, BODY_H)->l);
        if (ret) { return ret; }

        footag_gentwopin(
                s->pads,
                spec.land.dist,
                spec.land.w, spec.land.h,
                GETID(ctx, CALC_STACK)->e.val
        );
        s->body = footag_crlimit(
                GETID(ctx, BODY_L)->l,
                GETID(ctx, BODY_W)->l
        );
        footag_ipc7351b_setrrectpads(&ctx->spec);
        footag_snap(s, ROUNDOFF_TO_GRID[GETID(ctx, CALC_ROUND)->e.val]);
        footag_setcourtyard(s, spec.cyexc);
        footag_setref_ipc7351b(s, &spec.ref);

        return 0;
}

const struct footag_op footag_op_capae = {
        .size   = sizeof (struct footag_ctx),
        .init   = footag_init_twopin,
        .fini   = footag_fini_default,
        .calc   = calc,
        .info   = &info,
        .temp   = &temp[0],
};

