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

// application of a Gaussian Actuator Line -
// from Ronan Boisard Python source - RotorROM
# include "initiator.h"

using namespace K_FLD;
using namespace std;

#define RELEASEROTMAT                                       \
    for (E_Int nob = 0; nob < NbBlades; nob++)              \
        for (E_Int nosec = 0; nosec < NbPoints; nosec++)    \
        {                                                   \
            E_Int ind = nosec + nob*NbPoints;               \
            PyObject* tpl = PyList_GetItem(listOfRotMat2SourceFrame, ind);\
            RELEASESHAREDN(tpl, vectOfRotMat2LocalFrame[ind]);\
        }

#define RELEASEBLOCK0                                       \
    for (E_Int j = 0; j < NbBlades; j++)                    \
    {                                                       \
        PyObject* tpl = PyList_GetItem(listOfAllLoads, j);  \
        RELEASESHAREDN(tpl, vectOfLoads[j]);                \
    }                                                       

#define RELEASEBLOCKP0                                      \
    for (E_Int j = 0; j < i; j++)                           \
    {                                                       \
        PyObject* tpl = PyList_GetItem(listOfAllLoads, j);  \
        RELEASESHAREDN(tpl, vectOfLoads[j]);                \
    }
                                                      
#define RELEASEBLOCK1                                       \
    for (E_Int j = 0; j < NbBlades; j++)                    \
    {                                                       \
        PyObject* tpl = PyList_GetItem(listOfALPositions, j);         \
        RELEASESHAREDN(tpl, vectOfALPositions[j]);          \
    }    

#define RELEASEBLOCKP1                                          \
    for (E_Int j = 0; j < i; j++)                               \
    {                                                           \
        PyObject* tpl = PyList_GetItem(listOfALPositions, j);   \
        RELEASESHAREDN(tpl, vectOfALPositions[j]);              \
    }
    
#define RELEASEBLOCK2                               \
    RELEASESHAREDN(pyLocalEpsX, localEps_X);  \
    RELEASESHAREDN(pyLocalEpsY, localEps_Y);  \
    RELEASESHAREDN(pyLocalEpsZ, localEps_Z);


