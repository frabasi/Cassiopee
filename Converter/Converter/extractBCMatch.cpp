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
#include "converter.h"

#define signVar(a) (a < 0 ? -1 : 1)

using namespace std;
using namespace K_FLD;

#include <map>

//=============================================================================
//=============================================================================
PyObject* K_CONVERTER::extractBCMatchNG(PyObject* self, PyObject* args )
{
  // Return index of boundary faces in receiver zone and associated fields 
  // extracted from donor zone

  PyObject *zone, *pyIndices, *pyVariables ; 
  char *GridCoordinates, *FlowSolutionNodes, *FlowSolutionCenters;

  if (!PYPARSETUPLEI(args, "OOOsss", "OOOsss", &zone, &pyIndices, &pyVariables, 
               &GridCoordinates, &FlowSolutionNodes, &FlowSolutionCenters )) 
     return NULL;

  // Zone 
  // ~~~~
  E_Int ni, nj, nk, cnSize, cnNfld ; 
  char* varString; char* eltType;
  vector<E_Float*> fields; vector<E_Int> locs;
  vector<E_Int*> cn;
  vector<PyArrayObject*> hook;

  E_Int zoneType = K_PYTREE::getFromZone(zone, 0, 1, varString, fields, locs, ni, nj, nk, 
                                         cn, cnSize, cnNfld, eltType, hook, GridCoordinates, 
                                         FlowSolutionNodes, FlowSolutionCenters);

  if ( zoneType == 0) 
  {
    PyErr_SetString(PyExc_TypeError, "extractBCMatchNG: not a valid zone.");
    RELEASESHAREDZ(hook, varString, eltType);
    return NULL;
  }

  // Parent Elements 
  // ~~~~~~~~~~~~~~~
  E_Int* PE = NULL;
  if ( zoneType == 2)
  {
    if ( cn.size() < 3)//PE does not exist
    {
      PyErr_SetString(PyExc_TypeError, "extractBCMatchNG: ParentElements node must be defined in zone.");
      RELEASESHAREDZ(hook, varString, eltType);
      return NULL;  
    }
    else PE = cn[2];
  }

  // Positions des variables a extraire 
  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  vector<E_Int> posvars;
  E_Int posvar;   
  char* varStringOut = new char[K_ARRAY::VARSTRINGLENGTH];
  varStringOut[0] = '\0';

  if (PyList_Check(pyVariables) != 0)
  {
    int nvariables = PyList_Size(pyVariables);
    if (nvariables > 0)
    {
      for (int i = 0; i < nvariables; i++)
      {
        PyObject* tpl0 = PyList_GetItem(pyVariables, i);
        if (PyString_Check(tpl0) == 0) 
	{
          PyErr_Warn(PyExc_Warning, "extractBCMatchNG: variable must be a string. Skipped.");
	}
        else 
        {
          char* varname    = PyString_AsString(tpl0); 
	  if ( varStringOut[0] == '\0' )
	    strcpy( varStringOut, varname );
	  else 
	  {
	    strcat( varStringOut, "," );
	    strcat( varStringOut, varname );
	  }
	    
          posvar = K_ARRAY::isNamePresent(varname, varString);  
          if (posvar != -1 ) posvars.push_back(posvar);
        }
      }
    }
  }

  // Indices des faces 
  // ~~~~~~~~~~~~~~~~~
  FldArrayI* ind;
  E_Int res = K_NUMPY::getFromNumpyArray(pyIndices, ind, true);

  if ( res == 0)
  {
    PyErr_SetString(PyExc_TypeError, "extractBCMatchNG: not a valid numpy for indices.");
    RELEASESHAREDZ(hook, varString, eltType);
    return NULL;   
  }

  E_Int* ptrInd = ind->begin();

  // Tableau des champs 
  // ~~~~~~~~~~~~~~~~~~
  int nfld = PyList_Size(pyVariables);
  int nint = ind->getSize();
  PyObject* pyFldD = K_ARRAY::buildArray2(nfld,varStringOut,nint,1,1,2); 

  FldArrayF*  fldD; 
  FldArrayI* cn2;
  E_Int ni2, nj2, nk2;
  K_ARRAY::getFromArray2(pyFldD, varStringOut, fldD, ni2, nj2, nk2, cn2, eltType);


  // Extrapolation
  // ~~~~~~~~~~~~~

  for (E_Int novar = 0; novar < nfld; novar++)     
  {
      E_Int posv       = posvars[novar];
      E_Float* fieldV  = fields[posv];
      E_Float* ptrFldD = fldD->begin(novar+1); 

      for (E_Int noint = 0; noint < nint; noint++)
      {
	E_Int indint   = ptrInd[noint]-1;        
        E_Int indcell  = PE[indint]-1;
	ptrFldD[noint] = fieldV[indcell];
      }
  }

  return pyFldD; 
}


