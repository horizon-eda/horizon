/*
 * Copyright 2018 Martin Åberg
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

#include <math.h>
#include <stddef.h>

#include "priv.h"

/*
 * Calculate Euclidian distance from origin to (a,b,c)
 *
 * NOTE: This is NOT the root mean square as claimed in IPC-7351B.
 */
static double ed3(double a, double b, double c)
{
        return sqrt(a * a + b * b + c * c);
}

static void calc(
        const struct ipcb_comp *comp,
        const struct ipcb_attr *attr,
        struct ipcb_twospec *spec
)
{
        double cl, cs, cw;
        double smax_mod;
        double smin = comp->l.min - 2 * comp->t.max;
        double smax = comp->l.max - 2 * comp->t.min;

        {
                double ltol = footol_range(&comp->l);
                double ttol = footol_range(&comp->t);
                double wtol = footol_range(&comp->w);
                double stol = smax - smin;
                double stol_ed3;

                stol_ed3 = ed3(ltol, ttol, ttol);
                smax_mod = smax - (stol - stol_ed3) / 2;

                cl = ltol;
                cs = stol_ed3;
                cw = wtol;
        }

        spec->smin = smin;
        spec->zmax = comp->l.min + ed3(cl, attr->f, attr->p);
        spec->gmin = smax_mod   - ed3(cs, attr->f, attr->p);
        spec->xmax = comp->w.min + ed3(cw, attr->f, attr->p);
}

static void calcuser(
        const struct ipcbtable *table,
        const struct ipcb_attr *attr,
        struct ipcb_twospec *spec
)
{
        spec->round     = table->round;
        spec->cyexc     = table->cyexc[attr->density];
        spec->ref.where = table->ref;
        spec->ref.what  = table->desc;
        spec->land.dist = (spec->zmax + spec->gmin) / 2.0;
        spec->land.w    = spec->xmax;
        spec->land.h    = (spec->zmax - spec->gmin) / 2.0;
}

static int get_common(
        struct ipcb_twospec *spec,
        const struct ipcb_comp *comp,
        const struct ipcb_attr *attr,
        const struct ipcbtable *table
)
{
        calc(comp, attr, spec);
        spec->zmax += 2 * table->toe [attr->density];
        spec->gmin -= 2 * table->heel[attr->density];
        spec->xmax += 2 * table->side[attr->density];
        calcuser(table, attr, spec);
        return 0;
}

/* Chip components have two different tables, selected on dimensions. */
int ipcb_get_chip(
        struct ipcb_twospec *spec,
        const struct ipcb_comp *comp,
        const struct ipcb_attr *attr
)
{
        if ((comp->l.nom < 1.6) || (comp->w.nom < 0.8)) {
                return get_common(spec, comp, attr, &ipcb_table3_6);
        } else {
                return get_common(spec, comp, attr, &ipcb_table3_5);
        }
}

int ipcb_get_melf(
        struct ipcb_twospec *spec,
        const struct ipcb_comp *comp,
        const struct ipcb_attr *attr
)
{
        return get_common(spec, comp, attr, &ipcb_table3_7);
}

/* Molded have lead in opposite direction. */
int ipcb_get_molded(
        struct ipcb_twospec *spec,
        const struct ipcb_comp *comp,
        const struct ipcb_attr *attr
)
{
        const struct ipcbtable *table;

        calc(comp, attr, spec);

        table = &ipcb_table3_13;
        spec->zmax += 2 * table->heel[attr->density];
        spec->gmin -= 2 * table->toe [attr->density];
        spec->xmax += 2 * table->side[attr->density];

        calcuser(table, attr, spec);

        return 0;
}

int ipcb_get_sideconcave(
        struct ipcb_twospec *spec,
        const struct ipcb_comp *comp,
        const struct ipcb_attr *attr
)
{
        return get_common(spec, comp, attr, &ipcb_table3_9);
}

int ipcb_get_concavearray(
        struct ipcb_twospec *spec,
        const struct ipcb_comp *comp,
        const struct ipcb_attr *attr
)
{
        return get_common(spec, comp, attr, &ipcb_table3_9);
}

int ipcb_get_convexarray(
        struct ipcb_twospec *spec,
        const struct ipcb_comp *comp,
        const struct ipcb_attr *attr
)
{
        return get_common(spec, comp, attr, &ipcb_table3_10);
}

int ipcb_get_flatarray(
        struct ipcb_twospec *spec,
        const struct ipcb_comp *comp,
        const struct ipcb_attr *attr
)
{
        return get_common(spec, comp, attr, &ipcb_table3_11);
}

int ipcb_get_gullwing(
        struct ipcb_twospec *spec,
        const struct ipcb_comp *comp,
        const struct ipcb_attr *attr,
        double pitch
)
{
        const struct ipcbtable *table;

        calc(comp, attr, spec);

        table = &ipcb_table3_2;
        if (pitch <= 0.625) {
                table = &ipcb_table3_3;
        }

        double heel = table->heel[attr->density];

        /*
         * NOTE: Table 3-2 and Table 3-3, Note 1 and Note 2 not implemented.
         */
        if (0) {
                heel = (double []) {
                        [IPCB_DENSITY_M] = 0.25,
                        [IPCB_DENSITY_N] = 0.15,
                        [IPCB_DENSITY_L] = 0.05
                }[attr->density];
        }

        spec->zmax += 2 * table->toe [attr->density];
        spec->gmin -= 2 * heel;
        spec->xmax += 2 * table->side[attr->density];

        calcuser(table, attr, spec);

        return 0;
}

