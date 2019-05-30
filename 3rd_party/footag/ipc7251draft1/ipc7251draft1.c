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

#include <footag/ipc7251draft1.h>

struct ipctable {
        const char *ref;
        const char *desc;
        /* "Hole Diameter Factor" */
        double hole[IPC7251_DENSITY_NUM];
        /* "Int. & Ext. Annular ring Excess (added to hole dia.)" */
        double annul[IPC7251_DENSITY_NUM];
        /* "Anti Pad Excess (added to hole dia.) */
        double anti[IPC7251_DENSITY_NUM];
        /* "Courtyard Excess from Component body and/or lands" */
        double cyexc[IPC7251_DENSITY_NUM];
        /* Courtyard Round-off factor */
        double round;
};

#define DENS_M IPC7251_DENSITY_M
#define DENS_N IPC7251_DENSITY_N
#define DENS_L IPC7251_DENSITY_L

static const struct ipctable table3_9 = {
        .ref    = "Table 3-9",
        .desc   = "Multiple Leaded Semiconductors",
        .hole   = { [DENS_M] =  0.25, [DENS_N] =  0.20, [DENS_L] =  0.15 },
        .annul  = { [DENS_M] =  0.50, [DENS_N] =  0.35, [DENS_L] =  0.30 },
        .anti   = { [DENS_M] =  1.00, [DENS_N] =  0.70, [DENS_L] =  0.50 },
        .cyexc  = { [DENS_M] =  0.50, [DENS_N] =  0.25, [DENS_L] =  0.10 },
        .round  = 0.10,
};

#undef DENS_M
#undef DENS_N
#undef DENS_L

int ipc7251_get_spec(
        struct ipc7251_spec *spec,
        double leaddiam_max,
        enum ipc7251_density density
)
{
        const struct ipctable *table = &table3_9;

        spec->ref.where = table->ref;
        spec->ref.what = table->desc;
        spec->holediam = leaddiam_max + table->hole[density];
        /* yes it shall be * 1: see IPC-2221A */
        spec->paddiam = spec->holediam + 1 * table->annul[density];
        spec->antipaddiam = spec->holediam + table->anti[density];
        spec->round = table->round;
        spec->cyexc = table->cyexc[density];

        return 0;
}

