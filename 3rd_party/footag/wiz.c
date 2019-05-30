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

#include <stdio.h>
#include "priv.h"

extern const struct footag_op footag_op_chip;
extern const struct footag_op footag_op_chiparray;
extern const struct footag_op footag_op_ledsc;
extern const struct footag_op footag_op_melf;
extern const struct footag_op footag_op_molded;
extern const struct footag_op footag_op_capae;
extern const struct footag_op footag_op_so;
extern const struct footag_op footag_op_sod;
extern const struct footag_op footag_op_sodfl;
extern const struct footag_op footag_op_soj;
extern const struct footag_op footag_op_qfp;
extern const struct footag_op footag_op_son;
extern const struct footag_op footag_op_qfn;
extern const struct footag_op footag_op_pson;
extern const struct footag_op footag_op_pqfn;
extern const struct footag_op footag_op_bga;
extern const struct footag_op footag_op_sot223;
extern const struct footag_op footag_op_sot23;
extern const struct footag_op footag_op_dip;
extern const struct footag_op footag_op_sip;
extern const struct footag_op footag_op_pga;

static const struct footag_op *theop[FOOTAG_TYPE_NUM] = {
        [FOOTAG_TYPE_CHIP]      = &footag_op_chip,
        [FOOTAG_TYPE_CHIPARRAY] = &footag_op_chiparray,
        [FOOTAG_TYPE_LEDSC]     = &footag_op_ledsc,
        [FOOTAG_TYPE_MELF]      = &footag_op_melf,
        [FOOTAG_TYPE_MOLDED]    = &footag_op_molded,
        [FOOTAG_TYPE_CAPAE]     = &footag_op_capae,
        [FOOTAG_TYPE_SO]        = &footag_op_so,
        [FOOTAG_TYPE_SOD]       = &footag_op_sod,
        [FOOTAG_TYPE_SODFL]     = &footag_op_sodfl,
        [FOOTAG_TYPE_SOJ]       = &footag_op_soj,
        [FOOTAG_TYPE_QFP]       = &footag_op_qfp,
        [FOOTAG_TYPE_SON]       = &footag_op_son,
        [FOOTAG_TYPE_QFN]       = &footag_op_qfn,
        [FOOTAG_TYPE_PSON]      = &footag_op_pson,
        [FOOTAG_TYPE_PQFN]      = &footag_op_pqfn,
        [FOOTAG_TYPE_BGA]       = &footag_op_bga,
        [FOOTAG_TYPE_SOT223]    = &footag_op_sot223,
        [FOOTAG_TYPE_SOT23]     = &footag_op_sot23,
        [FOOTAG_TYPE_DIP]       = &footag_op_dip,
        [FOOTAG_TYPE_SIP]       = &footag_op_sip,
        [FOOTAG_TYPE_PGA]       = &footag_op_pga,
};

/* here we do some parameter checking to not break the application */
const struct footag_typeinfo *footag_get_typeinfo(
        enum footag_type footype
)
{
        if (footype < 0 || FOOTAG_TYPE_NUM <= footype) {
                return NULL;
        }
        const struct footag_op *op = theop[footype];
        if (!op) {
                return NULL;
        }
        return op->info;
}

struct footag_ctx *footag_open(int footype)
{
        const struct footag_op *op = theop[footype];
        struct footag_ctx *ctx;
        int ret;

        if (!op) {
                return NULL;
        }
        ctx = calloc(1, op->size);
        if (!ctx) {
                return NULL;
        }

        ctx->op = op;
        ret = op->init(ctx);
        if (ret) {
                free(ctx);
                return NULL;
        }

        return ctx;
}

int footag_close(struct footag_ctx *ctx)
{
        ctx->op->fini(ctx);
        free(ctx);
        return 0;
}

struct footag_param *footag_get_param(
        struct footag_ctx *ctx
)
{
        return ctx->param;
}

const struct footag_spec *footag_get_spec(
        struct footag_ctx *ctx
)
{
        int ret;
        ret = ctx->op->calc(ctx);
        if (ret) { return NULL; }
        return &ctx->spec;
}

int footag_init_from_template(
        struct footag_ctx *ctx,
        const struct footag_param *temp
)
{
        /* count number of parameters, including DONE tag */
        size_t nelem = 1;
        for (const struct footag_param *p = temp; p->id != PARAM_DONE; p++) {
                nelem++;
        }

        struct footag_param *param;

        param = calloc(nelem, sizeof (*param));
        if (!param) { return __LINE__; }

        memcpy(param, temp, nelem * sizeof (*param));
        ctx->param = param;

        return 0;
}

