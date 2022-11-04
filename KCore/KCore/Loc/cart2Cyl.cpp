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

#include "loc.h"
#include <math.h>

//=============================================================================
// Conversion repere Cartesien -> repere cylindrique
// Suivant les axes canoniques
// IN: npts: nbre de pts du maillage
// IN: xt, yt, zt: coord. repere cart
// OUT: rt, thetat: coord. cylindrique 
//=============================================================================
E_Int K_LOC::cart2Cyl(E_Int npts, E_Float* xt, E_Float* yt, E_Float* zt,
                      E_Float X0, E_Float Y0, E_Float Z0,
                      E_Float ex, E_Float ey, E_Float ez,
                      E_Float* rt, E_Float* thetat)
{
    E_Float x0, y0;
    E_Float *xl, *yl;/*, *zl*/
    E_Float eps = 1.e-12;

    // Choix direction suivant axe
    if (ex > eps && ey < eps && ez < eps) // axe X
    {
        xl = yt; yl = zt; //zl = xt;
        x0 = Y0; y0 = Z0;
    }
    else if (ey > eps && ex < eps && ez < eps) // axe Y
    {
        xl = zt; yl = xt; //zl = yt;
        x0 = Z0; y0 = X0;
    }
    else if (ez > eps && ey < eps && ex < eps) // axe Z
    {
        xl = xt; yl = yt; //zl = zt;
        x0 = X0; y0 = Y0;
    }
    else 
    { 
      // Not a canonical axis
      return 1; // FAILED
    }
    // Maintenant axe Z
    E_Float thetaref = atan2(yl[0]-y0,xl[0]-x0);

#pragma omp parallel default(shared)
    {
      E_Float dx, dy, r;
      E_Float theta;
      //E_Float eps = K_CONST::E_ZERO_MACHINE;
      E_Float eps = K_CONST::E_GEOM_CUTOFF;
#pragma omp for 
      for (E_Int ind = 0; ind < npts; ind++)                   
      {
        dx = xl[ind]-x0;
        dy = yl[ind]-y0;
        r = sqrt(dx*dx+dy*dy);
        if ( dx > eps )
        { 
            if ( dy > eps) theta = atan2(dy,dx);
            else if ( dy < -eps) 
            { 
                theta = 2*K_CONST::E_PI+atan2(dy,dx);
            }
            else theta = 0;
        }
        else if ( dx < -eps) 
        {
            if ( dy > eps) theta = atan2(dy,dx);
            else if ( dy < -eps) theta = 2*K_CONST::E_PI+atan2(dy,dx);   
            else theta = K_CONST::E_PI;
        }
        else 
        {
            if ( dy > eps)  theta = K_CONST::E_PI_2;
            else if ( dy < -eps ) theta = 3*K_CONST::E_PI_2;
            else theta = 0.;
        }

        rt[ind] = r; thetat[ind] = theta;
      }
    }
    // il faut corriger pour que les cas theta=2PI ne soient pas theta=0
    for (E_Int ind = 1; ind < npts-1; ind++)
    {
        E_Int indm = ind-1; E_Int indp = ind+1;
        if (thetat[ind]<eps && thetat[indp]<eps && thetat[indm]>eps) thetat[ind] = 2*K_CONST::E_PI;
    }
    if (thetat[npts-1]<eps && thetat[0]<eps && thetat[npts-2]>eps) thetat[npts-1] = 2*K_CONST::E_PI;

    return 0; // OK
}
