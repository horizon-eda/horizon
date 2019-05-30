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
        .type   = FOOTAG_TYPE_SIP,
        .name   = "SIP",
        .brief  = "One-row through-hole",
        .desc   = "SIP, pin headers, passives, etc.",
};

#define inch (25.4)
static const struct footag_param temp[] = {
        PARAM_TOPIC("Body"),
        PARAM_L(BODY_L,  "Length",      "-", 6 * 0.1 * inch),
        PARAM_L(BODY_W,  "Width",       "-", 0.1 * inch),
        PARAM_I(BODY_N,  "Pins",        "-", 6, 1, 1, 256),
        PARAM_B(BODY_POL,"Polarized",   "-", true),

        PARAM_TOPIC("Lead"),
        PARAM_L(LEAD_P,  "Pitch",       "-",  0.1 * inch),
        PARAM_L(LEAD_D,  "Max diameter", "-",  0.70),
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
        const int npads         = intmax(1, GETID(ctx, BODY_N)->i.val);
        int ret;

        ret = footag_realloc_pads(ctx, npads);
        if (ret) { return ret; }

        ret = ipc7251_get_spec(&spec, leaddiam_max, density);
        if (ret) { return ret; }

        footag_genlrow(
                s->pads,
                0.0,
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
        s->body = footag_crlimit(
                GETID(ctx, BODY_W)->l,
                GETID(ctx, BODY_L)->l
        );
        footag_snap(s, ROUNDOFF_TO_GRID[GETID(ctx, CALC_ROUND)->e.val]);
        footag_setcourtyard(s, spec.cyexc);
        footag_setref_ipc7251draft1(s, &spec.ref);

        return 0;
}

const struct footag_op footag_op_sip = {
        .size   = sizeof (struct footag_ctx),
        .init   = footag_init_default,
        .fini   = footag_fini_default,
        .hint   = footag_hint_ipc7251draft1,
        .calc   = calc,
        .info   = &info,
        .temp   = &temp[0],
};

