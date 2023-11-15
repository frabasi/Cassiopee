/*    
    Copyright 2013-2023 Onera.

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
#include "Array/Array.h"
#include "String/kstring.h"
#include <stdio.h>
#include <string.h>

using namespace K_FLD;
using namespace std;

//=============================================================================
/* Build an empty structured array3
   IN: nfld: nbre de champs
   IN: varString: variables string
   IN: ni,nj,nk: number of points in field
   IN: api (1: array, 2: array2, 3: array3)
   OUT: PyObject created. */
//=============================================================================
PyObject* K_ARRAY::buildArray3(E_Int nfld, const char* varString, 
                               E_Int ni, E_Int nj, E_Int nk, E_Int api)
{
    PyObject* tpl;
    IMPORTNUMPY;

    if (api == 1) // Array1
    {
        npy_intp dim[2];
        dim[1] = ni*nj*nk;
        dim[0] = nfld;
        PyArrayObject* a = (PyArrayObject*)PyArray_SimpleNew(2, dim, NPY_DOUBLE);
        tpl = Py_BuildValue("[sOlll]", varString, a, (long)ni, (long)nj, (long)nk);
        Py_DECREF(a);
    }
    else // Array2 ou Array3
    {
        npy_intp dim[3]; int ndim=3;
        dim[0] = ni; dim[1] = nj; dim[2] = nk;
        if (nk == 1) ndim--;
        if (nj == 1) ndim--;
        PyObject* rake = PyList_New(0);
        for (E_Int n=0; n < nfld; n++)
        {
            PyArrayObject* a = (PyArrayObject*)PyArray_EMPTY(3, dim, NPY_DOUBLE, 1);
            PyList_Append(rake, (PyObject*)a); Py_DECREF(a);
        }
        tpl = Py_BuildValue("[sOlll]", varString, rake, (long)ni, (long)nj, (long)nk);
        Py_DECREF(rake);
    }
    return tpl;
}

//=============================================================================
/* Build an empty NGON array 
   IN: nfld: number of fields
   IN: varString: variable string
   IN: nvertex: number of vertex

   IN: nelt: number total of elements
   IN: etString: NGON ou NGON*
   IN: center: set to true if field is localised in the centers of
   elements, otherwise let it to false.
   IN: sizeNGon, sizeNFace, nface: connectivity size.
   if sizeNFace == -1, NFACE is not created
   IN: ngonType=1 ou 2 (CGNSv3), ngonType=3 (CGNSv4)
   IN: api=1 (array1, ngonType=1), api=2 (array2, ngonType=2 ou 3), 
   api=3 (array3, ngonType=2 ou 3)
   OUT: PyObject created. */
