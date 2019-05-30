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

static const struct footag_typeinfo info_chip = {
        .type   = FOOTAG_TYPE_CHIP,
        .name   = "CHIP",
        .brief  = "Chip resistors, capacitors, inductors, etc.",
};

static const struct footag_param temp_chip[] = {
        PARAM_TOPIC("Body"),
        PARAM_T(BODY_L, "Length", "D",  1.0, 0.1),
        PARAM_T(BODY_W, "Width",  "E",  0.5, 0.1),
        PARAM_TOPIC("Lead"),
        PARAM_T(LEAD_L, "Length", "L",  0.2, 0.1),
        PARAM_CALC_IPC7351B,
        PARAM_PADSTACK_SMD_RECTS,
        PARAM_TERM,
};

static const struct footag_typeinfo info_ledsc = {
        .type   = FOOTAG_TYPE_LEDSC,
        .name   = "LEDSC",
        .brief  = "Side concave LEDs, diodes, oscillators etc.",
};

/* Avago 0805 LEDSC */
static const struct footag_param temp_ledsc[] = {
        PARAM_TOPIC("Body"),
        PARAM_T(BODY_L, "Length", "D",  2.00, 0.10),
        PARAM_T(BODY_W, "Width",  "E",  1.25, 0.10),
        PARAM_TOPIC("Lead"),
        PARAM_T(LEAD_L, "Length", "L",  0.4, 0.15),
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
                .w = GETID(ctx, BODY_W)->t,
        };
        struct ipcb_attr attr = {
                .density = footag_get_density(&GETID(ctx, CALC_D)->e),
                .f = GETID(ctx, CALC_F)->l,
                .p = GETID(ctx, CALC_P)->l,
        };
        int ret;

        if (ctx->op->info->type == FOOTAG_TYPE_CHIP) {
                ret = ipcb_get_chip(&spec, &comp, &attr);
                if (ret) { return ret; }
        } else {
                ret = ipcb_get_sideconcave(&spec, &comp, &attr);
                if (ret) { return ret; }
        }

        footag_gentwopin(
                s->pads,
                spec.land.dist,
                spec.land.w, spec.land.h,
                GETID(ctx, CALC_STACK)->e.val
        );
        s->body = footag_crlimit(
                GETID(ctx, BODY_L)->t.nom,
                GETID(ctx, BODY_W)->t.nom
        );
        footag_ipc7351b_setrrectpads(&ctx->spec);
        footag_snap(s, ROUNDOFF_TO_GRID[GETID(ctx, CALC_ROUND)->e.val]);
        footag_setcourtyard(s, spec.cyexc);
        footag_setref_ipc7351b(s, &spec.ref);

        return 0;
}

const struct footag_op footag_op_chip = {
        .size   = sizeof (struct footag_ctx),
        .init   = footag_init_twopin,
        .fini   = footag_fini_default,
        .calc   = calc,
        .info   = &info_chip,
        .temp   = &temp_chip[0],
};

const struct footag_op footag_op_ledsc = {
        .size   = sizeof (struct footag_ctx),
        .init   = footag_init_twopin,
        .fini   = footag_fini_default,
        .calc   = calc,
        .info   = &info_ledsc,
        .temp   = &temp_ledsc[0],
};

