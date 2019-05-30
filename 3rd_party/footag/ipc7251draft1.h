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

#ifndef FOOTAG_IPC7251DRAFT1_H
#define FOOTAG_IPC7251DRAFT1_H

/*
 * This is an implementation of part of
 *   "IPC-7251: Generic Requirements for Through-Hole Design and Land Pattern
 *   Standard, 1st Working Draft - June 2008"
 *
 * The working draft is available on the IPC website.
 *
 * The following parts of the standard are covered by the implementation:
 *   - Land pattern dimension and spacing calculations
 *   - Courtyard properties
 *
 * These parts are not implemented:
 *   - Land pattern naming convention
 *
 * Note also that the following items are not part of IPC-7251 draft 1 or this
 * implementation:
 *   - Silk screen
 *   - Assembly outline
 */

enum ipc7251_density {
        IPC7251_DENSITY_M,
        IPC7251_DENSITY_N,
        IPC7251_DENSITY_L,
        IPC7251_DENSITY_NUM,
};

static const char IPC7251_DENSITY_TO_CHAR[IPC7251_DENSITY_NUM] = {'M','N','L'};

struct ipc7251_ref {
        const char *where;
        const char *what;
};

struct ipc7251_spec {
        struct ipc7251_ref ref;
        double holediam;
        double paddiam;
        double antipaddiam;
        double round;
        double cyexc;
};

/* Return 0 iff success */
int ipc7251_get_spec(
        struct ipc7251_spec *spec,
        double leaddiam_max,
        enum ipc7251_density density
);

#endif

