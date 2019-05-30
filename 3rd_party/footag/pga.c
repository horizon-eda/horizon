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
#include <footag/ipc7251draft1.h>

static const struct footag_typeinfo info = {
        .type   = FOOTAG_TYPE_PGA,
        .name   = "PGA",
        .brief  = "Pin grid array (IPC-7251 1st Working Draft)",
};

#define inch (25.4)
static const struct footag_param temp[] = {
        PARAM_TOPIC("Body"),
        PARAM_L(BODY_L,  "Length",              "-",  1.86 * inch),
        PARAM_L(BODY_W,  "Width",               "-",  1.86 * inch),
        PARAM_I(BODY_R,  "Rows",                "-", 18, 1, 2, 64),
        PARAM_I(BODY_C,  "Columns",             "-", 18, 1, 2, 64),
        PARAM_I(BODY_PR, "Perimeter rows",      "-",  4, 1, 0, 64),
        PARAM_I(BODY_PC, "Perimeter columns",   "-",  4, 1, 0, 64),

        PARAM_TOPIC("Lead"),
        PARAM_L(LEAD_P,  "Pitch",               "-",  0.10 * inch),
        PARAM_L(LEAD_D,  "Max diameter",        "-",  0.02 * inch),
#if 0
        PARAM_B(NAME_Q,  "Q",                   "-",  false),
#endif
        PARAM_M(NAME_ROW, "Skip row",       "-",  0x00000003, 6, \
                "I", "O", "Q", "S", "X", "Z" \
        ), \
        PARAM_CALC_IPC7251DRAFT1,
        PARAM_TERM,
};
#undef inch

static int calc(
        struct footag_ctx *ctx
)
{
        struct footag_spec *const s = &ctx->spec;
        struct ipc7251_spec spec;
        const int density       = footag_get_density(&GETID(ctx, CALC_D)->e);
        const double leaddiam_max = GETID(ctx, LEAD_D)->l;
        const int rows          = intmax(2, GETID(ctx, BODY_R)->i.val);
        const int cols          = intmax(2, GETID(ctx, BODY_C)->i.val);
        const int npads         = rows * cols;
        int ret;

        ret = footag_realloc_pads(ctx, npads);
        if (ret) { return ret; }

        ret = ipc7251_get_spec(&spec, leaddiam_max, density);
        if (ret) { return ret; }

        footag_gengrid(
                s->pads,
                spec.paddiam, spec.paddiam,
                GETID(ctx, LEAD_P)->l,
                rows, cols,
                GETID(ctx, BODY_PR)->i.val,
                GETID(ctx, BODY_PC)->i.val,
                FOOTAG_PADSTACK_TH_ROUND
        );
        for (int i = 0; i < npads; i++) {
                struct footag_pad *p = &s->pads[i];
                p->diam = spec.paddiam;
                p->holediam = spec.holediam;
        }
        footag_gridnames(
                ctx->spec.pads,
                &GETID(ctx, NAME_ROW)->m,
                rows,
                cols
        );
        s->pads[0].stack = FOOTAG_PADSTACK_TH_ROUND_RPAD;
        s->body = footag_crlimit(
                GETID(ctx, BODY_W)->l,
                GETID(ctx, BODY_L)->l
        );
        footag_snap(s, ROUNDOFF_TO_GRID[GETID(ctx, CALC_ROUND)->e.val]);
        footag_setcourtyard(s, spec.cyexc);
        footag_setref_ipc7251draft1(s, &spec.ref);

        return 0;
}

const struct footag_op footag_op_pga = {
        .size   = sizeof (struct footag_ctx),
        .init   = footag_init_default,
        .fini   = footag_fini_default,
        .hint   = footag_hint_ipc7251draft1,
        .calc   = calc,
        .info   = &info,
        .temp   = &temp[0],
};