//=============================================================================
// build pour les NGONS
PyObject* K_ARRAY::buildArray3(E_Int nfld, const char* varString,
                               E_Int nvertex, E_Int nelt, E_Int nface,
                               const char* etString,
                               E_Int sizeNGon, E_Int sizeNFace, E_Int ngonType,  
                               E_Boolean center, E_Int api)
{
    npy_intp dim[2];
    PyObject* a; PyObject* ac; PyObject* tpl;
    char eltType[12];

    // taille de f
    E_Int fSize;
    if (center == true) fSize = nelt;
    else fSize = nvertex;

    IMPORTNUMPY;

    // element string - ajoute * pour les centres
    strcpy(eltType, etString);
    E_Int pos = strlen(eltType)-1;
    pos = 0;
    if (eltType[pos] != '*' && center == true) strcat(eltType, "*");
    else if (eltType[pos] == '*') eltType[pos] = '\0';

    // Build array of fields
    if (api == 1) // Array1
    { 
        dim[1] = fSize; dim[0] = nfld;
        a = PyArray_SimpleNew(2, dim, NPY_DOUBLE);
    }
    else // Array2 or Array3
    {
        dim[0] = fSize;
        a = PyList_New(0);
        for (E_Int n=0; n < nfld; n++)
        {
            PyArrayObject* ar = (PyArrayObject*)PyArray_EMPTY(1, dim, NPY_DOUBLE, 1);
            PyList_Append(a, (PyObject*)ar); Py_DECREF(ar);
        }
    } 

    // Build array for connectivity
    if (api == 1) // Array 1
    {
        dim[1] = 4+sizeNGon+sizeNFace; dim[0] = 1;
        ac = PyArray_SimpleNew(2, dim, E_NPY_INT);
        E_Int* data = (E_Int*)PyArray_DATA((PyArrayObject*)ac);
        data[0] = nface;
        data[1] = sizeNGon;
        data[sizeNGon+2] = nelt;
        data[sizeNGon+3] = sizeNFace;
    }
    else if (ngonType == 2) // Array2/3 - NGonv3 + indir
    {
        ac = PyList_New(0);
        // ngons - NGON - sizeNGon
        dim[0] = sizeNGon;
        PyObject* ar = PyArray_EMPTY(1, dim, E_NPY_INT, 0);
        PyList_Append(ac, (PyObject*)ar); Py_DECREF(ar);
        // ngons - NFACE - sizeNFace
        dim[0] = sizeNFace;
        ar = PyArray_EMPTY(1, dim, E_NPY_INT, 0);
        PyList_Append(ac, (PyObject*)ar); Py_DECREF(ar);
        // ngons - indPG - nfaces
        dim[0] = nface;
        ar = PyArray_EMPTY(1, dim, E_NPY_INT, 0);
        PyList_Append(ac, (PyObject*)ar); Py_DECREF(ar);
        // ngons - indPH - nelts
        dim[0] = nelt;
        ar = PyArray_EMPTY(1, dim, E_NPY_INT, 0);
        PyList_Append(ac, (PyObject*)ar); Py_DECREF(ar);
        // Eventuellement PE - 2*nface
        //dim[0] = nface; dim[1] = 2;
        //PyObject* ar = PyArray_EMPTY(2, dim, E_NPY_INT, 0);
        //PyList_Append(ac, (PyObject*)ar); Py_DECREF(ar);
    }
    else if (ngonType == 3) // array3 - NGONv4
    {
        ac = PyList_New(0);
        // NGON - sizeNGon
        dim[0] = sizeNGon;
        PyObject* ar = PyArray_EMPTY(1, dim, E_NPY_INT, 0);
        PyList_Append(ac, (PyObject*)ar); Py_DECREF(ar);
        // NFACE - sizeNFace
        dim[0] = sizeNFace;
        ar = PyArray_EMPTY(1, dim, E_NPY_INT, 0);
        PyList_Append(ac, (PyObject*)ar); Py_DECREF(ar);
        // NGON - StartOffset
        dim[0] = nface+1;
        ar = PyArray_EMPTY(1, dim, E_NPY_INT, 0);
        E_Int* pt = (E_Int*)PyArray_DATA((PyArrayObject*)ar);
        pt[nface-1] = sizeNGon; 
        PyList_Append(ac, (PyObject*)ar); Py_DECREF(ar);
        // NFACE - startOffset
        dim[0] = nelt+1;
        ar = PyArray_EMPTY(1, dim, E_NPY_INT, 0);
        PyList_Append(ac, (PyObject*)ar); Py_DECREF(ar);
        // Eventuellement PE - 2*nface
        //dim[0] = nface; dim[1] = 2;
        //PyObject* ar = PyArray_EMPTY(2, dim, E_NPY_INT, 0);
        //PyList_Append(ac, (PyObject*)ar); Py_DECREF(ar);
    }
    else
    {
        printf("Warning: buildArray3: invalid api/ngonType. Array not built.\n");
        return NULL;
    }
    tpl = Py_BuildValue("[sOOs]", varString, a, ac, eltType);
    Py_DECREF(a); Py_DECREF(ac);

    return tpl;
}

//=============================================================================
/* Build an empty BE array 
   IN: nfld: number of fields
   IN: varString: variable string
   IN: nvertex: number of vertex
   IN: nelt: number of elements
   IN: etString: "TRI" ou avec *
   IN: center: set to true if field is localised in the centers of
   elements, otherwise let it to false.
   IN: api=1 (array1), api=2 ou 3 (array2 ou 3)

   OUT: PyObject created. */