int footag_init_default(struct footag_ctx *ctx)
{
        int ret;
        ctx->spec.pads = NULL;
        ret = footag_init_from_template(ctx, ctx->op->temp);
        return ret;
}

int footag_init_twopin(struct footag_ctx *ctx)
{
        int ret;
        ctx->spec.pads = NULL;
        ret = footag_realloc_pads(ctx, 2);
        if (ret) { return ret; }
        ret = footag_init_from_template(ctx, ctx->op->temp);
        return ret;
}

int footag_fini_default(struct footag_ctx *ctx)
{
        free(ctx->spec.pads);
        free(ctx->param);
        return 0;
}

const union footag_data *footag_data_by_name(
        struct footag_ctx *ctx,
        const char *topic,
        const char *name
)
{
        const struct footag_param *p = ctx->param;

        while (p->id != PARAM_DONE) {
                if (p->id == PARAM_TOPIC) {
                        if (0 == strcmp(topic, p->name)) {
                                break;
                        }
                }
                p++;
        }

        while (p->id != PARAM_DONE) {
                if (p->id != PARAM_TOPIC) {
                        if (0 == strcmp(name, p->name)) {
                                return &p->item.data;
                        }
                }
                p++;
        }

        return NULL;
}

const union footag_data *footag_data_by_id(
        struct footag_ctx *ctx,
        int id
)
{
        const struct footag_param *p = ctx->param;

        while (p->id != PARAM_DONE) {
                if (p->id == id) {
                        return &p->item.data;
                }
                p++;
        }

        return NULL;
}

static const char *const GRID_ROWNAMES[] = {
        "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M",
        "N", "O", "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z",
        "AA", "AB", "AC", "AD", "AE", "AF", "AG", "AH", "AI", "AJ", "AK",
        "AL", "AM", "AN", "AO", "AP", "AQ", "AR", "AS", "AT", "AU", NULL
};

/* assumes strlen(rowname) */
static bool skiprow(
        const char *const rowname,
        const struct footag_bitmask *const skipmask
)
{
        if (!skipmask) { return false; }
        if (!rowname) { return false; }
        for (unsigned int i = 0; i < skipmask->num; i++) {
                if (!(skipmask->val & (1 << i))) {
                        continue;
                }
                const char skipchar = skipmask->strs[i][0];
                if (strchr(rowname, skipchar)) {
                        return true;
                }
        }
        return false;
}

void footag_gridnames(
        struct footag_pad *p,
        const struct footag_bitmask *const skipmask,
        int rows,
        int cols
)
{
        int i = 0;
        int rowni = 0;

        for (int row = 0; row < rows; row++) {
                const char *rowname;
                rowname = GRID_ROWNAMES[rowni];
                while (rowname && skiprow(rowname, skipmask)) {
                        rowni = intmin(rowni + 1, NELEM(GRID_ROWNAMES)-1);
                        rowname = GRID_ROWNAMES[rowni];
                }
                if (!rowname) { rowname = "?"; }

                for (int col = 0; col < cols; col++) {
                        snprintf(
                                p[i].name,
                                NELEM(p[0].name),
                                "%2s%d",
                                rowname,
                                (col+1) % 1000
                        );
                        p[i].name[NELEM(p[0].name)-1] = '\0';
                        i++;
                }
                rowni = intmin(rowni + 1, NELEM(GRID_ROWNAMES)-1);
        }
}

void footag_gennames(
        struct footag_pad *p,
        int npads,
        int startnum,
        int deltanum
)
{
        for (int i = 0; i < npads; i++) {
                unsigned int padnum = startnum + i * deltanum;
                snprintf(
                    p->name,
                    NELEM(p->name),
                    "%u",
                    padnum % 1000
                );
                p->name[NELEM(p->name)-1] = '\0';
                p++;
        }
}