//=============================================================================
PyObject* K_CONVERTER::extractBCMatchStruct(PyObject* self, PyObject* args )
{
  // Return index of boundary faces in receiver zone and associated fields 
  // extracted from donor zone

  PyObject *fields;

  E_Int niD, njD, nkD;       // dim zone donneuse
  E_Int niR, njR, nkR;       // dim zone receveuse 

  E_Int iminD, jminD, kminD; // indices fenetre donneuse
  E_Int imaxD, jmaxD, kmaxD; // indices fenetre donneuse
  E_Int iminR, jminR, kminR; // indices fenetre receveuse
  E_Int imaxR, jmaxR, kmaxR; // indices fenetre receveuse 

  E_Int triI, triJ, triK ;   // transform (issu du GC de la zone "receveuse")

  if (!PYPARSETUPLEI(args, "O(llllll)(llllll)(lll)(lll)", "O(iiiiii)(iiiiii)(iii)(iii)", 
                     &fields, &iminD, &jminD, &kminD, &imaxD, &jmaxD, &kmaxD,
		              &iminR, &jminR, &kminR, &imaxR, &jmaxR, &kmaxR, 
		              &niR, &njR, &nkR, 
                              &triI, &triJ, &triK )) return NULL;

  // Check array
  // ===========
  FldArrayF* FCenter; FldArrayI* cn;
  char* varString; char* eltType;
  E_Int res = K_ARRAY::getFromArray2(fields, varString, FCenter, niD, njD, nkD, 
                                    cn, eltType); 

  if (res != 1)
  {
    PyErr_SetString(PyExc_TypeError, "extractBCMatchStruct: array must be structured."); 
    if (res == 2) RELEASESHAREDS(fields, FCenter);
    return NULL; 
  }

  // 
  E_Int dim       = 3;
  E_Int noindint  = 0;
  E_Int ind ;
  E_Int nbIntID   = (niD+1)*njD*K_FUNC::E_max(1,nkD) ;
  E_Int nbIntJD   = (njD+1)*niD*K_FUNC::E_max(1,nkD) ;
  E_Int nbIntIR   = (niR+1)*njR*K_FUNC::E_max(1,nkR) ;
  E_Int nbIntJR   = (njR+1)*niR*K_FUNC::E_max(1,nkR) ;

  E_Int ifaceR ;
  E_Int jfaceR ;
  E_Int kfaceR ;
  E_Int shift  ;

  // compute dim 
  if ((niD == 1) or (njD == 1) or (nkD ==1)) 
  { 
    dim = 2; 
  }

  // build output arrays 
  // ===================
  E_Int nfld = FCenter->getNfld();
  E_Int nint = max(1,(imaxD-iminD))*max(1,(jmaxD-jminD))*max(1,(kmaxD-kminD)); 

  // 1. tableau des indices 
  // ~~~~~~~~~~~~~~~~~~~~~~
  // 1. a Indices des faces de la zone receveuse 
  // -------------------------------------------
  PyObject* indFaceR = K_NUMPY::buildNumpyArray(nint,1,1);
  E_Int* ptrIndFaceR = K_NUMPY::getNumpyPtrI(indFaceR);
  // 1. b Indices des faces de la zone donneuse
  // ------------------------------------------
  PyObject* indFaceD = K_NUMPY::buildNumpyArray(nint,1,1);
  E_Int* ptrIndFaceD = K_NUMPY::getNumpyPtrI(indFaceD);

  // 2. tableau des champs 
  // ~~~~~~~~~~~~~~~~~~~~~
  PyObject* pyFldD = K_ARRAY::buildArray2(nfld,varString,nint,1,1,2); 

  FldArrayF*  fBC; 
  FldArrayI* cnBC;
  E_Int ni2, nj2, nk2;
  K_ARRAY::getFromArray2(pyFldD, varString, fBC, ni2, nj2, nk2, cnBC, eltType);

  // printf("trirac : %d %d %d \n",triI,triJ,triK);

  // Cas 2D
  // ======
  if (dim == 2)
  {
    // Face donneuse en i 
    // ******************
    if (iminD == imaxD) 
    { 
      // printf("Frontiere en i \n");
      // 1. tableau des indices
      // ----------------------

        // for (E_Int jface = jminD-1 ; jface < jmaxD-1 ; jface ++) // A supprimer
        // {
        //   ptrIndFaceD[noindint]  = iminD - 1 + jface*(niD+1) ; // A supprimer
 
        //   // printf("indD : %d \n", ptrIndFaceD[noindint]);// A supprimer
        //   noindint++;// A supprimer
        // }// A supprimer

      // 1.a. face receveuse en i
      if (iminR == imaxR) 
      {
        // printf("Frontiere receveuse en i \n");
	noindint = 0;

        for (E_Int jface = jminD-1 ; jface < jmaxD-1 ; jface ++) 
        { 
          if (triJ > 0)
	  {
	    jfaceR =  jface + jminR - jminD ;
	  }
	  else
	  {
	    jfaceR = (jmaxR-2) - (jface-jminD+1); 
	  }

          ptrIndFaceR[noindint] = iminR - 1 + jfaceR*(niR+1) ;
          // printf("** indR : %d \n", ptrIndFaceR[noindint]);
          noindint++;
	}
      }
      // 1.b. face receveuse en j
      if (jminR == jmaxR) 
      {
        // printf("Frontiere receveuse en j \n");
	noindint = 0;

        for (E_Int jface = jminD-1 ; jface < jmaxD-1 ; jface ++) 
        { 
          if (triI > 0)
	  {
	    ifaceR = jface + iminR - jminD ;
	  }
	  else
	  {
	    ifaceR = (imaxR-2) - (jface-jminD+1)  ; 
	  }

          shift  = (jminR-1)*niR + nbIntIR ; 
          ptrIndFaceR[noindint] = shift + ifaceR ;
          // printf("indR   : %d \n", ptrIndFaceR[noindint]);
          noindint++;
	}
      } // jminR=jmaxR

      // 2. tableau des champs 
      for (E_Int var = 1; var <= nfld; var++)
      {  
        E_Float* fld = fBC->begin(var);
        E_Float* fce = FCenter->begin(var);

        noindint = 0 ;

        for (E_Int jface = jminD-1 ; jface < jmaxD-1 ; jface ++) 
        {
          if (iminD==1) { ind = jface*niD         ; } 
          else          { ind = jface*niD + niD-1 ; } 

          fld[noindint] = fce[ind] ; 
          noindint++;
        } 
      } // var loop 
    }
    // Si frontiere en j
    // *****************
    if (jminD == jmaxD) 
    {      
      // printf("Frontiere en j \n");
      // 1. tableau des indices 
      // E_Int shift = (jminD-1)*niD + nbIntID ;

      // for (E_Int iface = iminD - 1 ; iface < imaxD-1 ; iface ++)  // A supprimer
      // {
      //   ptrIndFaceD[noindint] = shift + iface ; // A supprimer
      //   // printf("indD : %d \n", ptrIndFaceD[noindint]);// A supprimer
      //   noindint++; // A supprimer
      // }   // A supprimer

      // printf("indR : ");
      // 1.a. face receveuse en i
      if (iminR == imaxR) 
      {
        // printf("Frontiere receveuse en i \n");
	noindint = 0;

	for (E_Int iface = iminD - 1 ; iface < imaxD-1 ; iface ++)
        { 
          if (triJ > 0)
	  {
	    jfaceR =  iface + jminR - iminD ; 
	  }
	  else
	  {
	    jfaceR = (jmaxR-2) - (iface-iminD+1);
	  }

          ptrIndFaceR[noindint] = iminR - 1 + jfaceR*(niR+1) ;
          // printf(" %d ", ptrIndFaceR[noindint]);
          noindint++;
	}
      } // iminR==imaxR 

      // 1.b. face receveuse en j
      if (jminR == jmaxR) 
      {
        // printf("Frontiere receveuse en j \n");
	noindint = 0;

	for (E_Int iface = iminD - 1 ; iface < imaxD-1 ; iface ++)
        { 
          E_Int shift  = (jminR-1)*niR + nbIntIR ;

          if (triI > 0)
	  {
	    ifaceR = iface + iminR - iminD ;
	  }
	  else
	  {
	    ifaceR = (imaxR-2) - (iface-iminD+1) ;
	  }

          ptrIndFaceR[noindint] = shift + ifaceR ;
          // printf(" %d ", ptrIndFaceR[noindint]);
          noindint++;
	}
      }
  
      // 2. tableau des champs 
      for (E_Int var = 1; var <= nfld; var++)
      {  
        E_Float* fld = fBC->begin(var);
        E_Float* fce = FCenter->begin(var);

        noindint = 0 ;

        for (E_Int iface = iminD-1 ; iface < imaxD-1 ; iface ++) 
        {
          if (jminD==1) { ind = iface               ; } 
          else          { ind = iface + niD*(njD-1) ; } 

          fld[noindint] = fce[ind] ; 
          noindint++;
        }
      } // var loop
    }// (si frontiere en j)
 
  } //(si dim=2)

  // Cas 3D
  // ======
  else if (dim == 3)
  {
    // printf("kminR : %d \n", kminR); 
    // ********************
    // Frontiere donneuse i 
    // ********************
    if (iminD == imaxD)
    {
      // printf("Frontiere donneuse en i \n");
      // noindint = 0 ;
      // printf("indD : ");// A supprimer

      // 1. tableau des indices  // A supprimer 
      // for (E_Int kface = kminD-1 ; kface < kmaxD-1 ; kface ++)  // A supprimer 
      // {
      //   for (E_Int jface = jminD-1 ; jface < jmaxD-1 ; jface ++)  // A supprimer 
      //   {
      //     ptrIndFaceD[noindint] = iminD - 1 + jface*(niD+1) + kface*(niD+1)*njD ; // A supprimer 
      //     // printf("%d ", ptrIndFaceD[noindint]);// A supprimer
      //     noindint++; // A supprimer 
      //   } 
      // } 
      // // printf("\n ");// A supprimer
 
    // ~~~~~~~~~~~~~~~~~~~~~~~~
    // Frontiere receveuse en i 
    // ~~~~~~~~~~~~~~~~~~~~~~~~
      if (iminR == imaxR)
      {
        // printf("Frontiere receveuse en i \n");
        noindint = 0 ;

        if (abs(triJ)==2) // kD <-> kR et  jD <-> jR
	{
          // printf("Face receveuse jD=jR, kD=kR) \n");
          // printf("indR : ");
          for (E_Int kface = kminD-1 ; kface < kmaxD-1 ; kface ++) 
          {
            if (triK > 0) { kfaceR = kface + kminR - kminD ;     }
	    else          { kfaceR = (kmaxR-2)-(kface-kminD+1) ; }

            for (E_Int jface = jminD-1 ; jface < jmaxD-1 ; jface ++) 
            {
              if (triJ > 0){ jfaceR = jface + jminR - jminD ;    }
	      else         { jfaceR = (jmaxR-2)-(jface-jminD+1); }

              ptrIndFaceR[noindint] = iminR - 1 + jfaceR*(niR+1) + kfaceR*(niR+1)*njR ;
              // printf("%d ", ptrIndFaceR[noindint]);
              noindint++;
	    }
          } 
        } // triJ=2

        if (abs(triJ)==3) // kD <-> jR et  jD <-> kR
	{
          // printf("Face receveuse jD=kR, kD=jR) \n");
          // printf("indR : ");

          for (E_Int kface = kminD-1 ; kface < kmaxD-1 ; kface ++)
          {
            for (E_Int jface = jminD-1 ; jface < jmaxD-1 ; jface ++)  
            { 
              if (triK > 0) { kfaceR = jface + kminR - jminD ;     }
	      else          { kfaceR = (kmaxR-2)-(jface-jminD+1) ; }

              if (triJ > 0) { jfaceR = kface + jminR - kminD ;     }
	      else          { jfaceR = (jmaxR-2)-(kface-kminD+1);  }
	    
            ptrIndFaceR[noindint] = iminR - 1 + jfaceR*(niR+1) + kfaceR*(niR+1)*njR ;
            // printf("%d ", ptrIndFaceR[noindint]);
            noindint++;
	    }
          } 
        } // triJ=3
 
        // printf("\n ");
      }
     
    // ~~~~~~~~~~~~~~~~~~~~~~~~
    // Frontiere receveuse en j 
    // ~~~~~~~~~~~~~~~~~~~~~~~~
      if (jminR == jmaxR)
      {
        // printf("Frontiere receveuse en j \n");
        noindint = 0 ;

        if (abs(triI)==2) // jD <-> iR et  kD <-> kR
	{
          // printf("Face receveuse jD=iR, kD=kR) \n");
          // printf("indR : ");
          E_Int shift = (jminR-1)*niR + nbIntIR ;

          for (E_Int kface = kminD-1 ; kface < kmaxD-1 ; kface ++) 
          {
            if (triK > 0) { kfaceR = kface + kminR - kminD ;     }
	    else          { kfaceR = (kmaxR-2)-(kface-kminD+1) ; }

            for (E_Int jface = jminD-1 ; jface < jmaxD-1 ; jface ++) 
            {
              if (triI > 0){ ifaceR = jface + iminR - jminD ;    }
	      else         { ifaceR = (imaxR-2)-(jface-jminD+1); }

              ptrIndFaceR[noindint] = shift + ifaceR + kfaceR*niR*(njR+1) ;
              // printf("%d ", ptrIndFaceR[noindint]);
              noindint++;
	    }
          } 
        } // triI=1

        if (abs(triI)==3) // jD <-> kR et  kD <-> iR
	{
          // printf("Face receveuse jD=kR, kD=iR) \n");
          // printf("indR : ");
          E_Int shift = (jminR-1)*niR + nbIntIR ;

          for (E_Int kface = kminD-1 ; kface < kmaxD-1 ; kface ++)
          {
            for (E_Int jface = jminD-1 ; jface < jmaxD-1 ; jface ++)  
            { 
              if (triK > 0) { kfaceR = jface + kminR - jminD ;     }
	      else          { kfaceR = (kmaxR-2)-(jface-jminD+1) ; }

              if (triI > 0) { ifaceR = kface + iminR - kminD ;     }
	      else          { ifaceR = (imaxR-2)-(kface-kminD+1);  }
	    
            ptrIndFaceR[noindint] = shift + ifaceR + kfaceR*niR*(njR+1) ;
            // printf("%d ", ptrIndFaceR[noindint]);
            noindint++;
	    }
          } 
        } // triJ=3
 
        // printf("\n ");
      }

    // ~~~~~~~~~~~~~~~~~~~~~~~~
    // Frontiere receveuse en k
    // ~~~~~~~~~~~~~~~~~~~~~~~~
      if (kminR == kmaxR)
      {
        // printf("Frontiere receveuse en k \n");
        noindint = 0 ;

        if (abs(triI)==2) // jD <-> iR et  kD <-> jR
      	{
          // printf("Face receveuse jD=iR, kD=jR) \n");
          // printf("indR : ");
          E_Int shift = (kminR-1)*niR*njR + nbIntIR + nbIntJR ;

          for (E_Int kface = kminD-1 ; kface < kmaxD-1 ; kface ++) 
          {
            if (triJ > 0) { jfaceR = kface + jminR - kminD ;     }
      	    else          { jfaceR = (jmaxR-2)-(kface-kminD+1) ; }

            for (E_Int jface = jminD-1 ; jface < jmaxD-1 ; jface ++) 
            {
              if (triI > 0){ ifaceR = jface + iminR - jminD ;    }
      	      else         { ifaceR = (imaxR-2)-(jface-jminD+1); }

              ptrIndFaceR[noindint] = shift + ifaceR + jfaceR*niR ;
              // printf("%d ", ptrIndFaceR[noindint]);
              noindint++;
      	    }
          } 
        } // triI=1
        // printf("\n");

        if (abs(triI)==3) // jD <-> jR et  kD <-> iR
      	{
          // printf("Face receveuse jD=jR, kD=iR) \n");
          // printf("indR : ");
          E_Int shift = (kminR-1)*niR*njR + nbIntIR + nbIntJR ;

          for (E_Int kface = kminD-1 ; kface < kmaxD-1 ; kface ++)
          {
            if (triI > 0) { ifaceR = kface + iminR - kminD ;     }
      	    else          { ifaceR = (imaxR-2)-(kface-kminD+1);  }

            for (E_Int jface = jminD-1 ; jface < jmaxD-1 ; jface ++)  
            { 
              if (triJ > 0) { jfaceR = jface + jminR - jminD ;     }
      	      else          { jfaceR = (jmaxR-2)-(jface-jminD+1) ; }
	    
            ptrIndFaceR[noindint] = shift + ifaceR + jfaceR*niR ;
            // printf("%d ", ptrIndFaceR[noindint]);
            noindint++;
      	    }
          } 
        } // triJ=3
 
        // printf("\n ");
      }

      // 2. tableau des champs 
      for (E_Int var = 1; var <= nfld; var++)
      {  
        E_Float* fld = fBC->begin(var);
        E_Float* fce = FCenter->begin(var);

        noindint = 0 ;

        for (E_Int kface = kminD-1 ; kface < kmaxD-1 ; kface ++) 
        {
          for (E_Int jface = jminD-1 ; jface < jmaxD-1 ; jface ++) 
          {
            if (iminD==1) { ind = iminD-1 + jface*niD + kface*njD*niD; } 
            else          { ind = niD-1    + jface*niD + kface*njD*niD; } 

            fld[noindint] = fce[ind] ; 
            noindint++;
          }
	}

      } // var loop 
    }
    // Si frontiere en j
    // *****************
    if (jminD == jmaxD)
    {
      // printf("Frontiere donneuse en j \n");
      // noindint = 0 ;
      // // printf("indD : ");// A supprimer

      // // 1. tableau des indices 
      // E_Int shift = (jminD-1)*niD + nbIntID ;

      // for (E_Int kface = kminD-1 ; kface < kmaxD-1 ; kface ++) // A supprimer
      // {
      //   for (E_Int iface = iminD-1 ; iface < imaxD-1 ; iface ++) // A supprimer
      //   {
      //     ptrIndFaceD[noindint]  = shift + iface + kface*niD*(njD+1) ;// A supprimer
      //     // printf("%d ", ptrIndFaceD[noindint]);// A supprimer
      //     noindint++;// A supprimer
      //   }// A supprimer
      // }
      // printf("\n ");// A supprimer

    // ~~~~~~~~~~~~~~~~~~~~~~~~
    // Frontiere receveuse en i 
    // ~~~~~~~~~~~~~~~~~~~~~~~~
      if (iminR == imaxR)
      {
        // printf("Frontiere receveuse en i \n");
        noindint = 0 ;

        if (abs(triJ)==1) // kD <-> kR et  iD <-> jR
	{
          // printf("Face receveuse jD=jR, kD=kR) \n");
          // printf("indR : ");
          for (E_Int kface = kminD-1 ; kface < kmaxD-1 ; kface ++) 
          {
            if (triK > 0) { kfaceR = kface + kminR - kminD ;     }
	    else          { kfaceR = (kmaxR-2)-(kface-kminD+1) ; }

            for (E_Int iface = iminD-1 ; iface < imaxD-1 ; iface ++) 
            {
              if (triJ > 0){ jfaceR = iface + jminR - iminD ;    }
	      else         { jfaceR = (jmaxR-2)-(iface-iminD+1); }

              ptrIndFaceR[noindint] = iminR - 1 + jfaceR*(niR+1) + kfaceR*(niR+1)*njR ;
              // printf("%d ", ptrIndFaceR[noindint]);
              noindint++;
	    }
          } 
        } // triJ=2

        if (abs(triJ)==3) // kD <-> jR et  iD <-> kR
	{
          // printf("Face receveuse iD=kR, kD=jR) \n");
          // printf("indR : ");

          for (E_Int kface = kminD-1 ; kface < kmaxD-1 ; kface ++)
          {
            for (E_Int iface = iminD-1 ; iface < imaxD-1 ; iface ++)  
            { 
              if (triK > 0) { kfaceR = iface + kminR - iminD ;     }
	      else          { kfaceR = (kmaxR-2)-(iface-iminD+1) ; }

              if (triJ > 0) { jfaceR = kface + jminR - kminD ;     }
	      else          { jfaceR = (jmaxR-2)-(kface-kminD+1);  }
	    
            ptrIndFaceR[noindint] = iminR - 1 + jfaceR*(niR+1) + kfaceR*(niR+1)*njR ;
            // printf("%d ", ptrIndFaceR[noindint]);
            noindint++;
	    }
          } 
        } // triJ=3
 
        // printf("\n ");
      }

    // ~~~~~~~~~~~~~~~~~~~~~~~~
    // Frontiere receveuse en j 
    // ~~~~~~~~~~~~~~~~~~~~~~~~
      if (jminR == jmaxR)
      {
        // printf("Frontiere receveuse en j \n");
        noindint = 0 ;

        if (abs(triI)==1) // iD <-> iR et  kD <-> kR
	{
          // printf("Face receveuse iD=iR, kD=kR) \n");
          // printf("indR : ");
          E_Int shift = (jminR-1)*niR + nbIntIR ;

          for (E_Int kface = kminD-1 ; kface < kmaxD-1 ; kface ++) 
          {
            if (triK > 0) { kfaceR = kface + kminR - kminD ;     }
	    else          { kfaceR = (kmaxR-2)-(kface-kminD+1) ; }

            for (E_Int iface = iminD-1 ; iface < imaxD-1 ; iface ++) 
            {
              if (triI > 0){ ifaceR = iface + iminR - iminD ;    }
	      else         { ifaceR = (imaxR-2)-(iface-iminD+1); }

              ptrIndFaceR[noindint] = shift + ifaceR + kfaceR*niR*(njR+1) ;
              // printf("%d ", ptrIndFaceR[noindint]);
              noindint++;
	    }
          } 
        } // triI=1

        if (abs(triI)==3) // iD <-> kR et  kD <-> iR
	{
          // printf("Face receveuse iD=kR, kD=iR) \n");
          // printf("indR : ");
          E_Int shift = (jminR-1)*niR + nbIntIR ;

          for (E_Int kface = kminD-1 ; kface < kmaxD-1 ; kface ++)
          {
            for (E_Int iface = iminD-1 ; iface < imaxD-1 ; iface ++)  
            { 
              if (triK > 0) { kfaceR = iface + kminR - iminD ;     }
	      else          { kfaceR = (kmaxR-2)-(iface-iminD+1) ; }

              if (triI > 0) { ifaceR = kface + iminR - kminD ;     }
	      else          { ifaceR = (imaxR-2)-(kface-kminD+1);  }
	    
            ptrIndFaceR[noindint] = shift + ifaceR + kfaceR*niR*(njR+1) ;
            // printf("%d ", ptrIndFaceR[noindint]);
            noindint++;
	    }
          } 
        } // triJ=3
 
        // printf("\n ");
      }

    // ~~~~~~~~~~~~~~~~~~~~~~~~
    // Frontiere receveuse en k 
    // ~~~~~~~~~~~~~~~~~~~~~~~~
      if (kminR == kmaxR)
      {
        // printf("Frontiere receveuse en k \n");
        noindint = 0 ;

        if (abs(triI)==1) // iD <-> iR et  kD <-> jR
	{
          // printf("Face receveuse iD=iR, kD=jR) \n");
          // printf("indR : ");
          E_Int shift = (kminR-1)*niR*njR + nbIntIR + nbIntJR ;

          for (E_Int kface = kminD-1 ; kface < kmaxD-1 ; kface ++) 
          {
            if (triJ > 0) { jfaceR = kface + jminR - kminD ;     }
	    else          { jfaceR = (jmaxR-2)-(kface-kminD+1) ; }

            for (E_Int iface = iminD-1 ; iface < imaxD-1 ; iface ++) 
            {
              if (triI > 0){ ifaceR = iface + iminR - iminD ;    }
	      else         { ifaceR = (imaxR-2)-(iface-iminD+1); }

              ptrIndFaceR[noindint] = shift + ifaceR + jfaceR*niR ;
              // printf("%d ", ptrIndFaceR[noindint]);
              noindint++;
	    }
          } 
        } // triI=1

        if (abs(triI)==3) // iD <-> jR et  kD <-> iR
	{
          // printf("Face receveuse iD=jR, kD=iR) \n");
          // printf("indR : ");
          E_Int shift = (kminR-1)*niR*njR + nbIntIR + nbIntJR ;

          for (E_Int kface = kminD-1 ; kface < kmaxD-1 ; kface ++)
          {
            if (triI > 0) { ifaceR = kface + iminR - kminD ;     }
	    else          { ifaceR = (imaxR-2)-(kface-kminD+1);  }

            for (E_Int iface = iminD-1 ; iface < imaxD-1 ; iface ++)  
            { 
              if (triJ > 0) { jfaceR = iface + jminR - iminD ;     }
	      else          { jfaceR = (jmaxR-2)-(iface-iminD+1) ; }
	    
              ptrIndFaceR[noindint] = shift + ifaceR + jfaceR*niR ;
              // printf("%d ", ptrIndFaceR[noindint]);
              noindint++;
	    }
          } 
        } // triJ=3
 
        // printf("\n ");
      }

      // 2. tableau des champs 
      for (E_Int var = 1; var <= nfld; var++)
      {  
        E_Float* fld = fBC->begin(var);
        E_Float* fce = FCenter->begin(var);

        noindint = 0 ;

        for (E_Int kface = kminD-1 ; kface < kmaxD-1 ; kface ++) 
        {
          for (E_Int iface = iminD-1 ; iface < imaxD-1 ; iface ++) 
          {
            if (jminD==1) { ind = iface + kface*njD*niD            ; } 
            else         { ind = iface + kface*njD*niD + (njD-1)*niD; } 

            fld[noindint] = fce[ind] ; 
            noindint++;
          }
	}

      } // var loop
    }
    // Si frontiere en k
    // *****************
    if (kminD == kmaxD)
    {
      // printf("Frontiere donneuse en k \n");
      // printf("indD : ");
      E_Int shift = (kminD-1)*niD*njD + nbIntID + nbIntJD ;

      for (E_Int jface = jminD-1 ; jface < jmaxD-1 ; jface ++) 
      {
        for (E_Int iface = iminD-1 ; iface < imaxD-1 ; iface ++) 
        {
          ptrIndFaceD[noindint] = shift + iface + jface*niD ;
          // printf("%d ", ptrIndFaceD[noindint]);
          noindint++;
        }
      }
      // printf("\n ");

    // ~~~~~~~~~~~~~~~~~~~~~~~~
    // Frontiere receveuse en i 
    // ~~~~~~~~~~~~~~~~~~~~~~~~
      if (iminR == imaxR)
      {
        // printf("Frontiere receveuse en i \n");
        noindint = 0 ;

        if (abs(triJ)==2) // iD <-> kR et  jD <-> jR
	{
          // printf("Face receveuse iD=kR, jD=jR) \n");
          // printf("indR : ");
          for (E_Int jface = jminD-1 ; jface < jmaxD-1 ; jface ++) 
          {
            if (triJ > 0){ jfaceR = jface + jminR - jminD ;    }
	    else         { jfaceR = (jmaxR-2)-(jface-jminD+1); }

            for (E_Int iface = iminD-1 ; iface < imaxD-1 ; iface ++) 
            {
              if (triK > 0) { kfaceR = iface + kminR - iminD ;     }
	      else          { kfaceR = (kmaxR-2)-(iface-iminD+1) ; }

              ptrIndFaceR[noindint] = iminR - 1 + jfaceR*(niR+1) + kfaceR*(niR+1)*njR ;
              // printf("%d ", ptrIndFaceR[noindint]);
              noindint++;
	    }
          } 
        } // triJ=2

        if (abs(triJ)==1) // iD <-> jR et  jD <-> kR
	{
          // printf("Face receveuse iD=jR, jD=kR) \n");
          // printf("indR : ");

          for (E_Int jface = jminD-1 ; jface < jmaxD-1 ; jface ++)
          {
            if (triK > 0) { kfaceR = jface + kminR - jminD ;     }
	    else          { kfaceR = (kmaxR-2)-(jface-jminD+1) ; }

            for (E_Int iface = iminD-1 ; iface < imaxD-1 ; iface ++)  
            { 
              if (triJ > 0) { jfaceR = iface + jminR - iminD ;     }
	      else          { jfaceR = (jmaxR-2)-(iface-iminD+1);  }

              ptrIndFaceR[noindint] = iminR - 1 + jfaceR*(niR+1) + kfaceR*(niR+1)*njR ;
              // printf("%d ", ptrIndFaceR[noindint]);
              noindint++;
	    }
          } 
        } // triJ=3
 
        // printf("\n ");
      }

    // ~~~~~~~~~~~~~~~~~~~~~~~~
    // Frontiere receveuse en j 
    // ~~~~~~~~~~~~~~~~~~~~~~~~
      if (jminR == jmaxR)
      {
        // printf("Frontiere receveuse en j \n");
        noindint = 0 ;

        if (abs(triI)==2) // iD <-> kR et  jD <-> iR
	{
          // printf("Face receveuse iD=kR, jD=iR) \n");
          // printf("indR : ");
          E_Int shift = (jminR-1)*niR + nbIntIR ;

          for (E_Int jface = jminD-1 ; jface < jmaxD-1 ; jface ++) 
          {
            if (triI > 0){ ifaceR = jface + iminR - jminD ;    }
	    else         { ifaceR = (imaxR-2)-(jface-jminD+1); }

            for (E_Int iface = iminD-1 ; iface < imaxD-1 ; iface ++) 
            {
              if (triK > 0) { kfaceR = iface + kminR - iminD ;     }
	      else          { kfaceR = (kmaxR-2)-(iface-iminD+1) ; }

              ptrIndFaceR[noindint] = shift + ifaceR + kfaceR*niR*(njR+1) ;
              // printf("%d ", ptrIndFaceR[noindint]);
              noindint++;
	    }
          } 
        } // triJ=2

        if (abs(triI)==1) // iD <-> iR et  jD <-> kR
	{
          // printf("Face receveuse iD=iR, jD=kR) \n");
          // printf("indR : ");
          E_Int shift = (jminR-1)*niR + nbIntIR ;

          for (E_Int jface = jminD-1 ; jface < jmaxD-1 ; jface ++)
          {
            if (triK > 0) { kfaceR = jface + kminR - jminD ;     }
	    else          { kfaceR = (kmaxR-2)-(jface-jminD+1) ; }

            for (E_Int iface = iminD-1 ; iface < imaxD-1 ; iface ++)  
            { 
              if (triI > 0) { ifaceR = iface + iminR - iminD ;     }
	      else          { ifaceR = (imaxR-2)-(iface-iminD+1);  }

              ptrIndFaceR[noindint] = shift + ifaceR + kfaceR*niR*(njR+1) ;
              // printf("%d ", ptrIndFaceR[noindint]);
              noindint++;
	    }
          } 
        } // triJ=3
 
        // printf("\n ");
      }
     
    // ~~~~~~~~~~~~~~~~~~~~~~~~
    // Frontiere receveuse en k
    // ~~~~~~~~~~~~~~~~~~~~~~~~
      if (kminR == kmaxR)
      {
        // printf("Frontiere receveuse en k \n");
        noindint = 0 ;

        if (abs(triI)==1) // iD <-> iR et  jD <-> jR
	{
          // printf("Face receveuse iD=iR, jD=jR) \n");
          // printf("indR : ");
          E_Int shift = (kminR-1)*niR*njR + nbIntIR + nbIntJR ;

          for (E_Int jface = jminD-1 ; jface < jmaxD-1 ; jface ++) 
          {
            if (triJ > 0){ jfaceR = jface + jminR - jminD ;    }
	    else         { jfaceR = (jmaxR-2)-(jface-jminD+1); }

            for (E_Int iface = iminD-1 ; iface < imaxD-1 ; iface ++) 
            {
              if (triI > 0) { ifaceR = iface + iminR - iminD ;     }
	      else          { ifaceR = (imaxR-2)-(iface-iminD+1) ; }

              ptrIndFaceR[noindint] = shift + ifaceR + jfaceR*niR ;
              // printf("%d ", ptrIndFaceR[noindint]);
              noindint++;
	    }
          } 
        } // triI=1

        if (abs(triI)==2) // iD <-> jR et  jD <-> iR
	{
          // printf("Face receveuse iD=jR, jD=iR) \n");
          // printf("indR : ");
          E_Int shift = (kminR-1)*niR*njR + nbIntIR + nbIntJR ;

          for (E_Int jface = jminD-1 ; jface < jmaxD-1 ; jface ++)
          {
            if (triI > 0) { ifaceR = jface + iminR - jminD ;     }
	    else          { ifaceR = (imaxR-2)-(jface-jminD+1) ; }

            for (E_Int iface = iminD-1 ; iface < imaxD-1 ; iface ++)  
            { 
              if (triJ > 0) { jfaceR = iface + jminR - iminD ;     }
	      else          { jfaceR = (jmaxR-2)-(iface-iminD+1);  }

              ptrIndFaceR[noindint] = shift + ifaceR + jfaceR*niR ;
              // printf("%d ", ptrIndFaceR[noindint]);
              noindint++;
	    }
          } 
        } // triJ=3
 
        // printf("\n ");
      }

      // 2. tableau des champs 
      for (E_Int var = 1; var <= nfld; var++)
      {  
        E_Float* fld = fBC->begin(var);
        E_Float* fce = FCenter->begin(var);

        noindint = 0 ;

        for (E_Int jface = jminD-1 ; jface < jmaxD-1 ; jface ++) 
        {
          for (E_Int iface = iminD-1 ; iface < imaxD-1 ; iface ++) 
          {
            if (kminD==1) { ind = iface + jface*niD            ; } 
            else          { ind = iface + jface*niD + (nkD-1)*niD*njD; } 

            fld[noindint] = fce[ind] ; 
            noindint++;
          }
	}

      } // var loop
    }
  }

  RELEASESHAREDS(fields, FCenter);



  PyObject* tplOut ;
  tplOut = Py_BuildValue("[OO]",indFaceR,pyFldD);

  return tplOut; 
}



