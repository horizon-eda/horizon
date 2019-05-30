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

#include <footag/ipc7351b.h>
void footag_setref_ipc7351b(
        struct footag_spec *s,
        const struct ipcb_ref *ref
)
{
        s->ref.doc   = "IPC-7351B";
        s->ref.where = ref->where;
        s->ref.what  = ref->what;
}

#include <footag/ipc7251draft1.h>
void footag_setref_ipc7251draft1(
        struct footag_spec *s,
        const struct ipc7251_ref *ref
)
{
        s->ref.doc   = "IPC-7251 1st Working Draft - June 2008";
        s->ref.where = ref->where;
        s->ref.what  = ref->what;
}