// ============================================================================
/* Computation of velocities * gaussian - version non normalisee (GaussInteg=1) */
// ============================================================================
PyObject* K_INITIATOR::getGaussianVelocitiesAL(PyObject* self, PyObject* args)
{
    char* TRUNCVAR;//normalement c est TruncRind ! 
    char* RINDVAR;//rindN
    E_Int NbPoints, NbBlades;
    PyObject *pyLocalEpsX, *pyLocalEpsY, *pyLocalEpsZ;//local epsilon dans chaque direction pour les pts de la section
    PyObject *listOfALPositions;//coordonnees de tous les pts des actuator lines de toutes les pales
    PyObject* array; // champs en centres + coordonnees en centres
    PyObject* listOfRotMat2SourceFrame;// matrice de rotation pour chaque pale & pour chaque section de pale
    // la liste est de longueur nbblades*nbPts : toutes les sections de la 1ere pale, puis toutes les sections de la 2e etc
    if (!PYPARSETUPLE_(args, OOO_ OOO_ II_ S_, 
                        &array, &listOfALPositions, 
                        &listOfRotMat2SourceFrame,
                        &pyLocalEpsX, &pyLocalEpsY, &pyLocalEpsZ,
                        &NbBlades, &NbPoints, &TRUNCVAR, &RINDVAR)) 
        return NULL;  


    // Check array
    E_Int nil, njl, nkl;
    FldArrayF* f; FldArrayI* cn;
    char* varString; char* eltType;
    E_Int res = K_ARRAY::getFromArray3(array, varString, f, nil, njl, nkl, 
                                         cn, eltType);  

    if (res != 1 && res != 2)
    { 
        PyErr_SetString(PyExc_TypeError,
                        "applyLoads: invalid array.");
        return NULL;
    }    
    E_Int posx = K_ARRAY::isNamePresent("XCell", varString); //K_ARRAY::isCoordinateXPresent(varString);
    E_Int posy = K_ARRAY::isNamePresent("YCell", varString);//K_ARRAY::isCoordinateYPresent(varString);
    E_Int posz = K_ARRAY::isNamePresent("ZCell", varString);//K_ARRAY::isCoordinateZPresent(varString);
    E_Int posvol = K_ARRAY::isNamePresent("vol", varString); 
    E_Int postrunc = K_ARRAY::isNamePresent(TRUNCVAR, varString); 
    E_Int posrind = K_ARRAY::isNamePresent(RINDVAR, varString); 
    E_Float posu = K_ARRAY::isVelocityXPresent(varString);
    E_Float posv = K_ARRAY::isVelocityYPresent(varString); 
    E_Float posw = K_ARRAY::isVelocityZPresent(varString);   
    E_Float posuo = K_ARRAY::isNamePresent("VelXOut", varString);
    E_Float posvo = K_ARRAY::isNamePresent("VelYOut", varString); 
    E_Float poswo = K_ARRAY::isNamePresent("VelZOut", varString);       
    if (posx==-1 || posy==-1 || posz==-1 || posvol == -1 || posrind == -1 ||
        posu==-1 || posv==-1 || posw==-1 || postrunc==-1 )
    {
        RELEASESHAREDB(res, array, f, cn);
        PyErr_SetString(PyExc_TypeError,
                    "applyLoads: cannot find coordinates in array.");
        return NULL;
    }
    posx++; posy++; posz++; posvol++; postrunc++; posrind++;
    posu++; posv++; posw++; posuo++; posvo++; poswo++;

    //recuperation des positions des AL de toutes les pales
    vector<FldArrayF*> vectOfALPositions(NbBlades);
    for (E_Int i = 0; i < NbBlades; i++)
    {
        PyObject* pyALPositionsForBlade = PyList_GetItem(listOfALPositions, i);
        FldArrayF* ALPositionsF;
        E_Int resl = K_NUMPY::getFromNumpyArray(pyALPositionsForBlade, ALPositionsF, true);
        if (resl == 0)
        {
            RELEASESHAREDB(res, array, f, cn);
            RELEASEBLOCKP1;        
            PyErr_SetString(PyExc_TypeError, 
                            "applyAL: 2nd arg (AL positions per blade) must be a numpy of floats.");
            return NULL;            
        }
        vectOfALPositions[i] = ALPositionsF;
    }

    vector< FldArrayF*> vectOfRotMat2LocalFrame(NbBlades*NbPoints);
    for (E_Int nob = 0; nob < NbBlades; nob++)
        for (E_Int nosec = 0; nosec < NbPoints; nosec++)
        {
            E_Int ind = nosec + nob*NbPoints;
            PyObject* tpl = PyList_GetItem(listOfRotMat2SourceFrame, ind);
            FldArrayF* RotMatLocal;
            E_Int resl = K_NUMPY::getFromNumpyArray(tpl, RotMatLocal, true);
            if (resl==0)
            {
                RELEASESHAREDB(res, array, f, cn);  
                RELEASEBLOCK1; 
                for (E_Int ind2 = 0; ind2 < ind; ind2++)
                {
                    PyObject* tpl = PyList_GetItem(listOfRotMat2SourceFrame, ind2);
                    RELEASESHAREDN(tpl, vectOfRotMat2LocalFrame[ind]);                  
                }
            }
            vectOfRotMat2LocalFrame[ind] = RotMatLocal;
        }
    // recuperation des epsilons locaux a chq section de pale
    // chaque pale a le meme epsx,epsy,epsz pour une section donnee
    FldArrayF* localEps_X;
    E_Int rese = K_NUMPY::getFromNumpyArray(pyLocalEpsX, localEps_X, true);
    if (rese == 0) 
    {
        RELEASESHAREDB(res, array, f, cn);  
        RELEASEBLOCK1; RELEASEROTMAT;
        PyErr_SetString(PyExc_TypeError, 
                        "applyAL: 3rd arg (eps local wrt x) must be a numpy of floats .");
        return NULL;
    }

    FldArrayF* localEps_Y;
    rese = K_NUMPY::getFromNumpyArray(pyLocalEpsY, localEps_Y, true);
    if (rese == 0) 
    {
        RELEASESHAREDB(res, array, f, cn);  
        RELEASEBLOCK1; RELEASEROTMAT;
        RELEASESHAREDN(pyLocalEpsX, localEps_X);
        PyErr_SetString(PyExc_TypeError, 
                        "applyAL: 4th arg (eps local wrt y) must be a numpy of floats .");
        return NULL;
    }
  
    FldArrayF* localEps_Z;
    rese = K_NUMPY::getFromNumpyArray(pyLocalEpsZ, localEps_Z, true);
    if (rese == 0) 
    {
        RELEASESHAREDB(res, array, f, cn);  
        RELEASEBLOCK1; RELEASEROTMAT;
        RELEASESHAREDN(pyLocalEpsX, localEps_X);
        RELEASESHAREDN(pyLocalEpsY, localEps_Y);
        PyErr_SetString(PyExc_TypeError, 
                        "applyAL: 5th arg (eps local wrt z) must be a numpy of floats .");
        return NULL;
    }
    E_Int npts = f->getSize();
    E_Float* xt = f->begin(posx);
    E_Float* yt = f->begin(posy);
    E_Float* zt = f->begin(posz);  
    E_Float* vxt = f->begin(posu);
    E_Float* vyt = f->begin(posv);
    E_Float* vzt = f->begin(posw);  
    E_Float* vxo = f->begin(posuo);
    E_Float* vyo = f->begin(posvo);
    E_Float* vzo = f->begin(poswo);          
    E_Float* volt = f->begin(posvol);  
    E_Float* trunct = f->begin(postrunc);  
    E_Float* rindt = f->begin(posrind);  
    
    E_Float* epsX = localEps_X->begin();
    E_Float* epsY = localEps_Y->begin();
    E_Float* epsZ = localEps_Z->begin();
    FldArrayF epsXInvF(NbPoints); E_Float* epsXInv = epsXInvF.begin(); 
    FldArrayF epsYInvF(NbPoints); E_Float* epsYInv = epsYInvF.begin();  
    FldArrayF epsZInvF(NbPoints); E_Float* epsZInv = epsZInvF.begin();  
    FldArrayF eps3InvF(NbPoints);  E_Float* eps3Inv = eps3InvF.begin(); 
 #pragma omp parallel default(shared)
  {
#pragma omp for 
    for (E_Int nosec = 0; nosec < NbPoints; nosec++)
    {
        epsXInv[nosec]= 1./epsX[nosec];
        epsYInv[nosec]= 1./epsY[nosec];
        epsZInv[nosec]= 1./epsZ[nosec];
        eps3Inv[nosec]= epsXInv[nosec]*epsYInv[nosec]*epsZInv[nosec];
    }
  }
    E_Float GaussianNormInv = 1.;//on ne normalise pas la gaussienne - cas le plus simple
    E_Float alphaPi = 1./sqrt(K_CONST::E_PI*K_CONST::E_PI*K_CONST::E_PI);

    vector<FldArrayF*> listOfVelX_AL(NbBlades);
    vector<FldArrayF*> listOfVelY_AL(NbBlades);
    vector<FldArrayF*> listOfVelZ_AL(NbBlades);

    for (E_Int nob = 0; nob < NbBlades; nob++)
    {
        E_Float* x_AL = vectOfALPositions[nob]->begin(1);
        E_Float* y_AL = vectOfALPositions[nob]->begin(2);
        E_Float* z_AL = vectOfALPositions[nob]->begin(3);

        /* Tableaux de sorties - un numpy par composante de vitesse par pale*/
        FldArrayF* VelX_AL = new FldArrayF(NbPoints); VelX_AL->setAllValuesAtNull();
        FldArrayF* VelY_AL = new FldArrayF(NbPoints); VelY_AL->setAllValuesAtNull();         
        FldArrayF* VelZ_AL = new FldArrayF(NbPoints); VelZ_AL->setAllValuesAtNull();         
        E_Float* vx_AL = VelX_AL->begin();
        E_Float* vy_AL = VelY_AL->begin();
        E_Float* vz_AL = VelZ_AL->begin();  

        for (E_Int nosec = 0; nosec < NbPoints; nosec++)
        {
            E_Float epsxi = epsXInv[nosec];
            E_Float epsyi = epsYInv[nosec];
            E_Float epszi = epsZInv[nosec];
      
            E_Int nomat = nosec + nob * NbPoints;
            FldArrayF& RotMatLocal = *(vectOfRotMat2LocalFrame[nomat]);            
            E_Float RotMat11 = epsxi*RotMatLocal(0,1);
            E_Float RotMat12 = epsxi*RotMatLocal(1,1);
            E_Float RotMat13 = epsxi*RotMatLocal(2,1);
            E_Float RotMat21 = epsyi*RotMatLocal(0,2);
            E_Float RotMat22 = epsyi*RotMatLocal(1,2);
            E_Float RotMat23 = epsyi*RotMatLocal(2,2);
            E_Float RotMat31 = epszi*RotMatLocal(0,3);
            E_Float RotMat32 = epszi*RotMatLocal(1,3);
            E_Float RotMat33 = epszi*RotMatLocal(2,3);

            E_Float coef = alphaPi * eps3Inv[nosec]; 
            E_Int nthreads = __NUMTHREADS__;
            FldArrayF sums(nthreads,3); sums.setAllValuesAtNull();
            E_Float* sumvx = sums.begin(1);
            E_Float* sumvy = sums.begin(2);
            E_Float* sumvz = sums.begin(3);

            #pragma omp parallel default(shared)
            {
            #pragma omp for 
            for (E_Int ind = 0; ind < npts; ind++)
            {
                vxo[ind]=0.; vyo[ind]=0.; vzo[ind]=0.;                   
                if ( rindt[ind] > 0.)
                {
                    E_Int ithread = __CURRENT_THREAD__;

                    E_Float volCell = volt[ind];
                    E_Float truncCell = trunct[ind];
                    E_Float XCell = xt[ind];
                    E_Float YCell = yt[ind];
                    E_Float ZCell = zt[ind];

                    //(Xcell-X_AL)/epsX
                    E_Float X_ind = 
                        RotMat11*(XCell-x_AL[nosec])+
                        RotMat12*(YCell-y_AL[nosec])+
                        RotMat13*(ZCell-z_AL[nosec]);
            
                    E_Float Y_ind = 
                    RotMat21*(XCell-x_AL[nosec])+
                    RotMat22*(YCell-y_AL[nosec])+
                    RotMat23*(ZCell-z_AL[nosec]);

                    E_Float Z_ind = 
                        RotMat31*(XCell-x_AL[nosec])+
                        RotMat32*(YCell-y_AL[nosec])+
                        RotMat33*(ZCell-z_AL[nosec]);

                    E_Float gauss = coef * volCell * truncCell * exp(-X_ind*X_ind-Y_ind*Y_ind-Z_ind*Z_ind);
                    vxo[ind] = vxt[ind]*gauss*GaussianNormInv;
                    vyo[ind] = vyt[ind]*gauss*GaussianNormInv;
                    vzo[ind] = vzt[ind]*gauss*GaussianNormInv;
                    sumvx[ithread] += vxo[ind];
                    sumvy[ithread] += vyo[ind];
                    sumvz[ithread] += vzo[ind];
                }//rind >0
            }   //npts
            }//omp
        
            for (E_Int nt = 0; nt < nthreads; nt++)
            {
                vx_AL[nosec] += sumvx[nt];
                vy_AL[nosec] += sumvy[nt];
                vz_AL[nosec] += sumvz[nt];
            }
        } // nosections          
        listOfVelX_AL[nob]=VelX_AL;
        listOfVelY_AL[nob]=VelY_AL;
        listOfVelZ_AL[nob]=VelZ_AL;    

    } // noblade
    PyObject* PyListOfVelXAL = PyList_New(0);
    PyObject* PyListOfVelYAL = PyList_New(0);
    PyObject* PyListOfVelZAL = PyList_New(0);

    for (E_Int noblade = 0; noblade < NbBlades; noblade++)
    {
        PyObject* txout = K_NUMPY::buildNumpyArray(*listOfVelX_AL[noblade],1);
        PyList_Append(PyListOfVelXAL, txout); Py_DECREF(txout);
        delete listOfVelX_AL[noblade];
        PyObject* tyout = K_NUMPY::buildNumpyArray(*listOfVelY_AL[noblade],1);
        PyList_Append(PyListOfVelYAL, tyout); Py_DECREF(tyout);
        delete listOfVelY_AL[noblade];       
        PyObject* tzout = K_NUMPY::buildNumpyArray(*listOfVelZ_AL[noblade],1);
        PyList_Append(PyListOfVelZAL, tzout); Py_DECREF(tzout);
        delete listOfVelZ_AL[noblade];               
    }
    RELEASESHAREDB(res, array, f, cn);  
    RELEASEBLOCK1; 

    PyObject* tplout = Py_BuildValue("[OOO]", PyListOfVelXAL, PyListOfVelYAL, PyListOfVelZAL);
    Py_DECREF(PyListOfVelXAL);
    Py_DECREF(PyListOfVelYAL);
    Py_DECREF(PyListOfVelZAL);

    return tplout; 
}
// ============================================================================
/* AL Gaussian - version non normalisee (GaussInteg=1)*/
// ============================================================================
PyObject* K_INITIATOR::applyGaussianAL(PyObject* self, PyObject* args)
{   
    char* TRUNCVAR;
    E_Int NbBlades, NbPoints;
    PyObject *listOfAllLoads;// liste des numpy (3, nbsec) des composantes des forces pour chaque pale 
    PyObject *pyLocalEpsX, *pyLocalEpsY, *pyLocalEpsZ;//local epsilon dans chaque direction pour les pts de la section
    PyObject *listOfALPositions;//coordonnees de tous les pts des actuator lines de toutes les pales
    PyObject* array; // champs en centres + coordonnees en centres
    PyObject* listOfRotMat2SourceFrame;// matrice de rotation pour chaque pale & pour chaque section de pale
    // la liste est de longueur nbblades*nbPts : toutes les sections de la 1ere pale, puis toutes les sections de la 2e etc
    if (!PYPARSETUPLE_(args, OOOO_ OOO_ II_ S_, 
                        &array, &listOfAllLoads, &listOfALPositions, 
                        &listOfRotMat2SourceFrame,
                        &pyLocalEpsX, &pyLocalEpsY, &pyLocalEpsZ,
                        &NbBlades, &NbPoints, &TRUNCVAR)) 
        return NULL;

    // Check array
    E_Int nil, njl, nkl;
    FldArrayF* f; FldArrayI* cn;
    char* varString; char* eltType;
    E_Int res = K_ARRAY::getFromArray3(array, varString, f, nil, njl, nkl, 
                                         cn, eltType);  

    if (res != 1 && res != 2)
    { 
        PyErr_SetString(PyExc_TypeError,
                        "applyLoads: invalid array.");
        return NULL;
    }    
    E_Int posx = K_ARRAY::isNamePresent("XCell", varString); //K_ARRAY::isCoordinateXPresent(varString);
    E_Int posy = K_ARRAY::isNamePresent("YCell", varString);//K_ARRAY::isCoordinateYPresent(varString);
    E_Int posz = K_ARRAY::isNamePresent("ZCell", varString);//K_ARRAY::isCoordinateZPresent(varString);
    E_Int posvol = K_ARRAY::isNamePresent("vol", varString); 
    E_Int postrunc = K_ARRAY::isNamePresent(TRUNCVAR, varString); 
    E_Int posgs = K_ARRAY::isNamePresent("GaussSum", varString); 

    E_Int posrou_src = K_ARRAY::isNamePresent("MomentumX_src", varString); 
    E_Int posrov_src = K_ARRAY::isNamePresent("MomentumY_src", varString); 
    E_Int posrow_src = K_ARRAY::isNamePresent("MomentumZ_src", varString); 
    E_Int posroe_src = K_ARRAY::isNamePresent("EnergyStagnationDensity_src", varString);    
    E_Float posu = K_ARRAY::isVelocityXPresent(varString);
    E_Float posv = K_ARRAY::isVelocityYPresent(varString); 
    E_Float posw = K_ARRAY::isVelocityZPresent(varString);   
    if (posx==-1 || posy==-1 || posz==-1 || posvol == -1 || posgs == -1 ||
        posu==-1 || posv==-1 || posw==-1 || postrunc==-1 ||
        posrou_src==-1 || posrov_src==-1 || posrow_src==-1 || posroe_src==-1)
    {
        RELEASESHAREDB(res, array, f, cn);
        PyErr_SetString(PyExc_TypeError,
                    "applyLoads: cannot find coordinates in array.");
        return NULL;
    }
    posx++; posy++; posz++; posvol++; postrunc++; posgs++; 
    posrou_src++; posrov_src++; posrow_src++; posroe_src++;
    posu++; posv++; posw++;

    //recuperation des composantes des efforts sur les pales
    vector<FldArrayF*> vectOfLoads(NbBlades);
    vector<FldArrayF*> vectOfALPositions(NbBlades);
    for (E_Int i = 0; i < NbBlades; i++)
    {
        PyObject* pyLoadsForBlade = PyList_GetItem(listOfAllLoads, i);
        FldArrayF* loadsF;
        E_Int resl = K_NUMPY::getFromNumpyArray(pyLoadsForBlade, loadsF, true);
        if (resl == 0)
        {
            RELEASESHAREDB(res, array, f, cn);
            RELEASEBLOCKP0;
            PyErr_SetString(PyExc_TypeError, 
                    "applyAL: 2nd arg (loads per blade) must be a numpy of floats.");
            return NULL;            
        }
        vectOfLoads[i] = loadsF;
    }
    for (E_Int i = 0; i < NbBlades; i++)
    {
        PyObject* pyALPositionsForBlade = PyList_GetItem(listOfALPositions, i);
        FldArrayF* ALPositionsF;
        E_Int resl = K_NUMPY::getFromNumpyArray(pyALPositionsForBlade, ALPositionsF, true);
        if (resl == 0)
        {
            RELEASESHAREDB(res, array, f, cn);  
            RELEASEBLOCK0;
            RELEASEBLOCKP1;        
            PyErr_SetString(PyExc_TypeError, 
                            "applyAL: 3rd arg (AL positions per blade) must be a numpy of floats.");
            return NULL;            
        }
        vectOfALPositions[i] = ALPositionsF;
    }
    vector< FldArrayF*> vectOfRotMat2LocalFrame(NbBlades*NbPoints);
    for (E_Int nob = 0; nob < NbBlades; nob++)
        for (E_Int nosec = 0; nosec < NbPoints; nosec++)
        {
            E_Int ind = nosec + nob*NbPoints;
            PyObject* tpl = PyList_GetItem(listOfRotMat2SourceFrame, ind);
            FldArrayF* RotMatLocal;
            E_Int resl = K_NUMPY::getFromNumpyArray(tpl, RotMatLocal, true);
            if (resl==0)
            {
                RELEASESHAREDB(res, array, f, cn);  
                RELEASEBLOCK0;                
                RELEASEBLOCK1; 
                for (E_Int ind2 = 0; ind2 < ind; ind2++)
                {
                    PyObject* tpl = PyList_GetItem(listOfRotMat2SourceFrame, ind2);
                    RELEASESHAREDN(tpl, vectOfRotMat2LocalFrame[ind]);                  
                }
            }
            vectOfRotMat2LocalFrame[ind] = RotMatLocal;
        }

    // recuperation des epsilons locaux a chq section de pale
    // chaque pale a le meme epsx,epsy,epsz pour une section donnee
    FldArrayF* localEps_X;
    E_Int rese = K_NUMPY::getFromNumpyArray(pyLocalEpsX, localEps_X, true);
    if (rese == 0) 
    {
        RELEASESHAREDB(res, array, f, cn);  
        RELEASEBLOCK0;           
        RELEASEBLOCK1; RELEASEROTMAT;
        PyErr_SetString(PyExc_TypeError, 
                        "applyAL: 3rd arg (eps local wrt x) must be a numpy of floats .");
        return NULL;
    }

    FldArrayF* localEps_Y;
    rese = K_NUMPY::getFromNumpyArray(pyLocalEpsY, localEps_Y, true);
    if (rese == 0) 
    {
        RELEASESHAREDB(res, array, f, cn);  
        RELEASEBLOCK0;          
        RELEASEBLOCK1; RELEASEROTMAT;
        RELEASESHAREDN(pyLocalEpsX, localEps_X);
        PyErr_SetString(PyExc_TypeError, 
                        "applyAL: 4th arg (eps local wrt y) must be a numpy of floats .");
        return NULL;
    }
  
    FldArrayF* localEps_Z;
    rese = K_NUMPY::getFromNumpyArray(pyLocalEpsZ, localEps_Z, true);
    if (rese == 0) 
    {
        RELEASESHAREDB(res, array, f, cn);  
        RELEASEBLOCK0;          
        RELEASEBLOCK1; RELEASEROTMAT;
        RELEASESHAREDN(pyLocalEpsX, localEps_X);
        RELEASESHAREDN(pyLocalEpsY, localEps_Y);
        PyErr_SetString(PyExc_TypeError, 
                        "applyAL: 5th arg (eps local wrt z) must be a numpy of floats .");
        return NULL;
    }
    E_Int npts = f->getSize();
    E_Float* xt = f->begin(posx);
    E_Float* yt = f->begin(posy);
    E_Float* zt = f->begin(posz);  
    E_Float* vxt = f->begin(posu);
    E_Float* vyt = f->begin(posv);
    E_Float* vzt = f->begin(posw);      
    E_Float* volt = f->begin(posvol);  
    E_Float* trunct = f->begin(postrunc);  
    E_Float* gausssumt = f->begin(posgs);

    E_Float* MomentumX_src = f->begin(posrou_src);
    E_Float* MomentumY_src = f->begin(posrov_src);
    E_Float* MomentumZ_src = f->begin(posrow_src);
    E_Float* EnergyStagnationDensity_src = f->begin(posroe_src);

    E_Float* epsX = localEps_X->begin();
    E_Float* epsY = localEps_Y->begin();
    E_Float* epsZ = localEps_Z->begin();
    FldArrayF epsXInvF(NbPoints); E_Float* epsXInv = epsXInvF.begin(); 
    FldArrayF epsYInvF(NbPoints); E_Float* epsYInv = epsYInvF.begin();  
    FldArrayF epsZInvF(NbPoints); E_Float* epsZInv = epsZInvF.begin();  
    FldArrayF eps3InvF(NbPoints);  E_Float* eps3Inv = eps3InvF.begin(); 
 #pragma omp parallel default(shared)
  {
#pragma omp for 
    for (E_Int nosec = 0; nosec < NbPoints; nosec++)
    {
        epsXInv[nosec]= 1./epsX[nosec];
        epsYInv[nosec]= 1./epsY[nosec];
        epsZInv[nosec]= 1./epsZ[nosec];
        eps3Inv[nosec]= epsXInv[nosec]*epsYInv[nosec]*epsZInv[nosec];
    }
  }
    E_Float GaussianNormInv = 1.;//on ne normalise pas la gaussienne - cas le plus simple
    E_Float alphaPi = 1./sqrt(K_CONST::E_PI*K_CONST::E_PI*K_CONST::E_PI);

#pragma omp parallel default(shared)
{
#pragma omp for 
    for (E_Int ind = 0; ind < npts; ind++)
    {
        MomentumX_src[ind] = 0.;
        MomentumY_src[ind] = 0.;
        MomentumZ_src[ind] = 0.;
        EnergyStagnationDensity_src[ind] = 0.;
        gausssumt[ind]=0.;  
    }
}
    for (E_Int nob = 0; nob < NbBlades; nob++)
    {
        E_Float* x_AL = vectOfALPositions[nob]->begin(1);
        E_Float* y_AL = vectOfALPositions[nob]->begin(2);
        E_Float* z_AL = vectOfALPositions[nob]->begin(3);
        E_Float* Fx = vectOfLoads[nob]->begin();
        E_Float* Fy = vectOfLoads[nob]->begin()+NbPoints;
        E_Float* Fz = vectOfLoads[nob]->begin()+2*NbPoints;
            
        for(E_Int nosec = 0; nosec < NbPoints; nosec++)
        {

            E_Float Fx0 = Fx[nosec];
            E_Float Fy0 = Fy[nosec];
            E_Float Fz0 = Fz[nosec];
            E_Float epsxi = epsXInv[nosec];
            E_Float epsyi = epsYInv[nosec];
            E_Float epszi = epsZInv[nosec];
      
            E_Int nomat = nosec + nob * NbPoints;
            FldArrayF& RotMatLocal = *(vectOfRotMat2LocalFrame[nomat]);            
            E_Float RotMat11 = epsxi*RotMatLocal(0,1);
            E_Float RotMat12 = epsxi*RotMatLocal(1,1);
            E_Float RotMat13 = epsxi*RotMatLocal(2,1);
            E_Float RotMat21 = epsyi*RotMatLocal(0,2);
            E_Float RotMat22 = epsyi*RotMatLocal(1,2);
            E_Float RotMat23 = epsyi*RotMatLocal(2,2);
            E_Float RotMat31 = epszi*RotMatLocal(0,3);
            E_Float RotMat32 = epszi*RotMatLocal(1,3);
            E_Float RotMat33 = epszi*RotMatLocal(2,3);

            E_Float coef = alphaPi * eps3Inv[nosec]; 

            #pragma omp parallel default(shared)
            {
            #pragma omp for 
            for (E_Int ind = 0; ind < npts; ind++)
            {
                E_Float volCell = volt[ind];
                E_Float truncCell = trunct[ind];
                E_Float XCell = xt[ind];
                E_Float YCell = yt[ind];
                E_Float ZCell = zt[ind];

                //(Xcell-X_AL)/epsX
                E_Float X_ind = 
                    RotMat11*(XCell-x_AL[nosec])+
                    RotMat12*(YCell-y_AL[nosec])+
                    RotMat13*(ZCell-z_AL[nosec]);
            
                E_Float Y_ind = 
                    RotMat21*(XCell-x_AL[nosec])+
                    RotMat22*(YCell-y_AL[nosec])+
                    RotMat23*(ZCell-z_AL[nosec]);

                E_Float Z_ind = 
                    RotMat31*(XCell-x_AL[nosec])+
                    RotMat32*(YCell-y_AL[nosec])+
                    RotMat33*(ZCell-z_AL[nosec]);

                E_Float gauss = coef * volCell * truncCell * exp(-X_ind*X_ind-Y_ind*Y_ind-Z_ind*Z_ind);
                E_Float gaussFx = -gauss*Fx0 * GaussianNormInv;
                E_Float gaussFy = -gauss*Fy0 * GaussianNormInv;
                E_Float gaussFz = -gauss*Fz0 * GaussianNormInv;
                MomentumX_src[ind] += gaussFx;
                MomentumY_src[ind] += gaussFy;
                MomentumZ_src[ind] += gaussFz;
                gausssumt[ind]+=gauss;    
            }   
            }
        }         
    }
#pragma omp parallel default(shared)
{
#pragma omp for     
    for (E_Int ind = 0; ind < npts; ind++)
    {
        EnergyStagnationDensity_src[ind]=MomentumX_src[ind]*vxt[ind]+MomentumY_src[ind]*vyt[ind]+MomentumZ_src[ind]*vzt[ind];
    }
}
    RELEASESHAREDB(res, array, f, cn);  
    RELEASEBLOCK0;  
    RELEASEBLOCK1; 
    RELEASEBLOCK2;
    RELEASEROTMAT;      
    Py_INCREF(Py_None);
    return Py_None;
}