//=============================================================================
// build pour les single Element (BE)
PyObject* K_ARRAY::buildArray3(E_Int nfld, const char* varString,
                               E_Int nvertex,
                               E_Int nelts,
                               const char* etString,
                               E_Boolean center, E_Int api)
{
    npy_intp dim[2];
    PyObject* a; PyObject* ac; PyObject* tpl;
    char eltType[256];
    strcpy(eltType, etString);

    // taille de f
    E_Int fSize;
    if (center == true) fSize = nelts;
    else fSize = nvertex;

    IMPORTNUMPY;

    // Build array of fields
    if (api == 1) // Array1
    { 
        dim[1] = fSize; dim[0] = nfld;
        a = PyArray_SimpleNew(2, dim, NPY_DOUBLE);
    }
    else // Array2 or Array3
    {
        dim[0] = fSize;
        a = PyList_New(0);
        for (E_Int n=0; n < nfld; n++)
        {
            PyArrayObject* ar = (PyArrayObject*)PyArray_EMPTY(1, dim, NPY_DOUBLE, 1);
            PyList_Append(a, (PyObject*)ar); Py_DECREF(ar);
        }
    } 

    // Connectivite
    if (api == 1) // Array1
    {
        E_Int cSize = nelts;
        char st[256]; E_Int dummy; E_Int nvpe;
        eltString2TypeId(eltType, st, nvpe, dummy, dummy);
        dim[1] = cSize; dim[0] = nvpe;
        ac = PyArray_SimpleNew(2, dim, E_NPY_INT);
    }
    else if (api == 2 || api == 3) // Array2 ou 3
    {
        E_Int cSize = nelts;
        char st[256]; E_Int dummy; E_Int nvpe;
        eltString2TypeId(eltType, st, nvpe, dummy, dummy);
        ac = PyList_New(0);
        dim[0] = cSize; dim[1] = nvpe;
        PyObject* ar = PyArray_EMPTY(2, dim, E_NPY_INT, 0);
        PyList_Append(ac, (PyObject*)ar); Py_DECREF(ar);
    }
    else
    {
        PyErr_SetString(PyExc_TypeError, "buildArray: unkown api.");
        return NULL;
    }

    tpl = Py_BuildValue("[sOOs]", varString, a, ac, eltType);
    Py_DECREF(a); Py_DECREF(ac);

    return tpl;
}

//=============================================================================
/* Build an empty ME array 
   IN: nfld: number of fields
   IN: varString: variable string
   IN: nvertex: number of vertex
   IN: neltsPerConnect: number of elements for each connect
   IN: etString: "TRI,QUAD" ou avec *
   IN: center: set to true if field is localised in the centers of
   elements, otherwise let it to false.
   IN: api=1 (array1, single connect), api=2 (array2, single connect), 
   api=3 (array3, all connects)

   OUT: PyObject created. */
//=============================================================================
// build pour les Multiple Element (ME)
PyObject* K_ARRAY::buildArray3(E_Int nfld, const char* varString,
                               E_Int nvertex,
                               std::vector<E_Int>& neltsPerConnect,
                               const char* etString,
                               E_Boolean center, E_Int api)
{
    npy_intp dim[2];
    PyObject* a; PyObject* ac; PyObject* tpl;
    char eltType[256];
    strcpy(eltType, etString);

    // taille de f
    E_Int nelt = 0;
    for (size_t i = 0; i < neltsPerConnect.size(); i++) nelt += neltsPerConnect[i];
    E_Int fSize;
    if (center == true) fSize = nelt;
    else fSize = nvertex;

    IMPORTNUMPY;

    // Build array of fields
    if (api == 1) // Array1
    { 
        dim[1] = fSize; dim[0] = nfld;
        a = PyArray_SimpleNew(2, dim, NPY_DOUBLE);
    }
    else // Array2 or Array3
    {
        dim[0] = fSize;
        a = PyList_New(0);
        for (E_Int n=0; n < nfld; n++)
        {
            PyArrayObject* ar = (PyArrayObject*)PyArray_EMPTY(1, dim, NPY_DOUBLE, 1);
            PyList_Append(a, (PyObject*)ar); Py_DECREF(ar);
        }
    } 

    // Connectivite
    if (api == 1) // Array1 - force single connect
    {
        E_Int cSize = nelt;
        char st[256]; E_Int dummy; E_Int nvpe;
        eltString2TypeId(eltType, st, nvpe, dummy, dummy);
        dim[1] = cSize; dim[0] = nvpe;
        ac = PyArray_SimpleNew(2, dim, E_NPY_INT);
    }
    else if (api == 2) // Array2 - force single connect
    {
        E_Int cSize = nelt;
        char st[256]; E_Int dummy; E_Int nvpe;
        eltString2TypeId(eltType, st, nvpe, dummy, dummy);
        ac = PyList_New(0);
        dim[0] = cSize; dim[1] = nvpe;
        PyObject* ar = PyArray_EMPTY(2, dim, E_NPY_INT, 0);
        PyList_Append(ac, (PyObject*)ar); Py_DECREF(ar);
    }
    else // Array3
    {
        std::vector<char*> eltTypes;
        K_ARRAY::extractVars(eltType, eltTypes);
        char st[256]; E_Int dummy; E_Int nvpe;
        ac = PyList_New(0);
        //printf("size=%d %s\n", eltTypes.size(), eltType);
        for (size_t i = 0; i < eltTypes.size(); i++)
        {
            E_Int cSize = neltsPerConnect[i];
            eltString2TypeId(eltTypes[i], st, nvpe, dummy, dummy);
            dim[0] = cSize; dim[1] = nvpe;
            PyObject* ar = PyArray_EMPTY(2, dim, E_NPY_INT, 0);
            PyList_Append(ac, (PyObject*)ar); Py_DECREF(ar);
        }
        for (size_t i = 0; i < eltTypes.size(); i++) delete [] eltTypes[i];
    }

    tpl = Py_BuildValue("[sOOs]", varString, a, ac, eltType);
    Py_DECREF(a); Py_DECREF(ac);

    return tpl;
}

