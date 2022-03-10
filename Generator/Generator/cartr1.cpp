/*    
    Copyright 2013-2022 Onera.

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

// Grille cartesienne avec facteurs d'expansion

#include "generator.h"

using namespace std;
using namespace K_FLD;
using namespace K_FUNC; 

// ============================================================================
/* Create a cartesian mesh of nixnjxnk points 
   IN: x0, y0, z0: origine de la grille
   IN: hi, hj, hk: pas de la grille 
   IN: ni, nj, nk: nombre de points
   IN: ri, rj, rk: facteur d'expansion dans chaque direction
   OUT: array definissant le maillage cree. */
// ============================================================================
PyObject* K_GENERATOR::cartr1(PyObject* self, PyObject* args)
{
  E_Int ni, nj, nk;
  E_Float xo, yo, zo;
  E_Float hi, hj, hk;
  E_Float ri, rj, rk;
  E_Int api = 1;
  if (!PYPARSETUPLE(args, 
                    "(ddd)(ddd)(lll)(ddd)l", "(ddd)(ddd)(iii)(ddd)i", 
                    "(fff)(fff)(lll)(fff)l", "(fff)(fff)(iii)(fff)i",
                    &xo, &yo, &zo, &hi, &hj, &hk, &ni, &nj, &nk, &ri, &rj, &rk, &api))
  {
    return NULL;
  }
  if (ni < 1 || nj < 1 || nk < 1)
  {
    PyErr_SetString(PyExc_ValueError, 
                    "cart: ni, nj, nk must be >= 1.");
    return NULL;
  }

  E_Int i, j, k, ind;
  // Create cartesian mesh
  PyObject* tpl;
  tpl = K_ARRAY::buildArray2(3, "x,y,z", ni, nj, nk, api);
  
  K_FLD::FldArrayF* f; K_FLD::FldArrayI* c;
  char* varString; char* eltType;
  K_ARRAY::getFromArray2(tpl, varString, f, ni, nj, nk, c, eltType);

  E_Int nij = ni*nj;
  E_Int nijk = ni*nj*nk;

  E_Float* xt = f->begin(1);
  E_Float* yt = f->begin(2);
  E_Float* zt = f->begin(3);
  

  if (ri != 1.0)
  {
    #pragma omp parallel for default(shared) private(k,j,i,ind)
    for (ind = 0; ind < nijk; ind++)
    {
      k = ind/nij;
      j = (ind-k*nij)/ni;
      i = ind-j*ni-k*nij;
      xt[ind] = xo + hi * ( ( -1 + pow(ri ,i) ) / (-1 + ri) );
    } 
  }
  else
  {
    #pragma omp parallel for default(shared) private(k,j,i,ind)
    for (ind = 0; ind < nijk; ind++)
    {
      k = ind/nij;
      j = (ind-k*nij)/ni;
      i = ind-j*ni-k*nij;
      xt[ind] = xo + hi * i ;
    } 
        
    }   
    
  if (rj != 1.0)
    {
      #pragma omp parallel for default(shared) private(k,j,i,ind)
      for (ind = 0; ind < nijk; ind++)
      {
        k = ind/nij;
        j = (ind-k*nij)/ni;
        i = ind-j*ni-k*nij;
        yt[ind] = yo + hj * ( ( -1 + pow(rj ,j) ) / (-1 + rj) );
      }
        
    }
    else
    {
      #pragma omp parallel for default(shared) private(k,j,i,ind)
      for (ind = 0; ind < nijk; ind++)
      {
        k = ind/nij;
        j = (ind-k*nij)/ni;
        i = ind-j*ni-k*nij;
        yt[ind] = yo + hj * j ;
      }
        
    } 

    if (rk != 1.0)
    { 
      #pragma omp parallel for default(shared) private(k,j,i,ind)
      for (ind = 0; ind < nijk; ind++)
      {
        k = ind/nij;
        j = (ind-k*nij)/ni;
        i = ind-j*ni-k*nij;
        zt[ind] = zo + hk * ( ( -1 + pow(rk ,k) ) / (-1 + rk) );
      }
        
    }
    else
    {
      #pragma omp parallel for default(shared) private(k,j,i,ind)
      for (ind = 0; ind < nijk; ind++)
      {
        k = ind/nij;
        j = (ind-k*nij)/ni;
        i = ind-j*ni-k*nij;
        zt[ind] = zo + hk * k ;
      }
        
    }

  // Return array
  RELEASESHAREDS(tpl, f);
  return tpl;
}
