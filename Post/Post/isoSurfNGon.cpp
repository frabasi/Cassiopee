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

// Build an isoSurf in a volume NGON mesh using marching tetra

# include <unordered_map>
# include "post.h"
using namespace std;
using namespace K_FLD;

#define VERTEXINTERP(nfld, value, f, poscellN, f0, f1, ind0, ind1, fisos, npt) \
  { E_Float alpha, alpha1, val; E_Float df = f1-f0;              \
    if (K_FUNC::fEqualZero(df) == true) alpha = 1.;              \
    else alpha = (value-f0)/df;                                  \
    alpha1 = 1.-alpha;                                           \
    for (E_Int j = 1; j <= nfld; j++)                            \
     { val = alpha1*f(ind0, j)+alpha*f(ind1, j);             \
      fisos(npt, j) = val; }                                    \
    if (poscellN != 0)                                         \
    { if (f(ind0, poscellN) == 0. || f(ind1, poscellN) == 0.)  \
     fisos(npt, poscellN) = 0.; }                    \
     npt++; }

PyObject* K_POST::isoSurfNGon(PyObject* self, PyObject* args)
{
  PyObject* array;
  char* fieldName; E_Float value;
  if (!PYPARSETUPLEF(args,
                    "Osd", "Osf",
                    &array, &fieldName, &value)) return NULL;

  /*----------------------------------------------*/
  /* Extraction des donnees du maillage volumique */ 
  /*----------------------------------------------*/
  char* varString; char* eltType;
  FldArrayF* f; FldArrayI* cn;
  E_Int nil, njl, nkl;
  E_Int res = 
    K_ARRAY::getFromArray2(array, varString, f, nil, njl, nkl, 
                          cn, eltType);

  if (res != 1 && res != 2)
  {
    PyErr_SetString(PyExc_TypeError,
                    "isoSurf: input array is invalid.");
    return NULL;
  }
  if (res != 2 || strcmp(eltType, "NGON") != 0)
  {
    RELEASESHAREDB(res, array, f, cn);
    PyErr_SetString(PyExc_TypeError,
                    "isoSurf: input array must be NGON.");
    return NULL;
  }
  
  // Check size of array
  E_Int posx = K_ARRAY::isCoordinateXPresent(varString);
  E_Int posy = K_ARRAY::isCoordinateYPresent(varString);
  E_Int posz = K_ARRAY::isCoordinateZPresent(varString);
  if (posx == -1 || posy == -1 || posz == -1)
  {
    PyErr_SetString(PyExc_TypeError,
                    "isoSurf: coordinates not found in array.");
    RELEASESHAREDU(array, f, cn);
    return NULL;
  }
  posx++; posy++; posz++;

  // position de la variable iso
  E_Int posf = K_ARRAY::isNamePresent(fieldName, varString);
  if (posf == -1)
  {
    PyErr_SetString(PyExc_TypeError,
                    "isoSurf: variable doesn't exist in array.");
    RELEASESHAREDU(array, f, cn);
    return NULL;
  }
  posf++;

  E_Int poscellN = K_ARRAY::isCellNatureField2Present(varString)+1;

  FldArrayF fiso; FldArrayI ciso;
  doIsoSurfNGon(*f, *cn, posf, value, poscellN, fiso, ciso);
  RELEASESHAREDU(array, f, cn);
  
  //E_Float tolc = 1.e-12;
  //K_CONNECT::cleanConnectivity(posx, posy, posz, tolc, 
  //                             "TRI", fiso, ciso);

  if (fiso.getSize() == 0 || ciso.getSize() == 0)
  {
    PyErr_SetString(PyExc_ValueError,
                    "isoSurf: isosurf is empty.");
    return NULL;
  }

  PyObject* t = K_ARRAY::buildArray(fiso, varString, ciso, -1, "TRI");
  return t;
}

