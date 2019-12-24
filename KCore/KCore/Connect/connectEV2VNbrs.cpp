/*    
    Copyright 2013-2020 Onera.

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
#include "Connect/connect.h"
#include <algorithm>

using namespace K_FLD;
using namespace std;

//=============================================================================
/*
  Change a Elts-Vertex connectivity to a Vertex-Vertex neighbours connectivity
  IN: cEV: Elts-Vertex connectivity. For each elt, give vertices index.
  IN: corners: 0: prend les vertex voisins de V par un edge, 
  IN: corners: 1: prend les vertex de tous les elements voisins de V.
  OUT: cVN: Vertex-Vertex neighbours connectivity. For each vertex, 
  give the indices of its neighbours.
  Les index des vertex commence a 1.
  Attention: cVN doit deja etre alloue au nombre de noeuds.
*/
//=============================================================================
void K_CONNECT::connectEV2VNbrs(FldArrayI& cEV,
                                vector< vector<E_Int> >& cVN, E_Int corners)
{
  E_Int ne = cEV.getSize();
  E_Int ns = cEV.getNfld();
  E_Int nv = cVN.size();
  E_Int vertex1, vertex2;

  if (corners == 0)
  {
    for (E_Int i = 0; i < ne; i++)
    {
      for (E_Int j = 1; j < ns; j++)
      {
        vertex1 = cEV(i, j);
        vertex2 = cEV(i, j+1);
        cVN[vertex1-1].push_back(vertex2);
        cVN[vertex2-1].push_back(vertex1);
      }
      cVN[cEV(i, ns)-1].push_back(cEV(i, 1));
      cVN[cEV(i, 1)-1].push_back(cEV(i, ns));
    }
  }
  else // corners = 1
  {
    for (E_Int i = 0; i < ne; i++)
    { 
      for (E_Int j = 1; j <= ns; j++)
      {
        vertex1 = cEV(i, j);
        for (E_Int j2 = 1; j2 <= ns; j2++)
        {
          vertex2 = cEV(i, j2);
          if (vertex2 != vertex1) cVN[vertex1-1].push_back(vertex2);
        }
      }
    }
  }

  for (E_Int i = 0; i < nv; i++)
  {
    sort(cVN[i].begin(), cVN[i].end());
    cVN[i].erase(unique(cVN[i].begin(), cVN[i].end()), cVN[i].end());
  }
}