// Build an array identical to f and c in size (unstructured only)
// but with nfld vars. Center and api can be changed.
// This doesnt perform copy
PyObject* K_ARRAY::buildArray3(E_Int nfld,
                               const char* varString,
                               FldArrayF& f,
                               FldArrayI& cn,
                               char* eltType,
                               E_Int center, E_Int api)
{
    PyObject* tpl = NULL;
    E_Int npts = f.getSize();
    if (api == -1) // if not given, find api from f
    {
        api = f.getApi();
        if (api == 2) api = 3;
    }
    if (center == -1) // find center from eltstring
    { 
        center = 0;
        E_Int l = strlen(eltType);
        if (eltType[l-2] == '*') center = 1;
    }
    if (strcmp(eltType, "NGON") == 0 || strcmp(eltType, "NGON*") == 0)
    {
        E_Int ngonType = cn.isNGon();
        E_Int nelts = cn.getNElts();
        E_Int nfaces = cn.getNFaces();
        E_Int sizeNGon = cn.getSizeNGon();
        E_Int sizeNFace = cn.getSizeNFace();
        tpl = K_ARRAY::buildArray3(nfld, varString, npts, nelts, nfaces, 
        eltType, sizeNGon, sizeNFace, ngonType, center, api);
    }
    else
    {
        E_Int ncon = cn.getNConnect();
        vector< E_Int > neltsPerConnect(ncon);
        E_Int nelts = 0;
        for (E_Int i = 0; i < ncon; i++)
        { FldArrayI& cm = *(cn.getConnect(i));
        neltsPerConnect[i] = cm.getSize(); 
        nelts += cm.getSize(); }
        tpl = K_ARRAY::buildArray3(nfld, varString, npts, neltsPerConnect, eltType, center, api);
    }
    return tpl;
}

