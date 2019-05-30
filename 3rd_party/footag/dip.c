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
        .type   = FOOTAG_TYPE_DIP,
        .name   = "DIP",
        .brief  = "Two-row through-hole",
        .desc   = "DIP, pin headers, passives, etc.",
};

static const struct footag_param temp[] = {
        PARAM_TOPIC("Body"),
        PARAM_L(BODY_L,  "Length",      "-", 19.30),
        PARAM_L(BODY_W,  "Width",       "-",  6.60),
        PARAM_I(BODY_N,  "Pins",        "-", 14, 2, 2, 512),
        PARAM_E(BODY_Nx, "Numbering",   "-",  0, 2,
                "IC",
                "Header",
        ),
        PARAM_B(BODY_POL,"Polarized",   "-", true),

        PARAM_TOPIC("Lead"),
        PARAM_L(LEAD_P,  "Pitch",       "-",  2.54),
        PARAM_L(LEAD_D,  "Max diameter", "-",  0.60),
        PARAM_L(LEAD_S,  "Span",        "-",  7.62),
        PARAM_CALC_IPC7251DRAFT1,
        PARAM_TERM,
};

static int calc(
        struct footag_ctx *ctx
)
{
        struct footag_spec *const s = &ctx->spec;
        struct ipc7251_spec spec;
        const int density       = footag_get_density(&GETID(ctx, CALC_D)->e);
        const double leaddiam_max = GETID(ctx, LEAD_D)->l;
        const int npads         = intmax(2, GETID(ctx, BODY_N)->i.val);
        int ret;

        ret = footag_realloc_pads(ctx, npads);
        if (ret) { return ret; }

        ret = ipc7251_get_spec(&spec, leaddiam_max, density);
        if (ret) { return ret; }

        footag_gentworow(
                s->pads,
                GETID(ctx, LEAD_S)->l,
                spec.paddiam, spec.paddiam,
                GETID(ctx, LEAD_P)->l,
                npads,
                FOOTAG_PADSTACK_TH_ROUND
        );
        for (int i = 0; i < npads; i++) {
                struct footag_pad *p = &s->pads[i];
                p->diam = spec.paddiam;
                p->holediam = spec.holediam;
        }
        if (GETID(ctx, BODY_POL)->b) {
                s->pads[0].stack = FOOTAG_PADSTACK_TH_ROUND_RPAD;
        }
        if (GETID(ctx, BODY_Nx)->e.val == 1) {
                /* we think we know the topological order */
                footag_gennames(&s->pads[      0], npads/2, 1, 2);
                footag_gennames(&s->pads[npads/2], npads/2, npads, -2);
        }
        s->body = footag_crlimit(
                GETID(ctx, BODY_W)->l,
                GETID(ctx, BODY_L)->l
        );
        footag_snap(s, ROUNDOFF_TO_GRID[GETID(ctx, CALC_ROUND)->e.val]);
        footag_setcourtyard(s, spec.cyexc);
        footag_setref_ipc7251draft1(s, &spec.ref);

        return 0;
}

const struct footag_op footag_op_dip = {
        .size   = sizeof (struct footag_ctx),
        .init   = footag_init_default,
        .fini   = footag_fini_default,
        .hint   = footag_hint_ipc7251draft1,
        .calc   = calc,
        .info   = &info,
        .temp   = &temp[0],
};