//=============================================================================
//=============================================================================
// PyObject* K_CONVERTER::computeBCMatchField(PyObject* self, PyObject* args ) // PAS UTILE ??
// {
//   PyObject *pyIndR1, *pyIndR2;
//   PyObject *pyFldD,  *pyFldR;

//   if (!PYPARSETUPLEI(args, "OOOO", "OOOO", 
//                      &pyIndR1, &pyIndR2, &pyFldD, &pyFldR )) return NULL;


//   // Recuperation tableaux indices
//   // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//   FldArrayI* indR1;
//   E_Int resi1 = K_NUMPY::getFromNumpyArray(pyIndR1, indR1, true);
//     if ( resi1 == 0)
//     {
//        PyErr_SetString(PyExc_TypeError, "computeBCMatchField: not a valid numpy for indices of BC (indR1).");
//        RELEASESHAREDN(pyIndR1, indR1);
//        return NULL;   
//     }

//   FldArrayI* indR2;
//   E_Int resi2 = K_NUMPY::getFromNumpyArray(pyIndR2, indR2, true);
//   if ( resi2 == 0)
//   {
//      PyErr_SetString(PyExc_TypeError, "computeBCMatchField: not a valid numpy for indices of BC (indR2).");
//      RELEASESHAREDN(pyIndR2, indR2);
//      return NULL;   
//   }