// Copy from f and cn unstructured
PyObject* K_ARRAY::buildArray3(FldArrayF& f,
                               const char* varString,
                               FldArrayI& cn,
                               const char* eltType,
                               E_Int api)
{
  if (api == -1) { api = f.getApi(); }
  if (api == 2) api = 3;
  E_Int nfld = f.getNfld(); E_Int npts = f.getSize();
  if (strcmp(eltType, "NGON") == 0 || strcmp(eltType, "NGON*") == 0)
  {
    E_Int dim;
    E_Int ngonType = cn.isNGon();
    E_Int nelts = cn.getNElts();
    E_Int nfaces = cn.getNFaces();
    E_Int sizeNGon = cn.getSizeNGon();
    E_Int sizeNFace = cn.getSizeNFace();
    E_Boolean center = false;
    E_Int l = strlen(eltType);
    if (eltType[l-2] == '*') center = true;
    
    PyObject* tpl = K_ARRAY::buildArray3(nfld, varString, npts, nelts, nfaces, 
        eltType, sizeNGon, sizeNFace, ngonType, center, api);
    FldArrayF* f2; FldArrayI* cn2;
    K_ARRAY::getFromArray3(tpl, f2, cn2);

    if (center == true) dim = nelts;
    else dim = npts;
    #pragma omp parallel
    {
      for (E_Int n = 1; n <= nfld; n++)
      {
        E_Float* fp = f.begin(n);
        E_Float* f2p = f2->begin(n);
        #pragma omp for
        for (E_Int i = 0; i < dim; i++) f2p[i] = fp[i];
      }
      E_Int* ngonp = cn.getNGon();
      E_Int* ngon2p = cn2->getNGon();
      #pragma omp for
      for (E_Int i = 0; i < sizeNGon; i++) ngon2p[i] = ngonp[i];
      E_Int* nfacep = cn.getNFace();
      E_Int* nface2p = cn2->getNFace();
      #pragma omp for
      for (E_Int i = 0; i < sizeNFace; i++) nface2p[i] = nfacep[i];
      if (api > 1)
      {
        E_Int* indPGp = cn.getIndPG();
        E_Int* indPG2p = cn2->getIndPG();
        E_Int dim2;
        if (ngonType == 2) dim2 = nfaces;
        else dim2 = nfaces+1;
        #pragma omp for
        for (E_Int i = 0; i < dim2; i++) indPG2p[i] = indPGp[i];
        E_Int* indPHp = cn.getIndPH();
        E_Int* indPH2p = cn2->getIndPH();
        if (ngonType == 2) dim2 = nelts;
        else dim2 = nelts+1;
        #pragma omp for
        for (E_Int i = 0; i < dim2; i++) indPH2p[i] = indPHp[i];
      }
    }
    return tpl;
  }
  else // BE
  {
    E_Int ncon = cn.getNConnect();
    E_Boolean center = false;
    E_Int l = strlen(eltType);
    if (eltType[l-2] == '*') center = true;
    vector< E_Int > neltsPerConnect(ncon);
    E_Int nelts = 0;
    for (E_Int i = 0; i < ncon; i++)
    { FldArrayI& cm = *(cn.getConnect(i));
      neltsPerConnect[i] = cm.getSize(); 
      nelts += cm.getSize(); }
    PyObject* tpl = K_ARRAY::buildArray3(nfld, varString, npts, neltsPerConnect, eltType, center, api);
    FldArrayF* f2; FldArrayI* cn2;  
    K_ARRAY::getFromArray3(tpl, f2, cn2);
      
    // copie des champs
    E_Int dim;
    if (center == true) dim = nelts;
    else dim = npts;
    #pragma omp parallel
    {
      for (E_Int n = 1; n <= nfld; n++)
      {
        E_Float* fp = f.begin(n);
        E_Float* f2p = f2->begin(n);
        #pragma omp for
        for (E_Int i = 0; i < dim; i++) f2p[i] = fp[i];
      }
      
      for (E_Int i = 0; i < ncon; i++)
      { 
        FldArrayI& cm = *(cn.getConnect(i));
        FldArrayI& cm2 = *(cn2->getConnect(i));
        E_Int* cmp = cm.begin();
        E_Int* cm2p = cm2.begin();
        #pragma omp for
        for (E_Int i = 0; i < cm.getSize()*cm.getNfld(); i++) cm2p[i] = cmp[i];
      }
    }
    return tpl;
  }
}

// Build a copy array from f structured
// if api=-1, keep f api
PyObject* K_ARRAY::buildArray3(FldArrayF& f, const char* varString,
                               E_Int ni, E_Int nj, E_Int nk, E_Int api)
{
  if (api == -1) // copie l'api de f
  { api = f.getApi(); }
  if (api == 2) api = 3;
  E_Int nfld = f.getNfld(); E_Int npts = f.getSize();
  PyObject* tpl = K_ARRAY::buildArray3(nfld, varString, ni, nj, nk, api);
  FldArrayF* f2; FldArrayI* cn2;
  K_ARRAY::getFromArray3(tpl, f2, cn2);

  #pragma omp parallel
  {
    for (E_Int n = 1; n <= nfld; n++)
    {
      E_Float* fp = f.begin(n);
      E_Float* f2p = f2->begin(n);
      #pragma omp for
      for (E_Int i = 0; i < npts; i++) f2p[i] = fp[i];
    }
  }
  return tpl;
}
