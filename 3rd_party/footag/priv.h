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

#include <footag/footag.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#ifndef UNUSED
 #define UNUSED(i) (void) (sizeof (i))
#endif

#ifndef NELEM
 #define NELEM(a) ((sizeof a) / (sizeof (a[0])))
#endif

struct footag_op {
        size_t size;
        int (*init)(struct footag_ctx *ctx);
        int (*fini)(struct footag_ctx *ctx);
        int (*calc)(struct footag_ctx *ctx);
        const char * (*hint)(
                struct footag_ctx *ctx,
                const struct footag_param *p
        );
        /* Description of component type, for use by application. */
        const struct footag_typeinfo *info;
        /* Parameter template, terminated with  */
        const struct footag_param *temp;
};

struct footag_ctx {
        const struct footag_op *op;
        struct footag_param *param;
        struct footag_spec spec;
};

#define ITEM_NONE { \
        .type   = FOOTAG_DATA_NONE, \
}

#define ITEM_BOOL(_val) { \
        .type   = FOOTAG_DATA_BOOL, \
        .data   = { \
                .b = (_val), \
        }, \
}

#define ITEM_INTEGER(_val, _step, _min, _max) { \
        .type   = FOOTAG_DATA_INTEGER, \
        .data   = { \
                .i = { \
                        .val    = (_val), \
                        .step   = (_step), \
                        .min    = (_min), \
                        .max    = (_max), \
                }, \
        }, \
}

#define ITEM_FLOAT(_val) { \
        .type   = FOOTAG_DATA_FLOAT, \
        .data   = { \
                .f = (_val), \
        }, \
}

#define ITEM_LENGTH(_val) { \
        .type   = FOOTAG_DATA_LENGTH, \
        .data   = { \
                .l = (_val), \
        }, \
}

/* nom, plus minus */
#define ITEM_TOL_PM(_nom, _pm) { \
        .type   = FOOTAG_DATA_TOL, \
        .data   = { \
                .t = { \
                        .nom    = (_nom), \
                        .min    = (_nom - _pm), \
                        .max    = (_nom + _pm), \
                } \
        }, \
}

/* min, max */
#define ITEM_TOL_MM(_min, _max) { \
        .type   = FOOTAG_DATA_TOL, \
        .data   = { \
                .t = { \
                        .nom    = ((_min + _max) / 2.0), \
                        .min    = (_min), \
                        .max    = (_max), \
                } \
        }, \
}

#define ITEM_TOL_DATA(a, b) { \
        .nom    = (a) <= (b) ? ((a + b) / 2.0) : (a), \
        .min    = (a) <= (b) ? (a) : (a - b), \
        .max    = (a) <= (b) ? (b) : (a + b), \
}

#define ITEM_TOL(_a, _b) { \
        .type   = FOOTAG_DATA_TOL, \
        .data   = { \
                .t = ITEM_TOL_DATA(_a, _b), \
        }, \
}

#define ITEM_ENUM(_val, _num, ...) { \
        .type   = FOOTAG_DATA_ENUM, \
        .data   = { \
                .e = { \
                        .val    = _val, \
                        .num    = _num, \
                        .strs   = (const char *const []) { __VA_ARGS__ }, \
                }, \
        }, \
}

#define ITEM_BITMASK(_val, _num, ...) { \
        .type   = FOOTAG_DATA_BITMASK, \
        .data   = { \
                .m = { \
                        .val    = _val, \
                        .num    = _num, \
                        .strs   = (const char *const []) { __VA_ARGS__ }, \
                }, \
        }, \
}

#define PARAM_HEADER(_id, _name, _abbr) \
        .id     = PARAM_ ## _id, \
        .name   = _name, \
        .abbr   = _abbr

#define PARAM_TERM { \
        PARAM_HEADER(DONE, "done", "done"), \
        .item   = ITEM_NONE, \
}

#define PARAM_TOPIC(_name) { \
        .id     = PARAM_TOPIC, \
        .name   = _name, \
        .abbr   = "", \
        .item   = ITEM_NONE, \
}

#define PARAM_B(_id, _name, _abbr, _val) { \
        PARAM_HEADER(_id, _name, _abbr), \
        .item   = ITEM_BOOL(_val), \
}

#define PARAM_I(_id, _name, _abbr, _val, _step, _min, _max) { \
        PARAM_HEADER(_id, _name, _abbr), \
        .item   = ITEM_INTEGER(_val, _step, _min, _max), \
}

#define PARAM_F(_id, _name, _abbr, _val) { \
        PARAM_HEADER(_id, _name, _abbr), \
        .item   = ITEM_FLOAT(_val), \
}

