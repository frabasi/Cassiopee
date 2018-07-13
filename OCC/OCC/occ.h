/*    
    Copyright 2013-2018 Onera.

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
#ifndef _OCC_OCC_H_
#define _OCC_OCC_H_

#include "kcore.h"

namespace K_OCC
{
  PyObject* convertIGES2Arrays(PyObject* self, PyObject* args);
  
  E_Int CADread(
  char* file, char* fimeFmt, E_Float h, E_Float chordal_err, char*& varString,
  std::vector<K_FLD::FldArrayF*>& unstructField,
  std::vector<K_FLD::FldArrayI*>& connect,
  std::vector<E_Int>& eltType,
  std::vector<char*>& zoneNames);
}

#endif
