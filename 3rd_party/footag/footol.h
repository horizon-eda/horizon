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

#ifndef FOOTAG_FOOTOL_H
#define FOOTAG_FOOTOL_H

#include <math.h>

/* interface for working with tolerances */

struct footol {
        double nom;
        double min;
        double max;
};

static inline void footol_nom(
        struct footol *tol,
        double nom
)
{
        tol->min = tol->max = tol->nom = nom;
}

static inline void footol_minmax(
        struct footol *tol,
        double min,
        double max
)
{
        tol->min = min;
        tol->max = max;
        tol->nom = min + fabs(max - min) / 2;
}

static inline void footol_minmaxnom(
        struct footol *tol,
        double min,
        double max,
        double nom
)
{
        tol->min = min;
        tol->max = max;
        tol->nom = nom;
}

static inline void footol_pm(
        struct footol *tol,
        double nom,
        double plusminus
)
{
        tol->nom = nom;
        tol->min = nom - fabs(plusminus);
        tol->max = nom + fabs(plusminus);
}

static inline struct footol footol_auto(
        double a,
        double b
)
{
        struct footol ret;
        if (fabs(b) < fabs(a)) {
                footol_pm(&ret, a, b);
        } else {
                footol_minmax(&ret, a, b);
        }
        return ret;
}

static inline double footol_range(
        const struct footol *const tol
)
{
        return tol->max - tol->min;
}

static const double FOOTOL_MM_PER_INCH = 25.4;

static inline double footol_inchtomm(
        double mmval
)
{
        return mmval * FOOTOL_MM_PER_INCH;
}

static inline struct footol footol_autoi(
        double a,
        double b
)
{
        return footol_auto(footol_inchtomm(a), footol_inchtomm(b));
}

#endif