//   // Map : indices locals <-> globals zone receveuse
//   // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//   std::map<int,int> g2lR; 

//   E_Int nind      = indR2->getSize();
//   E_Int* ptrIndR2 = indR2->begin();
  
//   for (E_Int noindint = 0 ; noindint < nind ; noindint++)
//   {
//     E_Int iglob = ptrIndR2[noindint]; 
//     g2lR[iglob] = noindint ;
//   }


//   // Recuperation tableau champs
//   // ~~~~~~~~~~~~~~~~~~~~~~~~~~~
//   // Zone donneuse
//   // -------------
//   E_Int nint, nn ; 
//   FldArrayF* fldD ; FldArrayI* cn;
//   char* varString; char* eltType;
//   E_Int resf1 = K_ARRAY::getFromArray2(pyFldD, varString, fldD, nint, nn, nn, 
//                                     cn, eltType); 
//   if (resf1 != 1)
//   {
//     PyErr_SetString(PyExc_TypeError, "computeBCMatchField: array must be structured."); 
//     if (resf1 == 2) RELEASESHAREDS(pyFldD, fldD);
//     return NULL; 
//   }

//   // Zone receveuse
//   // --------------
//   FldArrayF* fldR ;
//   E_Int resf2 = K_ARRAY::getFromArray2(pyFldR, varString, fldR, nint, nn, nn, 
//                                     cn, eltType); 
//   if (resf2 != 1)
//   {
//     PyErr_SetString(PyExc_TypeError, "computeBCMatchField: array must be structured."); 
//     if (resf2 == 2) RELEASESHAREDS(pyFldR, fldR);
//     return NULL; 
//   }

