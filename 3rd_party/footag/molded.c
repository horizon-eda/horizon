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

static const struct footag_typeinfo info_molded = {
        .type   = FOOTAG_TYPE_MOLDED,
        .name   = "MOLDED",
        .brief  = "Molded capacitors, inductors, diodes, etc.",
};

static const struct footag_typeinfo info_melf = {
        .type   = FOOTAG_TYPE_MELF,
        .name   = "MELF",
        .brief  = "Metal electrode lead face",
};

/* "AVX C" / "EIA 2312" from some datasheet. */
static const struct footag_param temp[] = {
        PARAM_TOPIC("Body"),
        PARAM_T(BODY_L, "Length", "L",  6.00, 0.30),
        PARAM_L(BODY_W, "Width",  "W",  3.20),
#if 0
        PARAM_T(BODY_H, "Height", "H",  2.50, 0.30),
#endif
        PARAM_TOPIC("Lead"),
        PARAM_T(LEAD_L, "Length", "A",  1.30, 0.30),
        PARAM_T(LEAD_W, "Width",  "W1", 2.20, 0.10),
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
                .l = GETID(ctx, BODY_L)->t,
                .t = GETID(ctx, LEAD_L)->t,
                .w = GETID(ctx, LEAD_W)->t,
        };
        struct ipcb_attr attr = {
                .density = footag_get_density(&GETID(ctx, CALC_D)->e),
                .f = GETID(ctx, CALC_F)->l,
                .p = GETID(ctx, CALC_P)->l,
        };
        int ret;

        if (ctx->op->info->type == FOOTAG_TYPE_MOLDED) {
                ret = ipcb_get_molded(&spec, &comp, &attr);
        } else if (ctx->op->info->type == FOOTAG_TYPE_MELF) {
                ret = ipcb_get_melf(&spec, &comp, &attr);
        } else {
                ret = 1;
        }
        if (ret) { return ret; }

        footag_gentwopin(
                s->pads,
                spec.land.dist,
                spec.land.w, spec.land.h,
                GETID(ctx, CALC_STACK)->e.val
        );
        s->body = footag_crlimit(
                GETID(ctx, BODY_L)->t.nom,
                GETID(ctx, BODY_W)->l
        );
        footag_ipc7351b_setrrectpads(&ctx->spec);
        footag_snap(s, ROUNDOFF_TO_GRID[GETID(ctx, CALC_ROUND)->e.val]);
        footag_setcourtyard(s, spec.cyexc);
        footag_setref_ipc7351b(s, &spec.ref);

        return 0;
}

const struct footag_op footag_op_molded = {
        .size   = sizeof (struct footag_ctx),
        .init   = footag_init_twopin,
        .fini   = footag_fini_default,
        .calc   = calc,
        .info   = &info_molded,
        .temp   = &temp[0],
};

const struct footag_op footag_op_melf = {
        .size   = sizeof (struct footag_ctx),
        .init   = footag_init_twopin,
        .fini   = footag_fini_default,
        .calc   = calc,
        .info   = &info_melf,
        .temp   = &temp[0],
};

