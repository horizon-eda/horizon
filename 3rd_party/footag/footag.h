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

#ifndef FOOTAG_FOOTAG_H
#define FOOTAG_FOOTAG_H

#include <stddef.h>
#include <stdbool.h>
#include <footag/footol.h>

enum footag_data_type {
        FOOTAG_DATA_NONE,
        /* boolean */
        FOOTAG_DATA_BOOL,
        /* integers, no unit */
        FOOTAG_DATA_INTEGER,
        /* floating point numbers, no unit */
        FOOTAG_DATA_FLOAT,
        /* length in mm, represented as point numbers */
        FOOTAG_DATA_LENGTH,
        /* length in mm, with tolerance */
        FOOTAG_DATA_TOL,
        /* string/unsigned pairs */
        FOOTAG_DATA_ENUM,
        FOOTAG_DATA_BITMASK,
        FOOTAG_DATA_NUM,
};

struct footag_enum {
        unsigned int val;
        unsigned int num;
        const char *const *strs;
};

struct footag_bitmask {
        unsigned long val;
        unsigned int num;
        const char *const *strs;
};

struct footag_integer {
        int val;
        int step;
        int min;
        int max;
};

union footag_data {
        struct footag_integer i;
        double l;
        double f;
        struct footol t;
        struct footag_enum e;
        struct footag_bitmask m;
        bool b;
};

struct footag_item {
        enum footag_data_type type;
        union footag_data data;
};

enum {
        FOOTAG_PARAM_DONE,
        FOOTAG_PARAM_IGNORE,
        FOOTAG_PARAM_TOPIC,
};

struct footag_param {
        int id;
        const char *name;
        const char *abbr;
        struct footag_item item;
};

#define FOOTAG_ANGLE_0          ((long) (0))
#define FOOTAG_ANGLE_90         ((long) (1 * (1<<16) / 4))
#define FOOTAG_ANGLE_180        ((long) (2 * (1<<16) / 4))
#define FOOTAG_ANGLE_270        ((long) (3 * (1<<16) / 4))

enum footag_padstack {
        FOOTAG_PADSTACK_SMD_RECT        = 0,
        FOOTAG_PADSTACK_SMD_RRECT,
        FOOTAG_PADSTACK_SMD_CIRC,
        FOOTAG_PADSTACK_SMD_OBLONG,
        FOOTAG_PADSTACK_SMD_DSHAPE,
        FOOTAG_PADSTACK_TH_ROUND,
        FOOTAG_PADSTACK_TH_ROUND_RPAD,
        FOOTAG_PADSTACK_NONE,
        FOOTAG_PADSTACK_NUM,
};

struct footag_pad {
        enum footag_padstack stack;
        double param;
        double x, y;
        double w, h;
        double diam;
        double holediam;
        long angle;
        /* must be NUL terminated */
        char name[8];
};

/* "rectangular limit" */
struct footag_rlimit {
        double minx, maxx;
        double miny, maxy;
};

static const struct footag_rlimit FOOTAG_RLIMIT_NONE = {
        .minx = 0.0, .maxx = 0.0,
        .miny = 0.0, .maxy = 0.0,
};

struct footag_ref {
        const char *doc;
        const char *where;
        const char *what;
};

/* NOTE: It is up to the user to name the pads. He can sort them if in doubt. */
struct footag_spec {
        struct footag_ref ref;
        int npads;
        struct footag_pad *pads;
        struct footag_rlimit body;
        struct footag_rlimit courtyard;
};

/*
 * Three different levels of something, which can be used where appropriate,
 * for example in IPC-7351B and IPC-7251.
 */
enum {
        FOOTAG_LEVEL_0 = 0, FOOTAG_LEVEL_1, FOOTAG_LEVEL_2,
        FOOTAG_LEVEL_A = 0, FOOTAG_LEVEL_B, FOOTAG_LEVEL_C,
        /* most             nominal         least */
        FOOTAG_LEVEL_M = 0, FOOTAG_LEVEL_N, FOOTAG_LEVEL_L,
        FOOTAG_LEVEL_NUM = 3,
};

enum footag_type {
        FOOTAG_TYPE_CHIP,
        FOOTAG_TYPE_CHIPARRAY,
        FOOTAG_TYPE_LEDSC,
        FOOTAG_TYPE_MELF,
        FOOTAG_TYPE_MOLDED,
        FOOTAG_TYPE_CAPAE,
        FOOTAG_TYPE_SO,
        FOOTAG_TYPE_SOD,
        FOOTAG_TYPE_SODFL,
        FOOTAG_TYPE_SOJ,
        FOOTAG_TYPE_QFP,
        FOOTAG_TYPE_SON,
        FOOTAG_TYPE_QFN,
        FOOTAG_TYPE_PSON,
        FOOTAG_TYPE_PQFN,
        FOOTAG_TYPE_BGA,
        FOOTAG_TYPE_SOT223,
        FOOTAG_TYPE_SOT23,
        FOOTAG_TYPE_DIP,
        FOOTAG_TYPE_SIP,
        FOOTAG_TYPE_PGA,
        FOOTAG_TYPE_NUM,
};

struct footag_typeinfo {
        enum footag_type type;
        char *name;
        char *brief;
        char *desc;
};

struct footag_ctx;

const struct footag_typeinfo *footag_get_typeinfo(
        enum footag_type footype
);

struct footag_ctx *footag_open(
        int footype
);

int footag_close(
        struct footag_ctx *ctx
);

struct footag_param *footag_get_param(
        struct footag_ctx *ctx
);

const struct footag_spec *footag_get_spec(
        struct footag_ctx *ctx
);

const char *footag_hint(
        struct footag_ctx *ctx,
        const struct footag_param *p
);

#endif