//   E_Int nfld = fldD->getNfld();

//   // build output arrays 
//   // ===================
//   PyObject* pyFld = K_ARRAY::buildArray2(nfld,varString,nint,1,1,2); 
//   FldArrayF* fld ;
//   K_ARRAY::getFromArray2(pyFld, varString, fld, nint, nn, nn, cn, eltType);

//   for (E_Int var = 1; var <= nfld; var++)
//   {
//     E_Int*   ptrIndR1 = indR1->begin();
//     E_Float* ptrFldD  = fldD->begin(var);
//     E_Float* ptrFldR  = fldR->begin(var);
//     E_Float* ptrFld   = fld->begin(var); 

//     for (E_Int ilocD = 0 ; ilocD < nint ; ilocD++)
//     {
//       E_Int igloR = ptrIndR1[ilocD];
//       E_Int ilocR = g2lR[igloR];
//       ptrFld[ilocD] = 0.5*( ptrFldD[ilocD] + ptrFldR[ilocR] );
//     }
//   }

//   RELEASESHAREDN(pyIndR1, indR1);
//   RELEASESHAREDN(pyIndR2, indR2);
//   RELEASESHAREDS(pyFldD,  fldD);
//   RELEASESHAREDS(pyFldR,  fldR);

//   return pyFld; 
// }

