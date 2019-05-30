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

/* NOTE: "P" is for "pullback" and not "plastic" */
static const struct footag_typeinfo info = {
        .type   = FOOTAG_TYPE_PSON,
        .name   = "PSON",
        .brief  = "Small outline no-lead with pullback",
};

/* Microchip Technology Drawing C04-269A  */
static const struct footag_param temp[] = {
        PARAM_TOPIC("Body"),
        PARAM_L(BODY_L,  "Length",      "D",  2.00),
        PARAM_T(BODY_W,  "Width",       "E",  2.00, 0.10),
        PARAM_I(BODY_N,  "Pins",        "N",  8, 2, 2, 512),

        PARAM_TOPIC("Terminal"),
        PARAM_L(LEAD_P,  "Pitch",       "e",  0.50),
        PARAM_T(LEAD_L,  "Length",      "L",  0.20, 0.45),
        PARAM_T(LEAD_W,  "Width",       "b",  0.15, 0.25),
        PARAM_L(LEAD_PB, "Pullback",    "L1", 0.075),
        PARAM_CALC_IPC7351B_HIRES,
        PARAM_TOPIC("Generate"), \
        PARAM_E(CALC_ROUND, "Round-off", "-", 3, 4, \
                "None", "0.01 mm", "0.02 mm", "0.05 mm" \
        ),
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

        ret = ipcb_get_pson(&spec, &comp, &attr, GETID(ctx, LEAD_PB)->l);
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

const struct footag_op footag_op_pson = {
        .size   = sizeof (struct footag_ctx),
        .init   = footag_init_default,
        .fini   = footag_fini_default,
        .calc   = calc,
        .info   = &info,
        .temp   = &temp[0],
};