/* "J Leads" have lead in opposite direction. */
int ipcb_get_jlead(
        struct ipcb_twospec *spec,
        const struct ipcb_comp *comp,
        const struct ipcb_attr *attr
)
{
        const struct ipcbtable *table;

        calc(comp, attr, spec);

        table = &ipcb_table3_4;
        spec->zmax += 2 * table->heel[attr->density];
        spec->gmin -= 2 * table->toe [attr->density];
        spec->xmax += 2 * table->side[attr->density];

        calcuser(table, attr, spec);

        return 0;
}

/* NOTE: Note 1 not implemented because I don't understand it. */
int ipcb_get_son(
        struct ipcb_twospec *spec,
        const struct ipcb_comp *comp,
        const struct ipcb_attr *attr
)
{
        return get_common(spec, comp, attr, &ipcb_table3_16);
}

/* NOTE: Note 1 not implemented because I don't understand it. */
int ipcb_get_qfn(
        struct ipcb_twospec *spec,
        const struct ipcb_comp *comp,
        const struct ipcb_attr *attr
)
{
        return get_common(spec, comp, attr, &ipcb_table3_15);
}

int ipcb_get_pson(
        struct ipcb_twospec *spec,
        const struct ipcb_comp *comp,
        const struct ipcb_attr *attr,
        double pullback
)
{
        const struct ipcbtable *table;
        /* adjusted for pullback */
        struct ipcb_comp compx = *comp;

        footol_minmax(
                &compx.l,
                comp->l.min - 2 * pullback,
                comp->l.max - 2 * pullback
        );

        calc(&compx, attr, spec);

        table = &ipcb_table3_18;

        spec->zmax += 2 * table->toe [attr->density];
        spec->gmin -= 2 * table->heel[attr->density];
        spec->xmax += 2 * table->side[attr->density];

        calcuser(table, attr, spec);

        return 0;
}

int ipcb_get_sodfl(
        struct ipcb_twospec *spec,
        const struct ipcb_comp *comp,
        const struct ipcb_attr *attr
)
{
        return get_common(spec, comp, attr, &ipcb_table3_22);
}

int ipcb_get_capae(
        struct ipcb_twospec *spec,
        const struct ipcb_comp *comp,
        const struct ipcb_attr *attr,
        double height
)
{
        const struct ipcbtable *const table = &ipcb_table3_20;

        calc(comp, attr, spec);

        const double *toe  = table->toe;
        const double *heel = table->heel;
        const double *side = table->side;
        if (10.0 <= height) {
                static const double hitoe[] = {
                        [IPCB_DENSITY_M] =  1.00,
                        [IPCB_DENSITY_N] =  0.70,
                        [IPCB_DENSITY_L] =  0.40
                };
                static const double hiheel[] = {
                        [IPCB_DENSITY_M] =  0.00,
                        [IPCB_DENSITY_N] = -0.05,
                        [IPCB_DENSITY_L] = -0.10
                };
                static const double hiside[] = {
                        [IPCB_DENSITY_M] =  0.60,
                        [IPCB_DENSITY_N] =  0.50,
                        [IPCB_DENSITY_L] =  0.40
                };
                toe  = hitoe;
                heel = hiheel;
                side = hiside;
        }

        spec->zmax += 2 * toe [attr->density];
        spec->gmin -= 2 * heel[attr->density];
        spec->xmax += 2 * side[attr->density];

        calcuser(table, attr, spec);

        return 0;
}

struct balltoland { double ball; double land; };

static const struct balltoland BALLTOLAND_COLLAPS[] = {
        {0.75, 0.60},
        {0.65, 0.55},
        {0.60, 0.50},
        {0.55, 0.50},
        {0.50, 0.45},
        {0.45, 0.40},
        {0.40, 0.35},
        {0.35, 0.33},
        {0.30, 0.25},
        {0.25, 0.20},
        {0.20, 0.20},
        {0.17, 0.18},
        {0.15, 0.15},
        {-1,   -1},
};

static const struct balltoland BALLTOLAND_NONCOLLAPS[] = {
        {0.75, 0.90},
        {0.60, 0.75},
        {0.55, 0.70},
        {0.50, 0.60},
        {0.45, 0.55},
        {0.40, 0.50},
        {0.30, 0.38},
        {0.25, 0.33},
        {0.20, 0.24},
        {0.17, 0.21},
        {0.15, 0.19},
        {-1,   -1},
};

static double findbgaland(double lead, const struct balltoland *btol)
{
        double land = 100000.0;
        double mindiff = 100000.0;

        while (0 < btol->ball) {
                double diff = fabs(lead - btol->ball) / lead;
                if (diff < mindiff) {
                        land = btol->land;
                        mindiff = diff;
                }
                btol++;
        }

        return land;
}

int ipcb_get_bga(struct ipcb_bgaspec *spec, double diam, int collapsing)
{
        if (collapsing) {
                spec->ref.where = "14-5";
                spec->ref.what =
                        "Land Approximation (mm) for "
                        "Collapsible Solder Balls";
                spec->diam = findbgaland(diam, &BALLTOLAND_COLLAPS[0]);
        } else {
                spec->ref.where = "14-6";
                spec->ref.what =
                        "Land Approximation (mm) for "
                        "Non-Collapsible Solder Balls";
                spec->diam = findbgaland(diam, &BALLTOLAND_NONCOLLAPS[0]);
        }
        if (100 < spec->diam) {
                return 1;
        }

        return 0;
}