//=============================================================================
//=============================================================================
void K_CONVERTER::indface2index(E_Int indface, E_Int ni, E_Int nj, E_Int nk, E_Int& ind)
{
  // Return cell index 'ind' given a face index 'indFace' 
  // Warning: only valid for boundary faces imin/imax, jmin/jmax, kmin/kmax 
  //          because for general case 1 face connect with 2 cells
  //          here information of 'min or max' enable to pick a unique (i,j,k) 
  
  // printf("indface : %d \n", indface);
  // printf("ni : %d, nj : %d, nk : %d \n", ni,nj,nk);

  E_Int i,j,k,res;

  E_Int nbIntI = (ni+1)*nj*K_FUNC::E_max(1,nk) ;
  E_Int nbIntJ = (nj+1)*ni*K_FUNC::E_max(1,nk) ;

  i = 10 ;
  j = 20 ;
  k = 30 ; 

  // printf("nbIntI : %d , nbIntJ : %d \n",nbIntI, nbIntJ); 

  if ( indface < nbIntI )
  {
    k   = indface/( (ni+1)*nj ) + 1 ;
    res = indface - (k-1)*(ni+1)*nj;
    j   = res/(ni+1) + 1;
    i   = res - (j-1)*(ni+1) + 1 ;

    if ( i==ni+1) { i = ni; }
  }
  else if ( indface < nbIntI + nbIntJ )
  {
    res = indface - nbIntI;
    k   = res/(ni*(nj+1)) + 1 ;
    res = indface - nbIntI - (k-1)*ni*(nj+1) ; 
    j   = res/ni + 1 ;
    i   = res - (j-1)*ni + 1 ; 

    if ( j==nj+1) { j = nj; }
  }
  else
  {
    res = indface - nbIntI - nbIntJ;
    k   = res/(ni*nj) + 1;
    res = res - (k-1)*ni*nj ; 
    j   = res/ni + 1;
    i   = res - (j-1)*ni + 1;

    if ( k==nk+1) { k = nk; }
    
  }

  ind = i-1 + (j-1)*ni + (k-1)*nj*ni;
  // printf("iface: %d, i: %d, j: %d, k: %d, ind: %d \n",indface,i,j,k,ind);

  return;
}
//=============================================================================
//=============================================================================
PyObject* K_CONVERTER::buildBCMatchFieldNG(PyObject* self, PyObject* args )
{
// compute fld = 0.5(fldD+flR)

  PyObject *zone, *pyIndR, *pyFldD, *pyVariables ; 
  char *GridCoordinates, *FlowSolutionNodes, *FlowSolutionCenters;
  
  if (!PYPARSETUPLEI(args, "OOOOsss", "OOOOsss", &zone, &pyIndR, &pyFldD, 
                     &pyVariables, &GridCoordinates, &FlowSolutionNodes, 
                     &FlowSolutionCenters )) return NULL;

  // Zone 
  // ~~~~
  E_Int ni, nj, nk, cnSize, cnNfld ; 
  char* varString; char* eltType;
  vector<E_Float*> fields; vector<E_Int> locs;
  vector<E_Int*> cn;
  vector<PyArrayObject*> hook;

  E_Int zoneType = K_PYTREE::getFromZone(zone, 0, 1, varString, fields, locs, ni, nj, nk, 
                                         cn, cnSize, cnNfld, eltType, hook, GridCoordinates, 
                                         FlowSolutionNodes, FlowSolutionCenters);

  if ( zoneType == 0) 
  {
    PyErr_SetString(PyExc_TypeError, "buildBCMatchFieldNG: not a valid zone.");
    RELEASESHAREDZ(hook, varString, eltType);
    return NULL;
  }

  // Parent Elements 
  // ~~~~~~~~~~~~~~~
  E_Int* PE = NULL;
  if ( zoneType == 2)
  {
    if ( cn.size() < 3)//PE does not exist
    {
      PyErr_SetString(PyExc_TypeError, "buildBCMatchFieldNG: ParentElements node must be defined in zone.");
      RELEASESHAREDZ(hook, varString, eltType);
      return NULL;  
    }
    else PE = cn[2];
  }

  // Champs de la zone donneuse
  // ~~~~~~~~~~~~~~~~~~~~~~~~~~
  E_Int ni2, nj2, nk2 ;
  FldArrayF* fldD;
  FldArrayI* cn2;
  char* varStringOut;
  E_Int res2 = K_ARRAY::getFromArray2(pyFldD, varStringOut, fldD, ni2, nj2, nk2, 
                                    cn2, eltType); 

  if (res2 != 1)
  {
    PyErr_SetString(PyExc_TypeError, "buildBCMatchFieldNG: wrong array."); 
    RELEASESHAREDS(pyFldD, fldD);
    return NULL; 
  }

  // Positions des variables a extraire 
  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  vector<E_Int> posvars;
  E_Int posvar;   

  if (PyList_Check(pyVariables) != 0)
  {
    int nvariables = PyList_Size(pyVariables);
    if (nvariables > 0)
    {
      for (int i = 0; i < nvariables; i++)
      {
        PyObject* tpl0 = PyList_GetItem(pyVariables, i);
        if (PyString_Check(tpl0) == 0) 
	{
          PyErr_Warn(PyExc_Warning, "buildBCMatchFieldNG: variable must be a string. Skipped.");
	}
        else 
        {
          char* varname    = PyString_AsString(tpl0); 

	  // Verif. presence variables a extraire dans le dict.
	  E_Int verif = K_ARRAY::isNamePresent(varname, varStringOut);  
	  if (verif == -1) 
	  {
	    PyErr_SetString(PyExc_TypeError, "buildBCMatchFieldNG: Variable not found in dictionary allMatch.");
	  }

          posvar = K_ARRAY::isNamePresent(varname, varString);  
          if (posvar != -1 ) posvars.push_back(posvar);
        }
      }
    }
  }

  // Indices des faces 
  // ~~~~~~~~~~~~~~~~~
  FldArrayI* indR;
  E_Int res = K_NUMPY::getFromNumpyArray(pyIndR, indR, true);

  if ( res == 0)
  {
    PyErr_SetString(PyExc_TypeError, "buildBCMatchFieldNG: not a valid numpy for indR.");
    RELEASESHAREDZ(hook, varString, eltType);
    return NULL;   
  }

 // Tableau des champs (output)
  // ~~~~~~~~~~~~~~~~~~
  int nfld = fldD->getNfld();
  int nind = indR->getSize();
  PyObject* pyFld = K_ARRAY::buildArray2(nfld,varStringOut,nind,1,1,2); 

  FldArrayF*  fld; 
  FldArrayI* cn3;
  K_ARRAY::getFromArray2(pyFld, varStringOut, fld, ni2, nj2, nk2, cn3, eltType);


  // Build 0.5(fldD+fldR) array on boundary faces
  // ============================================
  E_Int  ind;
  E_Int* ptrIndR = indR->begin();

  for (E_Int var = 1; var <= nfld; var++)
  {
    E_Int posv          = posvars[var-1];
    E_Float* fieldV     = fields[posv];
    E_Float* ptrFldD    = fldD->begin(var);
    E_Float* ptrFld     = fld->begin(var);

    for (E_Int noindint = 0 ; noindint < nind ; noindint++)
    {
      E_Int indFace    = ptrIndR[noindint]-1;
      E_Int indcell    = PE[indFace]-1;
      ptrFld[noindint] = 0.5*( fieldV[indcell]+ptrFldD[noindint] );    
    }
  }

  return pyFld; 
}


