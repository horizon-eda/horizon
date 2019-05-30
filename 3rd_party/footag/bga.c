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
        .type   = FOOTAG_TYPE_BGA,
        .name   = "BGA",
        .brief  = "Ball grid array",
};

static const struct footag_param temp[] = {
        PARAM_TOPIC("Body"),
        PARAM_L(BODY_L,  "Length",              "-", 23.00),
        PARAM_L(BODY_W,  "Width",               "-", 23.00),
        PARAM_I(BODY_R,  "Rows",                "-", 22, 1, 2, 64),
        PARAM_I(BODY_C,  "Columns",             "-", 22, 1, 2, 64),
        PARAM_I(BODY_PR, "Perimeter rows",      "-", 11, 1, 0, 64),
        PARAM_I(BODY_PC, "Perimeter columns",   "-", 11, 1, 0, 64),

        PARAM_TOPIC("Terminal"),
        PARAM_L(LEAD_P,  "Pitch",               "-",  1.00),
        PARAM_L(LEAD_D,  "Ball diameter",       "-",  0.60),
        PARAM_E(CUSTOM,  "Solder ball", "-", 1, 2, \
                "Non-collapsing", "Collapsing", \
        ),
        PARAM_M(NAME_ROW, "Skip row",           "-",  0x0000003f, 6, \
                "I", "O", "Q", "S", "X", "Z" \
        ), \
        PARAM_TOPIC("Generate"), \
        PARAM_E(CALC_ROUND, "Round-off", "-", 3, 4, \
                "None", "0.01 mm", "0.02 mm", "0.05 mm" \
        ),
        PARAM_E(CALC_STACK, "Padstack", "-", 2, 3, \
                "Rectangular", "Rounded rectangular", "Circular" \
        ),
        PARAM_TERM,
};

static int calc(
        struct footag_ctx *ctx
)
{
        struct footag_spec *const s = &ctx->spec;
        struct ipcb_bgaspec spec;
        double diam = GETID(ctx, LEAD_D)->l;
        const int rows          = intmax(2, GETID(ctx, BODY_R)->i.val);
        const int cols          = intmax(2, GETID(ctx, BODY_C)->i.val);
        const int npads         = rows * cols;
        int ret;

        ret = footag_realloc_pads(ctx, npads);
        if (ret) { return ret; }

        ret = ipcb_get_bga(&spec, diam, GETID(ctx, CUSTOM)->e.val);
        if (ret) { return ret; }

        footag_gengrid(
                s->pads,
                spec.diam, spec.diam,
                GETID(ctx, LEAD_P)->l,
                rows, cols,
                GETID(ctx, BODY_PR)->i.val,
                GETID(ctx, BODY_PC)->i.val,
                GETID(ctx, CALC_STACK)->e.val
        );
        for (int i = 0; i < npads; i++) {
                struct footag_pad *p = &s->pads[i];
                p->diam = spec.diam;
        }
        footag_gridnames(
                ctx->spec.pads,
                &GETID(ctx, NAME_ROW)->m,
                rows,
                cols
        );
        s->body = footag_crlimit(
                GETID(ctx, BODY_W)->l,
                GETID(ctx, BODY_L)->l
        );
        footag_ipc7351b_setrrectpads(&ctx->spec);
        footag_snap(s, ROUNDOFF_TO_GRID[GETID(ctx, CALC_ROUND)->e.val]);
        /* NOTE: this is probably wrong */
        footag_setcourtyard(s, 1.0);
        footag_setref_ipc7351b(s, &spec.ref);

        return 0;
}

const struct footag_op footag_op_bga = {
        .size   = sizeof (struct footag_ctx),
        .init   = footag_init_default,
        .fini   = footag_fini_default,
        .calc   = calc,
        .info   = &info,
        .temp   = &temp[0],
};

