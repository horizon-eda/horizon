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

#define PLUSMINUS "\u00B1"

static const char *const hints[] = {
[PARAM_BODY_L]  = "<big>Component body length</big>",
[PARAM_BODY_W]  = "<big>Component body width</big>",
[PARAM_BODY_H]  =
"<big>Component body height</big>\n"
"A:  from PCB to top of body\n"
"A1: from PCB to bottom of body (component seating plane stand-off)\n"
"A2: from bottom of body to top of body"
,
[PARAM_LEAD_L]  = "<big>Lead length</big>",
[PARAM_LEAD_W]  = "<big>Lead width</big>",
[PARAM_LEAD_W1] = "<big>Lead width, alt.</big>",
[PARAM_LEAD_S]  = "<big>Lead span</big>\n"
"Distance measured from lead termination to lead termination.",
[PARAM_LEAD_PB]  = "<big>Lead pullback</big>\n"
"Distance from edge of body to edge of lead terminal.",
[PARAM_LEAD_P]  = "<big>Lead pitch</big>\n",
[PARAM_CALC_D]  =
"<big>Land protrusion</big>\n"
"<i>Density Level A: Maximum (Most) Land Protrusion</i>\n"
"<i>Density Level B: Median (Nominal) Land Protrusion</i>\n"
"<i>Density Level C: Minimum (Least) Land Protrusion</i>\n"
,
[PARAM_CALC_F] =
"<big>PCB fabrication tolerance</big>\n"
"It is determined by the PCB manufacturing process and "
"expressed as a min-max span and not a " PLUSMINUS "value: "
"<i>The difference between the MMC and the LMC of each land pattern dimension.</i>\n"
"NOTE: The parameter is independent of component dimensions."
,
[PARAM_CALC_P] =
"<big>Part placement tolerance</big>\n"
"It is determined by the variation board assembly process and "
"expressed as <i>diameter of true position</i>.\n"
"<i>This variation represents the location of the component in relation to its true position as defined by the design.</i>\n"
"NOTE: The parameter is independent of component dimensions."
,
[PARAM_CALC_STACK] =
"<big>Padstack</big>\n"
"<b>Rectangular</b>\n"
"Defined by width and height\n"
"\n"
"<b>Rounded rectangular</b>\n"
"Corner radius is 20 " "%" " of shortest pad dimension, and maximum 0.25 mm"
,
[PARAM_CALC_ROUND] =
"<big>IPC-7351B round-off factor</big>\n"
"All features of the land pattern is rounded to this value, "
"including pad sizes and placement.\n"
"\n"
"IPC-7351B specifies 0.05 mm for all lead/component types, "
"except chip components smaller than 1608 (0603)."
,
};

const char *footag_hint(
        struct footag_ctx *ctx,
        const struct footag_param *p
)
{
        const char *h = NULL;

        if (ctx->op->hint) {
                h = ctx->op->hint(ctx, p);
                if (h) {
                        return h;
                }
        }
        if (p->id < (int) NELEM(hints)) {
                h = hints[p->id];
                if (h) {
                        return h;
                }
        }
        return p->name;
}

const char *const hints_ipc7251draft1[] = {
[PARAM_CALC_ROUND] =
"<big>Round-off factor</big>\n"
"All features of the land pattern is rounded to this value, "
"including pad sizes and placement.\n"
"\n"
"<b>IPC-7251B, 1<sup>st</sup> Working Draft</b> does not specify this."
,
[PARAM_CALC_D]  =
"<big>IPC-2221A Producibility Level</big>\n"
"<i>Level A: General Design Producibility - Preferred</i>\n"
"<i>Level B: Moderate Design Producibility - Standard</i>\n"
"<i>Level C: High Design Producibility - Reduced</i>\n"
,
[PARAM_LEAD_D]  = "<big>Maximum lead diameter</big>\n"
"• Round leads: <b>diameter</b>.\n"
"• Rectangular leads: <b>diagonal</b>."
};

const char *footag_hint_ipc7251draft1(
        struct footag_ctx *ctx,
        const struct footag_param *p
)
{
        UNUSED(ctx);
        const char *h = NULL;
        if (p->id < (int) NELEM(hints_ipc7251draft1)) {
                h = hints_ipc7251draft1[p->id];
        }
        return h;
}