//=============================================================================
//=============================================================================
PyObject* K_CONVERTER::buildBCMatchFieldStruct(PyObject* self, PyObject* args )
{
//   // indR fldD fldR >> fld = 0.5(fldD+flR)

  PyObject *pyFieldsR, *pyIndR, *pyFldD ; 
  
  if (!PYPARSETUPLEI(args, "OOO", "OOO", &pyFieldsR, &pyIndR, &pyFldD )) return NULL;

  // Get current zone fields (in volume)
  // ===================================
  E_Int ni, nj, nk ;
  FldArrayF* fieldsR; FldArrayI* cn;
  char* varString; char* eltType;
  E_Int res = K_ARRAY::getFromArray2(pyFieldsR, varString, fieldsR, ni, nj, nk, 
                                    cn, eltType); 

  if (res != 1)
  {
    PyErr_SetString(PyExc_TypeError, "buildBCMatchFieldStruct: array must be structured."); 
    if (res == 2) RELEASESHAREDS(pyFieldsR, fieldsR);
    return NULL; 
  }

  // Get donor zone fields (on BCMatch)
  // ==================================
  E_Int ni2, nj2, nk2 ;
  FldArrayF* fldD;
  E_Int res2 = K_ARRAY::getFromArray2(pyFldD, varString, fldD, ni2, nj2, nk2, 
                                    cn, eltType); 

  // printf("ni2 : %d, nj2 : %d, nk2 : %d \n", ni2, nj2, nk2);

  if (res2 != 1)
  {
    PyErr_SetString(PyExc_TypeError, "buildBCMatchFieldStruct: array must be structured."); 
    if (res2 == 2) RELEASESHAREDS(pyFldD, fldD);
    return NULL; 
  }

  E_Int nfld = fldD->getNfld();


  // Get index of boundary faces in current zone
  // ============================================
  FldArrayI* indR;
  E_Int resi = K_NUMPY::getFromNumpyArray(pyIndR, indR, true);
  if ( resi == 0)
  {
     PyErr_SetString(PyExc_TypeError, "buildBCMatchFieldStruct: not a valid numpy for indices of BC (indR).");
     RELEASESHAREDN(pyIndR, indR);
     return NULL;   
  }

  E_Int  nind    = indR->getSize();

  // Create output array 
  // ===================
  E_Int nn;
  PyObject* pyFld = K_ARRAY::buildArray2(nfld,varString,nind,1,1,2); 
  // printf("nfld : %d, nind : %d \n",nfld,nind);
  FldArrayF* fld ;
  K_ARRAY::getFromArray2(pyFld, varString, fld, nind, nn, nn, cn, eltType);


  // Build 0.5(fldD+fldR) array on boundary faces
  // ============================================
  E_Int  ind,indFace;
  E_Int* ptrIndR = indR->begin();

  for (E_Int noindint = 0 ; noindint < nind ; noindint++)
  {
    indFace = ptrIndR[noindint]; 
    indface2index(indFace,ni,nj,nk,ind);
    // printf("indFace: %d, ind: %d \n", indFace,ind);

    for (E_Int var = 1; var <= nfld; var++)
    {
      E_Float* ptrFieldsR = fieldsR->begin(var);
      E_Float* ptrFldD    = fldD->begin(var);
      E_Float* ptrFld     = fld->begin(var);
      ptrFld[noindint]    = 0.5*( ptrFieldsR[ind]+ptrFldD[noindint] );    
    }
  }

  RELEASESHAREDN(pyIndR, indR);
  RELEASESHAREDS(pyFldD, fldD);
  RELEASESHAREDS(pyFieldsR, fieldsR);

  return pyFld; 
}