#define PARAM_L(_id, _name, _abbr, _val) { \
        PARAM_HEADER(_id, _name, _abbr), \
        .item   = ITEM_LENGTH(_val), \
}

#define PARAM_TPM(_id, _name, _abbr, _nom, _pm) { \
        PARAM_HEADER(_id, _name, _abbr), \
        .item   = ITEM_TOL_PM(_nom, _pm), \
}

#define PARAM_TMM(_id, _name, _abbr, _min, _max) { \
        PARAM_HEADER(_id, _name, _abbr), \
        .item   = ITEM_TOL_MM(_min, _max), \
}

/*
 * interprets _a and _b, based on their values, as one of the following:
 *   1. _a: nominal, _b: plus/minus
 *   2. _a: min, _b: max
 */
#define PARAM_T(_id, _name, _abbr, _a, _b) { \
        PARAM_HEADER(_id, _name, _abbr), \
        .item   = ITEM_TOL(_a, _b), \
}

#define PARAM_E(_id, _name, _abbr, _val, _num, ...) { \
        PARAM_HEADER(_id, _name, _abbr), \
        .item   = ITEM_ENUM(_val, _num, __VA_ARGS__), \
}

#define PARAM_M(_id, _name, _abbr, _val, _num, ...) { \
        PARAM_HEADER(_id, _name, _abbr), \
        .item   = ITEM_BITMASK(_val, _num, __VA_ARGS__), \
}

#define PARAM_CALC_IPC7351B \
        PARAM_TOPIC("Calc"), \
        PARAM_E(CALC_D, "Density", "-", FOOTAG_LEVEL_N, FOOTAG_LEVEL_NUM, \
                "Most", "Nominal", "Least" \
        ), \
        PARAM_L(CALC_F, "Fabrication", "-", 0.10), \
        PARAM_L(CALC_P, "Placement",   "-", 0.10), \
        PARAM_TOPIC("Generate"), \
        PARAM_E(CALC_ROUND, "Round-off", "-", 3, 4, \
                "None", "0.01 mm", "0.02 mm", "0.05 mm" \
        )

#define PARAM_CALC_IPC7351B_HIRES \
        PARAM_TOPIC("Calc"), \
        PARAM_E(CALC_D, "Density", "-", FOOTAG_LEVEL_N, FOOTAG_LEVEL_NUM, \
                "Most", "Nominal", "Least" \
        ), \
        PARAM_L(CALC_F, "Fabrication", "-", 0.05), \
        PARAM_L(CALC_P, "Placement",   "-", 0.05), \
        PARAM_TOPIC("Generate"), \
        PARAM_E(CALC_ROUND, "Round-off", "-", 1, 4, \
                "None", "0.01 mm", "0.02 mm", "0.05 mm" \
        )

#define PARAM_CALC_IPC7251DRAFT1 \
        PARAM_TOPIC("Calc"), \
        PARAM_E(CALC_D, "Level", "-", FOOTAG_LEVEL_N, FOOTAG_LEVEL_NUM, \
                "A (Maximum)", "B (Nominal)", "C (Least)" \
        ), \
        PARAM_TOPIC("Generate"), \
        PARAM_E(CALC_ROUND, "Round-off", "-", 3, 4, \
                "None", "0.01 mm", "0.02 mm", "0.05 mm" \
        )

#define PARAM_PADSTACK_SMD_RECTS \
        PARAM_E(CALC_STACK, "Padstack", "-", 1, 2, \
                "Rectangular", "Rounded rectangular" \
        )

int footag_init_from_template(
        struct footag_ctx *ctx,
        const struct footag_param *temp
);

int footag_init_default(
        struct footag_ctx *ctx
);

int footag_init_twopin(
        struct footag_ctx *ctx
);

int footag_fini_default(
        struct footag_ctx *ctx
);

const union footag_data *footag_data_by_name(
        struct footag_ctx *ctx,
        const char *topic,
        const char *name
);

const union footag_data *footag_data_by_id(
        struct footag_ctx *ctx,
        int id
);

#define GETID(_ctx, _id) footag_data_by_id(_ctx, PARAM_ ## _id)

