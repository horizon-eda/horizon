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

#include <footag/ipc7351b.h>

#ifndef UNUSED
 #define UNUSED(i) (void) (sizeof (i))
#endif

#ifndef NELEM
 #define NELEM(a) ((sizeof a) / (sizeof (a[0])))
#endif

/* From IPC-7351B */
struct ipcbtable {
        const char *ref;
        const char *desc;
        double toe[IPCB_DENSITY_NUM];
        double heel[IPCB_DENSITY_NUM];
        double side[IPCB_DENSITY_NUM];
        double round;
        double cyexc[IPCB_DENSITY_NUM];
};

extern const struct ipcbtable ipcb_table3_2;
extern const struct ipcbtable ipcb_table3_3;
extern const struct ipcbtable ipcb_table3_4;
extern const struct ipcbtable ipcb_table3_5;
extern const struct ipcbtable ipcb_table3_6;
extern const struct ipcbtable ipcb_table3_7;
extern const struct ipcbtable ipcb_table3_9;
extern const struct ipcbtable ipcb_table3_10;
extern const struct ipcbtable ipcb_table3_11;
extern const struct ipcbtable ipcb_table3_13;
extern const struct ipcbtable ipcb_table3_15;
extern const struct ipcbtable ipcb_table3_16;
extern const struct ipcbtable ipcb_table3_18;
extern const struct ipcbtable ipcb_table3_20;
extern const struct ipcbtable ipcb_table3_22;

