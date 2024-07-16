/*    
    Copyright 2013-2024 Onera.

    This file is part of Cassiopee.

    Cassiopee is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Cassiopee is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Cassiopee.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef _COMMON_XCORE_H
#define _COMMON_XCORE_H

#include "float.h"
#include "xcore.h"

#define EXIT \
  do { \
    MPI_Finalize(); \
    exit(0); \
  } while (0);

#define RAISE(error) PyErr_SetString(PyExc_ValueError, (error))

#define IntArray(n) (Int *)XCALLOC(n, sizeof(Int))
#define FloatArray(n) (Float *)XCALLOC(n, sizeof(Float))

#define E_FLOAT_MAX FLT_MAX
#define E_FLOAT_MIN -FLT_MAX

void merr(const char *fmt, ...);

typedef E_Int Int;
typedef E_Float Float;

void parray(Int *arr, Int n);

inline Int Get_pos(Int e, Int *pn, Int size)
{
    for (E_Int i = 0; i < size; i++) {
        if (pn[i] == e) return i;
    }
    
    assert(0);
    
    return -1;
}

inline void Right_shift(E_Int *pn, E_Int pos, E_Int size)
{
    E_Int tmp[24];
    assert(size <= 24);
    for (E_Int i = 0; i < size; i++) tmp[i] = pn[i];
    for (E_Int i = 0; i < size; i++) pn[i] = tmp[(i+pos)%size];
}

#endif