//=============================================================================
/* 
   IN: f: le champ a iso-surfacer.
   IN: la connectivite de f
   IN: posf: la position de la variable d'isosurface dans f
   IN: value: la valeur de l'isosurface
   IN: posCellN: la position de cellN dans f
   OUT: fiso: le champ de l'iso
   OUT: ciso: la connectivite de l'iso
*/
//==============================================================================
void K_POST::doIsoSurfNGon(FldArrayF& f, FldArrayI& cn, E_Int posf, E_Float value,
                           E_Int poscellN,
                           FldArrayF& fiso, FldArrayI& ciso)
{
  E_Int nfld = f.getNfld();
  E_Int npoints = f.getSize();
  E_Float* fp = f.begin(posf);
  
  E_Int nthreads = __NUMTHREADS__;

  E_Int nelts = cn.getNElts();
  E_Int nfaces = cn.getNFaces();
  printf("nelts=%d, nfaces=%d\n", nelts, nfaces);
  printf("api=%d, ngon=%d\n", cn.getApi(), cn.isNGon());
  printf("nfld=%d\n", cn.getNfld());
  fflush(stdout);
  E_Int* ptrf = cn.getNGon();
  E_Int* ptre = cn.getNFace();

  // Tableau de position des faces dans la connectivite
  E_Int* indPG = cn.getIndPG();

  /*
  printf("indPG\n");
  for (E_Int i = 0; i < nfaces; i++) printf("%d ", indPG[i]);
  printf("\n");
  */

  // Tableau de position des elements dans la connectivite
  E_Int* indPH = cn.getIndPH();
  
  /*
  printf("indPH\n");
  for (E_Int i = 0; i < nelts; i++) printf("%d ", indPH[i]);
  printf("\n");
  */

  // Dimension du NGON
  //FldArrayI dimElts;
  //K_CONNECT::getDimElts(cn, indPG, indPH, dimElts);
  /*
  printf("dimElts\n");
  for (E_Int i = 0; i < dimElts.getSize(); i++) printf("%d ", dimElts[i]);
  printf("\n");
  */
  //E_Int dim = dimElts[0];

#define COMPUTEFF(fa) \
    indFace = pte[fa+1]-1; \
    pf = indPG[indFace]; ptf = ptrf+pf; \
    nbPts = ptf[0]; \
    for (E_Int p = 0; p < nfld; p++) ff[p] = 0.; \
      for (E_Int pt = 0; pt < nbPts; pt++) \
      { ind = ptf[1+pt]-1; \
        for (E_Int p = 0; p < nfld; p++) ff[p] += f(ind,p+1); } \
    for (E_Int p = 0; p < nfld; p++) ff[p] = ff[p]/nbPts;

#define COMPUTEFFS(fa) \
    indFace = pte[fa+1]-1; \
    pf = indPG[indFace]; ptf = ptrf+pf; \
    nbPts = ptf[0]; \
    ffs = 0.; \
    for (E_Int pt = 0; pt < nbPts; pt++) \
    { ind = ptf[1+pt]-1; \
      ffs += fp[ind]; } \
    ffs = ffs/nbPts;

    // Dimensionnement: npts et ntri (par thread*10)
    E_Int* ntris2 = new E_Int [nthreads*10];
    E_Int* npts2 = new E_Int [nthreads*10];
    
#pragma omp parallel default(shared)
    {
      E_Int  ithread = __CURRENT_THREAD__;
      int triindex;
      E_Int np = 0; E_Int ntri = 0;
      E_Int pe, pf, indFace, nbFaces, nbPts, ind, ind2;
      E_Int* ptf; E_Int* pte;
      E_Float f0, f1, f2, f3;
      E_Float ffs, fcs;
      E_Float delta = (nelts*1.)/(nthreads*1.);
      E_Int ieltstart = int(ithread*delta);
      E_Int ieltend = int((ithread+1)*delta);
      //printf("ieltstart = %d , %d chekc=%d\n",ieltstart, ieltend, nelts);
      E_Float deltap = (ieltend-ieltstart)/(10.);
      E_Int elt;

      elt = ieltstart;
      //printf("borne start=%d\n", elt);
      for (E_Int j = 0; j < 10; j++)
      {
        np = 0; ntri = 0;
        //printf("%d %d\n", int((j)*deltap), int((j+1)*deltap));
        for (E_Int k = 0; k < int((j+1)*deltap)-int(j*deltap); k++)
        {
          //printf("%d %d\n", elt, nelts);
          // Construit centre de l'element
          pe = indPH[elt]; pte = ptre+pe;
          nbFaces = pte[0];
          fcs = 0.;
          for (E_Int fa = 0; fa < nbFaces; fa++)
          {
            COMPUTEFFS(fa);
            fcs += ffs;
          }
          fcs = fcs/nbFaces;

          // construit les tetras
          for (E_Int fa = 0; fa < nbFaces; fa++)
          {
            COMPUTEFFS(fa);
            for (E_Int pt = 0; pt < nbPts; pt++)
            {
              ind = ptf[1+pt]-1;
              if (pt == nbPts-1) ind2 = ptf[1]-1;
              else ind2 = ptf[2+pt]-1;
              // tetra = centre, face, pts de la face
              f0 = fcs; f1 = ffs; f2 = fp[ind]; f3 = fp[ind2];
              triindex = 0;
              if (f0 < value) triindex |= 1;
              if (f1 < value) triindex |= 2;
              if (f2 < value) triindex |= 4;
              if (f3 < value) triindex |= 8;
              switch (triindex)
              {
                case 0x00:
                case 0x0F:
                break;

                case 0x0E: // OK
                np += 3; ntri++;
                break;

                case 0x01: // OK
                np += 3; ntri++;
                break;

                case 0x0D: // OK
                np += 3; ntri++;
                break;

                case 0x02: // OK
                np += 3; ntri++;
                break;

                case 0x0C: // OK
                np += 6; ntri += 2;
                break;

                case 0x03:
                np += 6; ntri += 2;
                break;

                case 0x0B: // OK
                np += 3; ntri++;
                break;

                case 0x04: // OK
                np += 3; ntri++;
                break;

                case 0x0A:
                np += 6; ntri += 2;
                break;

                case 0x05:
                np += 6; ntri += 2;
                break;

                case 0x09: // OK
                np += 6; ntri += 2;        
                break;

                case 0x06:
                np += 6; ntri += 2;        
                break;

                case 0x07:
                np += 3; ntri++;
                break;

                case 0x08: // OK
                np += 3; ntri++;
                break;
              }
            }
          }
          elt++;
        }
        npts2[j+ithread*10] = np;
        ntris2[j+ithread*10] = ntri;
      }
      //printf("borne end=%d\n", elt);
    }

    
    for (E_Int i = 0; i < nthreads; i++)
    {
      for (E_Int j = 0; j < 10; j++) printf("thread=%d, j=%d -> %d,%d\n", i,j,npts2[j+i*10],ntris2[j+i*10]);
    }
    printf("dimensionnement...\n");
    fflush(stdout);

    // equilibrage dynamique
    FldArrayI iestart(10*nthreads);
    FldArrayI ieend(10*nthreads);
    E_Float delta = (nelts*1.)/(nthreads*1.);
    for (E_Int i = 0; i < nthreads; i++)
    {
      E_Int ieltstart = int(i*delta);
      E_Int ieltend = int((i+1)*delta);
      E_Float deltap = (ieltend-ieltstart)/(10.);
      for (E_Int j = 0; j < 10; j++)
      {
        iestart[j+10*i] = ieltstart+int(deltap*j);
        ieend[j+10*i] = ieltstart+int(deltap*(j+1));
        //printf("%d %d: %d %d\n",i,j,iestart[j+10*i],ieend[j+10*i]);
      }
    }

    E_Int* istart = new E_Int [nthreads];
    E_Int* iend = new E_Int [nthreads];
    E_Int* ntris = new E_Int [nthreads];
    E_Int* npts = new E_Int [nthreads];
    E_Float alpha = 0.;
    for (E_Int i = 0; i < 10*nthreads; i++) alpha += ntris2[i];
    alpha = alpha/nthreads;
    printf("ntri moyen equil=%d\n", int(alpha));
    fflush(stdout);
    
    istart[0] = 0; E_Int ibold = 0; E_Int ib = 0;
    E_Float plus = 0.;
    for (E_Int i = 0; i < nthreads; i++)
    {
      E_Int nc = 0; E_Int np = 0;
      while (ib < nthreads*10 && nc+plus*ntris2[ib] < int(alpha))
      {
        nc += ntris2[ib];
        np += npts2[ib]; ib++;
      }
      if (plus == 0.) plus = 1.;
      else plus = 0.;
      
      if (i == nthreads-1) // ajoute la fin (si necessaire)
      {
        while (ib < nthreads*10)
        {
          nc += ntris2[ib];
          np += npts2[ib]; ib++; 
        }
      }
      
      ntris[i] = nc; npts[i] = np;
      istart[i] = iestart[ibold];
      iend[i] = ieend[ib-1];
      ibold = ib;
    }
    //iend[nthreads-1] = nelts;
    printf("reequilibrage: nthreads=%d\n", nthreads);
    for (E_Int i = 0; i < nthreads; i++) printf("thread=%d: %d / %d %d\n", i, ntris[i], istart[i], iend[i]);
    fflush(stdout);
    // fin equilibrage dynamique
    delete [] npts2;
    delete [] ntris2;

    FldArrayI** cisos = new FldArrayI* [nthreads];
    for (E_Int i = 0; i < nthreads; i++) cisos[i] = new FldArrayI(ntris[i], 3);
    FldArrayF** fisos = new FldArrayF* [nthreads];
    for (E_Int i = 0; i < nthreads; i++) fisos[i] = new FldArrayF(npts[i],nfld);
    E_Int* prevT = new E_Int [nthreads];
    E_Int* prevF = new E_Int [nthreads];
    
    
#define KEY(p1,p2,e,f) p1+p2+2*npoints*e+(2*npoints*nelts)*f
#define ETK E_Int

    FldArray<ETK>** keys = new FldArray<ETK>* [nthreads];
    for (E_Int i = 0; i < nthreads; i++) keys[i] = new FldArray<ETK>(npts[i],2);

#pragma omp parallel default(shared)
    {
      E_Int ithread = __CURRENT_THREAD__;
      E_Int pe, pf, indFace, nbFaces, nbPts, ind, indp;
      E_Int ind0, ind1, ind2, ind3;
      E_Int* ptf; E_Int* pte;
      FldArrayF fco(nfld); E_Float* fc = fco.begin();
      FldArrayF ffo(nfld); E_Float* ff = ffo.begin();
      FldArrayF ffp(4,nfld);
      E_Float f0, f1, f2, f3;
      int triindex;

      E_Int ntri = 0; // nombre de tri dans l'iso
      E_Int npt = 0; // nombre de pts dans l'iso

      FldArrayI& cisop = *cisos[ithread];
      E_Int* ciso1 = cisop.begin(1);
      E_Int* ciso2 = cisop.begin(2);
      E_Int* ciso3 = cisop.begin(3);
      FldArrayF& fisop = *fisos[ithread];

      ETK* key = keys[ithread]->begin();
      ETK* key2 = keys[ithread]->begin(2);

      for (E_Int elt = istart[ithread]; elt < iend[ithread]; elt++)
      {
        // Construit centre de l'element
        pe = indPH[elt]; pte = ptre+pe;
        nbFaces = pte[0];
        for (E_Int p = 0; p < nfld; p++) fc[p] = 0.;
        for (E_Int fa = 0; fa < nbFaces; fa++)
        {
          COMPUTEFF(fa);
          for (E_Int p = 0; p < nfld; p++) fc[p] += ff[p];
        }
        for (E_Int p = 0; p < nfld; p++) fc[p] = fc[p]/nbFaces;

        // construit les tetras
        for (E_Int fa = 0; fa < nbFaces; fa++)
        {
          COMPUTEFF(fa);
          for (E_Int pt = 0; pt < nbPts; pt++)
          {
            ind = ptf[1+pt]-1;
            if (pt == nbPts-1) indp = ptf[1]-1;
            else indp = ptf[2+pt]-1;
            // tetra = centre, face, pts de la face
            f0 = fc[posf-1]; f1 = ff[posf-1]; f2 = fp[ind]; f3 = fp[indp];
            for (E_Int n = 0; n < nfld; n++) ffp(0,n+1) = fc[n];
            for (E_Int n = 0; n < nfld; n++) ffp(1,n+1) = ff[n];
            for (E_Int n = 0; n < nfld; n++) ffp(2,n+1) = f(ind,n+1);
            for (E_Int n = 0; n < nfld; n++) ffp(3,n+1) = f(indp,n+1);
            ind0 = 0; ind1 = 1; ind2 = 2; ind3 = 3;
              
            triindex = 0;
            if (f0 < value) triindex |= 1;
            if (f1 < value) triindex |= 2;
            if (f2 < value) triindex |= 4;
            if (f3 < value) triindex |= 8;

            switch (triindex)
            {
              case 0x00:
              case 0x0F:
              break;

              case 0x0E: // OK
              VERTEXINTERP(nfld, value, ffp, poscellN,
                           f0, f1, ind0, ind1,
                           fisop, npt);
              VERTEXINTERP(nfld, value, ffp, poscellN,
                           f0, f2, ind0, ind2,
                           fisop, npt);
              VERTEXINTERP(nfld, value, ffp, poscellN,
                           f0, f3, ind0, ind3,
                           fisop, npt);
              ciso1[ntri] = npt;
              ciso2[ntri] = npt-1;
              ciso3[ntri] = npt-2;
              ntri++;
              key[npt-3] = KEY(-1,-1,elt,indFace);
              key[npt-2] = KEY(ind,-1,elt,-1);
              key[npt-1] = KEY(indp,-1,elt,-1);
              key2[npt-3] = 0;
              key2[npt-2] = 1;
              key2[npt-1] = 2;
              break;

              case 0x01: // OK
              VERTEXINTERP(nfld, value, ffp, poscellN,
                           f0, f1, ind0, ind1,
                           fisop, npt);
              VERTEXINTERP(nfld, value, ffp, poscellN,
                           f0, f2, ind0, ind2,
                           fisop, npt);
              VERTEXINTERP(nfld, value, ffp, poscellN,
                           f0, f3, ind0, ind3,
                           fisop, npt);
              ciso1[ntri] = npt-2;
              ciso2[ntri] = npt-1;
              ciso3[ntri] = npt;
              ntri++;
              key[npt-3] = KEY(-1,-1,elt,indFace);
              key[npt-2] = KEY(ind,-1,elt,-1);
              key[npt-1] = KEY(indp,-1,elt,-1);
              key2[npt-3] = 3;
              key2[npt-2] = 4;
              key2[npt-1] = 5;
              break;

              case 0x0D: // OK
              VERTEXINTERP(nfld, value, ffp, poscellN,
                           f1, f0, ind1, ind0,
                           fisop, npt);
              VERTEXINTERP(nfld, value, ffp, poscellN,
                           f1, f3, ind1, ind3,
                           fisop, npt);
              VERTEXINTERP(nfld, value, ffp, poscellN,
                           f1, f2, ind1, ind2,
                           fisop, npt);
              ciso1[ntri] = npt;
              ciso2[ntri] = npt-1;
              ciso3[ntri] = npt-2;
              ntri++;
              key[npt-3] = KEY(-1,-1,elt,indFace);
              key[npt-2] = KEY(indp,-1,-1,indFace);
              key[npt-1] = KEY(ind,-1,-1,indFace);
              key2[npt-3] = 6;
              key2[npt-2] = 7;
              key2[npt-1] = 8;
              break;

              case 0x02: // OK
              VERTEXINTERP(nfld, value, ffp, poscellN,
                           f1, f0, ind1, ind0,
                           fisop, npt);
              VERTEXINTERP(nfld, value, ffp, poscellN,
                           f1, f3, ind1, ind3,
                           fisop, npt);
              VERTEXINTERP(nfld, value, ffp, poscellN,
                           f1, f2, ind1, ind2,
                           fisop, npt);
              ciso1[ntri] = npt-2;
              ciso2[ntri] = npt-1;
              ciso3[ntri] = npt;
              ntri++;
              key[npt-3] = KEY(-1,-1,elt,indFace);
              key[npt-2] = KEY(indp,-1,-1,indFace);
              key[npt-1] = KEY(ind,-1,-1,indFace);
              key2[npt-3] = 9;
              key2[npt-2] = 10;
              key2[npt-1] = 11;
              break;

              case 0x0C: // OK
              VERTEXINTERP(nfld, value, ffp, poscellN,
                           f0, f3, ind0, ind3,
                           fisop, npt);
              VERTEXINTERP(nfld, value, ffp, poscellN,
                           f0, f2, ind0, ind2,
                           fisop, npt);
              VERTEXINTERP(nfld, value, ffp, poscellN,
                           f1, f3, ind1, ind3,
                           fisop, npt);
              ciso1[ntri] = npt-2;
              ciso2[ntri] = npt-1;
              ciso3[ntri] = npt;
              ntri++;
              key[npt-3] = KEY(indp,-1,elt,-1);
              key[npt-2] = KEY(ind,-1,elt,-1);
              key[npt-1] = KEY(indp,-1,-1,indFace);
              key2[npt-3] = 12;
              key2[npt-2] = 13;
              key2[npt-1] = 14;
              
              VERTEXINTERP(nfld, value, ffp, poscellN,
                           f1, f3, ind1, ind3,
                           fisop, npt);
              VERTEXINTERP(nfld, value, ffp, poscellN,
                           f1, f2, ind1, ind2,
                           fisop, npt);
              VERTEXINTERP(nfld, value, ffp, poscellN,
                           f0, f2, ind0, ind2,
                           fisop, npt);
              ciso1[ntri] = npt;
              ciso2[ntri] = npt-1;
              ciso3[ntri] = npt-2;
              ntri++;
              key[npt-3] = KEY(indp,-1,-1,indFace);
              key[npt-2] = KEY(ind,-1,-1,indFace);
              key[npt-1] = KEY(ind,-1,elt,-1);
              key2[npt-3] = 15;
              key2[npt-2] = 16;
              key2[npt-1] = 17;
              break;

              case 0x03:
              VERTEXINTERP(nfld, value, ffp, poscellN,
                           f0, f3, ind0, ind3,
                           fisop, npt);
              VERTEXINTERP(nfld, value, ffp, poscellN,
                           f0, f2, ind0, ind2,
                           fisop, npt);
              VERTEXINTERP(nfld, value, ffp, poscellN,
                           f1, f3, ind1, ind3,
                           fisop, npt);
              ciso1[ntri] = npt-2;
              ciso2[ntri] = npt-1;
              ciso3[ntri] = npt;
              ntri++;
              key[npt-3] = KEY(indp,-1,elt,-1);
              key[npt-2] = KEY(ind,-1,elt,-1);
              key[npt-1] = KEY(indp,-1,-1,indFace);
              key2[npt-3] = 18;
              key2[npt-2] = 19;
              key2[npt-1] = 20;
              
              VERTEXINTERP(nfld, value, ffp, poscellN,
                           f1, f3, ind1, ind3,
                           fisop, npt);
              VERTEXINTERP(nfld, value, ffp, poscellN,
                           f1, f2, ind1, ind2,
                           fisop, npt);
              VERTEXINTERP(nfld, value, ffp, poscellN,
                           f0, f2, ind0, ind2,
                           fisop, npt);
              ciso1[ntri] = npt-2;
              ciso2[ntri] = npt-1;
              ciso3[ntri] = npt;
              ntri++;
              key[npt-3] = KEY(indp,-1,-1,indFace);
              key[npt-2] = KEY(ind,-1,-1,indFace);
              key[npt-1] = KEY(ind,-1,elt,-1);
              key2[npt-3] = 21;
              key2[npt-2] = 22;
              key2[npt-1] = 23;
              break;

              case 0x0B: // OK
              VERTEXINTERP(nfld, value, ffp, poscellN,
                           f2, f0, ind2, ind0,
                           fisop, npt);
              VERTEXINTERP(nfld, value, ffp, poscellN,
                           f2, f1, ind2, ind1,
                           fisop, npt);
              VERTEXINTERP(nfld, value, ffp, poscellN,
                           f2, f3, ind2, ind3,
                           fisop, npt);
              ciso1[ntri] = npt;
              ciso2[ntri] = npt-1;
              ciso3[ntri] = npt-2;
              ntri++;
              key[npt-3] = KEY(ind,-1,elt,-1);
              key[npt-2] = KEY(ind,-1,-1,indFace);
              key[npt-1] = KEY(ind,indp,-1,-1);
              key2[npt-3] = 24;
              key2[npt-2] = 25;
              key2[npt-1] = 26;
              break;

              case 0x04: // OK
              VERTEXINTERP(nfld, value, ffp, poscellN,
                           f2, f0, ind2, ind0,
                           fisop, npt);
              VERTEXINTERP(nfld, value, ffp, poscellN,
                           f2, f1, ind2, ind1,
                           fisop, npt);
              VERTEXINTERP(nfld, value, ffp, poscellN,
                           f2, f3, ind2, ind3,
                           fisop, npt);
              ciso1[ntri] = npt-2;
              ciso2[ntri] = npt-1;
              ciso3[ntri] = npt;
              ntri++;
              key[npt-3] = KEY(ind,-1,elt,-1);
              key[npt-2] = KEY(ind,-1,-1,indFace);
              key[npt-1] = KEY(indp,ind,-1,-1);
              key2[npt-3] = 27;
              key2[npt-2] = 28;
              key2[npt-1] = 29;
              break;

              case 0x0A:
              VERTEXINTERP(nfld, value, ffp, poscellN,
                           f0, f1, ind0, ind1,
                           fisop, npt);
              VERTEXINTERP(nfld, value, ffp, poscellN,
                           f2, f3, ind2, ind3,
                           fisop, npt);
              VERTEXINTERP(nfld, value, ffp, poscellN,
                           f0, f3, ind0, ind3,
                           fisop, npt);
              ciso1[ntri] = npt;
              ciso2[ntri] = npt-1;
              ciso3[ntri] = npt-2;
              ntri++;
              key[npt-3] = KEY(-1,-1,elt,indFace);
              key[npt-2] = KEY(ind,indp,-1,-1);
              key[npt-1] = KEY(indp,-1,elt,-1);
              key2[npt-3] = 30;
              key2[npt-2] = 31;
              key2[npt-1] = 32;
              
              VERTEXINTERP(nfld, value, ffp, poscellN,
                           f0, f1, ind0, ind1,
                           fisop, npt);
              VERTEXINTERP(nfld, value, ffp, poscellN,
                           f1, f2, ind1, ind2,
                           fisop, npt);
              VERTEXINTERP(nfld, value, ffp, poscellN,
                           f2, f3, ind2, ind3,
                           fisop, npt);
              ciso1[ntri] = npt;
              ciso2[ntri] = npt-1;
              ciso3[ntri] = npt-2;
              ntri++;
              key[npt-3] = KEY(-1,-1,elt,indFace);
              key[npt-2] = KEY(ind,-1,-1,indFace);
              key[npt-1] = KEY(indp,ind,-1,-1);
              key2[npt-3] = 33;
              key2[npt-2] = 34;
              key2[npt-1] = 35;
              break;

              case 0x05:
              VERTEXINTERP(nfld, value, ffp, poscellN,
                           f0, f1, ind0, ind1,
                           fisop, npt);
              VERTEXINTERP(nfld, value, ffp, poscellN,
                           f2, f3, ind2, ind3,
                           fisop, npt);
              VERTEXINTERP(nfld, value, ffp, poscellN,
                           f0, f3, ind0, ind3,
                           fisop, npt);
              ciso1[ntri] = npt-2;
              ciso2[ntri] = npt-1;
              ciso3[ntri] = npt;
              ntri++;
              key[npt-3] = KEY(-1,-1,elt,indFace);
              key[npt-2] = KEY(ind,indp,-1,-1);
              key[npt-1] = KEY(indp,-1,elt,-1);
              key2[npt-3] = 36;
              key2[npt-2] = 37;
              key2[npt-1] = 38;
              
              VERTEXINTERP(nfld, value, ffp, poscellN,
                           f0, f1, ind0, ind1,
                           fisop, npt);
              VERTEXINTERP(nfld, value, ffp, poscellN,
                           f1, f2, ind1, ind2,
                           fisop, npt);
              VERTEXINTERP(nfld, value, ffp, poscellN,
                           f2, f3, ind2, ind3,
                           fisop, npt);
              ciso1[ntri] = npt-2;
              ciso2[ntri] = npt-1;
              ciso3[ntri] = npt;
              ntri++;
              key[npt-3] = KEY(-1,-1,elt,indFace);
              key[npt-2] = KEY(ind,-1,indFace,-1);
              key[npt-1] = KEY(indp,ind,-1,-1);
              key2[npt-3] = 39;
              key2[npt-2] = 40;
              key2[npt-1] = 41;
              break;

              case 0x09: // OK
              VERTEXINTERP(nfld, value, ffp, poscellN,
                           f0, f1, ind0, ind1,
                           fisop, npt);
              VERTEXINTERP(nfld, value, ffp, poscellN,
                           f1, f3, ind1, ind3,
                           fisop, npt);
              VERTEXINTERP(nfld, value, ffp, poscellN,
                           f2, f3, ind2, ind3,
                           fisop, npt);
              ciso1[ntri] = npt; // OK
              ciso2[ntri] = npt-1;
              ciso3[ntri] = npt-2;
              ntri++;
              key[npt-3] = KEY(-1,-1,elt,indFace);
              key[npt-2] = KEY(indp,-1,-1,indFace);
              key[npt-1] = KEY(indp,ind,-1,-1);
              key2[npt-3] = 42;
              key2[npt-2] = 43;
              key2[npt-1] = 44;
              
              VERTEXINTERP(nfld, value, ffp, poscellN,
                           f0, f1, ind0, ind1,
                           fisop, npt);
              VERTEXINTERP(nfld, value, ffp, poscellN,
                           f0, f2, ind0, ind2,
                           fisop, npt);
              VERTEXINTERP(nfld, value, ffp, poscellN,
                           f2, f3, ind2, ind3,
                           fisop, npt);
              ciso1[ntri] = npt-2; // OK
              ciso2[ntri] = npt-1;
              ciso3[ntri] = npt;
              ntri++;
              key[npt-3] = KEY(-1,-1,elt,indFace);
              key[npt-2] = KEY(ind,-1,elt,-1);
              key[npt-1] = KEY(ind,indp,-1,-1);
              key2[npt-3] = 45;
              key2[npt-2] = 46;
              key2[npt-1] = 47;
              break;

              case 0x06:
              VERTEXINTERP(nfld, value, ffp, poscellN,
                           f0, f1, ind0, ind1,
                           fisop, npt);
              VERTEXINTERP(nfld, value, ffp, poscellN,
                           f1, f3, ind1, ind3,
                           fisop, npt);
              VERTEXINTERP(nfld, value, ffp, poscellN,
                           f2, f3, ind2, ind3,
                           fisop, npt);
              ciso1[ntri] = npt-2; // OK
              ciso2[ntri] = npt-1;
              ciso3[ntri] = npt;
              ntri++;
              key[npt-3] = KEY(-1,-1,elt,indFace);
              key[npt-2] = KEY(indp,-1,-1,indFace);
              key[npt-1] = KEY(indp,ind,-1,-1);
              key2[npt-3] = 48;
              key2[npt-2] = 49;
              key2[npt-1] = 50;
              
              VERTEXINTERP(nfld, value, ffp, poscellN,
                           f0, f1, ind0, ind1,
                           fisop, npt);
              VERTEXINTERP(nfld, value, ffp, poscellN,
                           f0, f2, ind0, ind2,
                           fisop, npt);
              VERTEXINTERP(nfld, value, ffp, poscellN,
                           f2, f3, ind2, ind3,
                           fisop, npt);
              ciso1[ntri] = npt;
              ciso2[ntri] = npt-1;
              ciso3[ntri] = npt-2;
              ntri++;
              key[npt-3] = KEY(-1,-1,elt,indFace);
              key[npt-2] = KEY(ind,-1,elt,-1);
              key[npt-1] = KEY(indp,ind,-1,-1);
              key2[npt-3] = 51;
              key2[npt-2] = 52;
              key2[npt-1] = 53;
              break;

              case 0x07:
              VERTEXINTERP(nfld, value, ffp, poscellN,
                           f3, f0, ind3, ind0,
                           fisop, npt);
              VERTEXINTERP(nfld, value, ffp, poscellN,
                           f3, f2, ind3, ind2,
                           fisop, npt);
              VERTEXINTERP(nfld, value, ffp, poscellN,
                           f3, f1, ind3, ind1,
                           fisop, npt);
              ciso1[ntri] = npt-2;
              ciso2[ntri] = npt-1;
              ciso3[ntri] = npt;
              ntri++;
              key[npt-3] = KEY(indp,-1,elt,-1);
              key[npt-2] = KEY(ind,indp,-1,-1);
              key[npt-1] = KEY(indp,-1,-1,indFace);
              key2[npt-3] = 54;
              key2[npt-2] = 55;
              key2[npt-1] = 56;
              break;

              case 0x08: // OK
              VERTEXINTERP(nfld, value, ffp, poscellN,
                           f3, f0, ind3, ind0,
                           fisop, npt);
              VERTEXINTERP(nfld, value, ffp, poscellN,
                           f3, f2, ind3, ind2,
                           fisop, npt);
              VERTEXINTERP(nfld, value, ffp, poscellN,
                           f3, f1, ind3, ind1,
                           fisop, npt);
              ciso1[ntri] = npt-2;
              ciso2[ntri] = npt-1;
              ciso3[ntri] = npt;
              ntri++;
              key[npt-3] = KEY(indp,-1,elt,0);
              key[npt-2] = KEY(ind,indp,-1,-1);
              key[npt-1] = KEY(indp,-1,-1,indFace);
              key2[npt-3] = 57;
              key2[npt-2] = 58;
              key2[npt-1] = 59;
              break;
            }
          }
        }
      }
    }

  // Analyse
  /*
  for (E_Int ithread = 0; ithread < nthreads; ithread++)
  {
    E_Float* x = fisos[ithread]->begin(1);
    E_Float* y = fisos[ithread]->begin(2);
    E_Float* z = fisos[ithread]->begin(3);
    ETK* key = keys[ithread]->begin();
    ETK* key2 = keys[ithread]->begin(2);
    for (E_Int i = 0; i < npts[ithread]; i++)
      printf("%d: %f %f %f (key=%d, source=%d)\n",i,x[i],y[i],z[i],key[i],key2[i]);
    fflush(stdout);
  }
  */
    
  // Nbre de pts dup + nbre de tri
  E_Int ntri = 0; E_Int npt = 0;
  for (E_Int i = 0; i < nthreads; i++) 
  { prevT[i] = ntri; ntri += ntris[i];
    prevF[i] = npt; npt += npts[i]; }
  printf("nbre de pts dup=%d, nbre de tris=%d\n",npt,ntri);
  fflush(stdout);
  
  // Construction de la map (cher) key->indDup
  //printf("construction de la map\n");
  std::unordered_map<ETK, E_Int> map;
  for (E_Int i = 0; i < nthreads; i++)
  {
    E_Int f = prevF[i];
    ETK* key = keys[i]->begin();
    for (E_Int j = 0; j < keys[i]->getSize(); j++)
      map[key[j]] = j+f;
  }
  
  // invMap: ind dup -> ind 
  FldArrayI invMap(npt);
    
  // Nouveau nombre de points (non dup)
  npt = map.size();
  //printf("nbre de pts uniques=%d\n",npt); fflush(stdout);
  E_Int c = 0;
  for (std::pair<E_Int,E_Int> elt : map)
  {
    //E_Int k = elt.first;
    E_Int ind = elt.second;
    //printf("map c=%d inddup=%d key=%d\n",c,ind,k);
    invMap[ind] = c;
    c++;
  }
  //fflush(stdout);
  //printf("invmap0\n");
  //for (E_Int i = 0; i < invMap.getSize(); i++) printf("invdup=%d: ind=%d\n",i,invMap[i]);
  //fflush(stdout);
  
  // complete invMap
#pragma omp parallel default(shared)
  {
    E_Int ithread = __CURRENT_THREAD__;

    E_Int f = prevF[ithread];
    ETK* key = keys[ithread]->begin();
    for (E_Int i = 0; i < npts[ithread]; i++)
    { 
      E_Int k = key[i];
      //printf("check f=%d key=%d inddup=%d [%d]\n",f+i,k,map[k],invMap[map[k]]);
      //if (f+i != map[k]) 
      invMap[f+i] = invMap[map[k]];
    }
  }
  
  //printf("invmap\n");
  //for (E_Int i = 0; i < invMap.getSize(); i++) printf("invdup=%d: ind=%d\n",i,invMap[i]);
  //fflush(stdout);
  
  printf("reconstruction fiso (%d points)\n", npt); fflush(stdout);
  fiso.malloc(npt, nfld);
  ciso.malloc(ntri, 3);
  
#pragma omp parallel default(shared)
  {
    E_Int ithread = __CURRENT_THREAD__;
  
    E_Int f = prevF[ithread];
    E_Int np = npts[ithread];
    //printf("%d %d\n", np, f); fflush(stdout);
    for (E_Int n = 1; n <= nfld; n++)
    {
      E_Float* fisop = fiso.begin(n);
      E_Float* fisol = fisos[ithread]->begin(n);
      for (E_Int e = 0; e < np; e++) fisop[invMap[e+f]] = fisol[e];
    }
  }
  //for (E_Int i = 0; i < npt; i++) printf("f %d: %f %f %f\n",i,fiso(i,1),fiso(i,2),fiso(i,3));
  //fflush(stdout);
  
  //printf("reconstruction ciso\n"); fflush(stdout);
#pragma omp parallel default(shared)
  {
    E_Int ithread = __CURRENT_THREAD__;

    E_Int f = prevF[ithread];
    E_Int p = prevT[ithread];
    E_Int ne = ntris[ithread];
    for (E_Int n = 1; n <= 3; n++)
    {
      E_Int* cisop = ciso.begin(n);
      E_Int* cisol = cisos[ithread]->begin(n);
      for (E_Int e = 0; e < ne; e++) cisop[e+p] = invMap[cisol[e]+f-1]+1;
    }
  } 
  
  //for (E_Int i = 0; i < ntri; i++) printf("c %d: %d %d %d\n",i,ciso(i,1),ciso(i,2),ciso(i,3));
  //fflush(stdout);
    
  // delete
  for (E_Int i = 0; i < nthreads; i++) delete keys[i];
  delete [] keys;

  delete [] prevT; delete [] prevF;
  delete [] npts; delete [] ntris;
  for (E_Int i = 0; i < nthreads; i++) delete fisos[i];
  for (E_Int i = 0; i < nthreads; i++) delete cisos[i];
  delete [] fisos; delete [] cisos;
  
  return;

  // old code
  /*
    E_Int ntri = 0; E_Int npt = 0;
    for (E_Int i = 0; i < nthreads; i++) 
      { prevT[i] = ntri; ntri += ntris[i];
        prevF[i] = npt; npt += npts[i]; }
    //printf("%d %d\n", npt, ntri);

    fiso.malloc(npt, nfld);
    ciso.malloc(ntri, 3);
  
#pragma omp parallel default(shared)
    {
    E_Int ithread = __CURRENT_THREAD__;
    E_Int nq = ntris[ithread];
    E_Int p = prevT[ithread];
    E_Int f = prevF[ithread];
    for (E_Int n = 1; n <= 3; n++)
    {
      E_Int* cisop = ciso.begin(n);
      E_Int* cisol = cisos[ithread]->begin(n);
      for (E_Int e = 0; e < nq; e++) cisop[e+p] = cisol[e]+f;
    }
    E_Int np = npts[ithread];
    for (E_Int n = 1; n <= nfld; n++)
    {
      E_Float* fisop = fiso.begin(n);
      E_Float* fisol = fisos[ithread]->begin(n);
      for (E_Int e = 0; e < np; e++) fisop[e+f] = fisol[e];
    }
  }
  delete [] prevT; delete [] prevF;
  delete [] npts; delete [] ntris;
  for (E_Int i = 0; i < nthreads; i++) delete fisos[i];
  for (E_Int i = 0; i < nthreads; i++) delete cisos[i];
  delete [] fisos; delete [] cisos;
  */
}