/* does not set w, h, translation, name, padstack */
void footag_genrow(
        struct footag_pad *p,
        double addx, double addy,
        double pitch,
        /* start positions */
        int x, int y,
        int dx, int dy,
        int rows,
        long angle
)
{
        for (int i = 0; i < rows; i++) {
                p->x = addx;
                if (dx) {
                        p->x = footag_padypos(pitch, rows, x);
                }
                p->y = addy;
                if (dy) {
                        p->y = footag_padypos(pitch, rows, y);
                }
                p->angle = angle;
                x += dx;
                y += dy;
                p++;
        }
}

int footag_realloc_pads(
        struct footag_ctx *ctx,
        int npads
)
{
        struct footag_pad *p;
        p = realloc(
                ctx->spec.pads,
                npads * sizeof (*ctx->spec.pads)
        );
        if (!p) {
                ctx->spec.npads = 0;
                return 1;
        }
        if (p != ctx->spec.pads) {
                memset(p, 0, npads * sizeof (*ctx->spec.pads));
        }
        ctx->spec.npads = npads;
        ctx->spec.pads = p;
        return 0;
}

void footag_gengrid(
        struct footag_pad *p,
        double w, double h,
        double pitch,
        int rows, int cols,
        int prows, int pcols,
        enum footag_padstack stack
)
{
        for (int row = 0; row < rows; row++) {
                double y = footag_padypos(pitch, rows, rows - row - 1);
                footag_genrow(
                        &p[row * cols],
                        0, y,
                        pitch,
                        0, 0,
                        1, 0,
                        cols,
                        FOOTAG_ANGLE_0
                );
        }

        int i = 0;
        for (int row = 0; row < rows; row++) {
                for (int col = 0; col < cols; col++) {
                        p[i].w = w;
                        p[i].h = h;
                        if (
                                row < prows ||
                                col < pcols ||
                                (rows - prows) <= row ||
                                (cols - pcols) <= col
                        ) {
                                p[i].stack = stack;
                        } else {
                                p[i].stack = FOOTAG_PADSTACK_NONE;
                        }
                        i++;
                }
        }
        footag_gennames(p, rows * cols, 1, 1);
}

void footag_genlrow(
        struct footag_pad *p,
        double dist,
        double w, double h,
        double pitch,
        int npads,
        enum footag_padstack stack
)
{
        footag_genrow(
                &p[0],
                - dist / 2.0, 0,
                pitch,
                0, npads - 1,
                0, -1,
                npads,
                FOOTAG_ANGLE_270
        );

        for (int i = 0; i < npads; i++) {
                p[i].w = w;
                p[i].h = h;
                p[i].stack = stack;
        }
        footag_gennames(p, npads, 1, 1);
}

void footag_genrrow(
        struct footag_pad *p,
        double dist,
        double w, double h,
        double pitch,
        int npads,
        enum footag_padstack stack
)
{
        footag_genrow(
                &p[0],
                dist / 2.0, 0,
                pitch,
                0, 0,
                0, 1,
                npads,
                FOOTAG_ANGLE_90
        );

        for (int i = 0; i < npads; i++) {
                p[i].w = w;
                p[i].h = h;
                p[i].stack = stack;
        }
        footag_gennames(p, npads, 1, 1);
}

void footag_gentworow(
        struct footag_pad *p,
        double dist,
        double w, double h,
        double pitch,
        int npads,
        enum footag_padstack stack
)
{
        footag_genrow(
                &p[0],
                - dist / 2.0, 0,
                pitch,
                0, npads/2 - 1,
                0, -1,
                npads / 2,
                FOOTAG_ANGLE_270
        );
        footag_genrow(
                &p[npads/2],
                dist / 2.0, 0,
                pitch,
                0, 0,
                0, 1,
                npads / 2,
                FOOTAG_ANGLE_90
        );

        for (int i = 0; i < npads; i++) {
                p[i].w = w;
                p[i].h = h;
                p[i].stack = stack;
        }
        footag_gennames(p, npads, 1, 1);
}