enum {
        PARAM_DONE      = FOOTAG_PARAM_DONE,
        PARAM_IGNORE    = FOOTAG_PARAM_IGNORE,
        PARAM_TOPIC     = FOOTAG_PARAM_TOPIC,
        PARAM_BODY_L,
        PARAM_BODY_W,
        PARAM_BODY_H,
        /* number of pins */
        PARAM_BODY_N,
        PARAM_BODY_Nx,
        /* rows */
        PARAM_BODY_R,
        /* columns */
        PARAM_BODY_C,
        /* perimeter rows and columns */
        PARAM_BODY_PR,
        PARAM_BODY_PC,
        /* polarized */
        PARAM_BODY_POL,
        PARAM_LEAD_L,
        PARAM_LEAD_W,
        PARAM_LEAD_W1,
        /* diameter */
        PARAM_LEAD_D,
        /* pullback */
        PARAM_LEAD_PB,
        /* span */
        PARAM_LEAD_S,
        PARAM_LEAD_Sx,
        /* pitch: spacing between leads or castellations */
        PARAM_LEAD_P,
        PARAM_CALC_D,
        PARAM_CALC_F,
        PARAM_CALC_P,
        PARAM_CALC_STACK,
        PARAM_CALC_ROUND,
        PARAM_NAME_ROW,
        PARAM_CUSTOM,
};

/* translate from PARAM_CALC_D to FOOTAG_LEVEL_ */
static inline int footag_get_density(
        const struct footag_enum *e
)
{
        return e->val;
}

/*
 * IPC-7351B seems to say 0.05
 */
static const double ROUNDOFF_TO_GRID[4] = {
        [0] = 0.00,
        [1] = 0.01,
        [2] = 0.02,
        [3] = 0.05,
};

/* NOTE: snap is an operation and grid is a property */
static inline double snap(double v, double grid)
{
        if (!grid) { return v; }
        return round(v / grid) * grid;
}

/*
 * round to grid:
 * - placement
 * - dimensions
 * - parameter (radius)
 */
void footag_snap(
        struct footag_spec *s,
        double grid
);

void footag_setcourtyard(
        struct footag_spec *s,
        double cyexc
);

static inline double footag_padypos(double pitch, int rows, int row)
{
        double y;
        y = - 1 * ((rows / 2.0) - 1.0 / 2);
        y += row;
        y *= pitch;
        return y;
}

void footag_gridnames(
        struct footag_pad *p,
        const struct footag_bitmask *const skipmask,
        int rows,
        int cols
);

void footag_gennames(
        struct footag_pad *p,
        int npads,
        int startnum,
        int deltanum
);

void footag_genrow(
        struct footag_pad *p,
        double addx, double addy,
        double pitch,
        /* start positions */
        int x, int y,
        int dx, int dy,
        int rows,
        long angle
);

void footag_gentworow(
        struct footag_pad *p,
        double dist,
        double w, double h,
        double pitch,
        int npads,
        enum footag_padstack stack
);

void footag_genquad(
        struct footag_pad *p,
        double distrow,
        double wrow, double hrow,
        double distcol,
        double wcol, double hcol,
        double pitch,
        int rows, int cols,
        enum footag_padstack stack
);

void footag_gengrid(
        struct footag_pad *p,
        double w, double h,
        double pitch,
        int rows, int cols,
        int prows, int pcols,
        enum footag_padstack stack
);

void footag_genlrow(
        struct footag_pad *p,
        double dist,
        double w, double h,
        double pitch,
        int npads,
        enum footag_padstack stack
);

void footag_genrrow(
        struct footag_pad *p,
        double dist,
        double w, double h,
        double pitch,
        int npads,
        enum footag_padstack stack
);

void footag_gentwopin(
        struct footag_pad *p,
        double dist,
        double w, double h,
        enum footag_padstack stack
);

void footag_ipc7351b_setrrectpad(
        struct footag_pad *p
);

void footag_ipc7351b_setrrectpads(
        const struct footag_spec *s
);

int footag_realloc_pads(
        struct footag_ctx *ctx,
        int npads
);

static inline int intmin(int a, int b) { return a < b ? a : b; }
static inline int intmax(int a, int b) { return a < b ? b : a; }

static inline struct footag_rlimit footag_crlimit(
        double w,
        double h
)
{
        return (const struct footag_rlimit) {
                .minx = - w / 2.0,
                .maxx =   w / 2.0,
                .miny = - h / 2.0,
                .maxy =   h / 2.0,
        };
}

const char *footag_hint_ipc7251draft1(
        struct footag_ctx *ctx,
        const struct footag_param *p
);

struct ipcb_ref;
void footag_setref_ipc7351b(
        struct footag_spec *s,
        const struct ipcb_ref *ref
);

struct ipc7251_ref;
void footag_setref_ipc7251draft1(
        struct footag_spec *s,
        const struct ipc7251_ref *ref
);