void footag_genquad(
        struct footag_pad *p,
        double distrow,
        double wrow, double hrow,
        double distcol,
        double wcol, double hcol,
        double pitch,
        int rows, int cols,
        enum footag_padstack stack
)
{
        footag_genrow(
                &p[0],
                - distrow / 2.0, 0,
                pitch,
                0, rows - 1,
                0, -1,
                rows,
                FOOTAG_ANGLE_270
        );
        footag_genrow(
                &p[rows],
                0, - distcol / 2.0,
                pitch,
                0, 0,
                1, 0,
                cols,
                FOOTAG_ANGLE_0
        );
        footag_genrow(
                &p[rows + cols],
                distrow / 2.0, 0,
                pitch,
                0, 0,
                0, 1,
                rows,
                FOOTAG_ANGLE_90
        );
        footag_genrow(
                &p[rows + cols + rows],
                0, distcol / 2.0,
                pitch,
                cols - 1, 0,
                -1, 0,
                cols,
                FOOTAG_ANGLE_180
        );

        for (int i = 0; i < rows; i++) {
                p[i].w = wrow;
                p[i].h = hrow;
                p[i].stack = stack;
        }
        for (int i = rows; i < rows + cols; i++) {
                p[i].w = wcol;
                p[i].h = hcol;
                p[i].stack = stack;
        }
        for (int i = rows + cols; i < rows + cols + rows; i++) {
                p[i].w = wrow;
                p[i].h = hrow;
                p[i].stack = stack;
        }
        for (int i = rows + cols + rows; i < rows + cols + rows + cols; i++) {
                p[i].w = wcol;
                p[i].h = hcol;
                p[i].stack = stack;
        }
        footag_gennames(p, 2 * (cols + rows), 1, 1);
}

void footag_gentwopin(
        struct footag_pad *pads,
        double dist,
        double w, double h,
        enum footag_padstack stack
)
{
        pads[0].stack = pads[1].stack = stack;
        pads[0].x     = - dist / 2.0;
        pads[1].x     = dist / 2.0;
        pads[0].y     = pads[1].y = 0;
        pads[0].w     = pads[1].w = w;
        pads[0].h     = pads[1].h = h;
        pads[0].angle = FOOTAG_ANGLE_270;
        pads[1].angle = FOOTAG_ANGLE_90;
        pads[0].name[0] = '1';
        pads[0].name[1] = '\0';
        pads[1].name[0] = '2';
        pads[1].name[1] = '\0';
}

void footag_ipc7351b_setrrectpad(struct footag_pad *p)
{
        if (p->stack == FOOTAG_PADSTACK_SMD_RRECT) {
                double radius = fmin(p->w, p->h) / 5.0;
                radius = fmin(radius, 0.25);
                p->param = radius;
        }
}

void footag_ipc7351b_setrrectpads(const struct footag_spec *s)
{
        for (int i = 0; i < s->npads; i++) {
                footag_ipc7351b_setrrectpad(&s->pads[i]);
        }
}

void footag_snap(
        struct footag_spec *s, double grid
)
{
        if (!grid) { return; }
        for (int i = 0; i < s->npads; i++) {
                struct footag_pad *p = &s->pads[i];
                p->x = snap(p->x, grid);
                p->y = snap(p->y, grid);
                p->w = snap(p->w, grid);
                p->h = snap(p->h, grid);
                p->diam = snap(p->diam, grid);
                p->holediam = snap(p->holediam, grid);
                p->param = snap(p->param, grid);
        }
        s->body.minx = snap(s->body.minx, grid);
        s->body.maxx = snap(s->body.maxx, grid);
        s->body.miny = snap(s->body.miny, grid);
        s->body.maxy = snap(s->body.maxy, grid);
}

void footag_setcourtyard(
        struct footag_spec *s,
        double cyexc
)
{
        /* pads and outline is now at its final (rounded) value */
        s->courtyard = s->body;
        /* determine theoretical courtyard */
        for (int i = 0; i < s->npads; i++) {
                struct footag_pad *p = &s->pads[i];
                double w = p->w;
                double h = p->h;
                if (p->angle == FOOTAG_ANGLE_90 || p->angle == FOOTAG_ANGLE_270) {
                        w = p->h;
                        h = p->w;
                }
                double xadd = fmax(w, p->diam) / 2;
                s->courtyard.maxx = fmax(s->courtyard.maxx, p->x + xadd);
                s->courtyard.minx = fmin(s->courtyard.minx, p->x - xadd);
                double yadd = fmax(h, p->diam) / 2;
                s->courtyard.maxy = fmax(s->courtyard.maxy, p->y + yadd);
                s->courtyard.miny = fmin(s->courtyard.miny, p->y - yadd);
        }
        /* add courtyard excess */
        s->courtyard.maxx += cyexc;
        s->courtyard.minx -= cyexc;
        s->courtyard.maxy += cyexc;
        s->courtyard.miny -= cyexc;
        /* (round it) */
}

