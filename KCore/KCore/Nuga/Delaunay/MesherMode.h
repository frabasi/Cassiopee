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

#ifndef _DELAUNAY_MESHER_MODE_H_
#define _DELAUNAY_MESHER_MODE_H_

#include "Def/DefContainers.h"

namespace DELAUNAY
{
  struct MesherMode
  {
    enum eMeshMode {TRIANGULATION_MODE, REFINE_MODE};
    enum eInterpolType {LINEAR = 0, GEOMETRIC};

    MesherMode():mesh_mode(REFINE_MODE), hmin(K_CONST::E_MAX_FLOAT), hmax(-K_CONST::E_MAX_FLOAT),
                 do_not_shuffle(false), remove_holes(true), silent_errors(false), ignore_coincident_nodes(false), 
                 metric_interpol_type(LINEAR), growth_ratio(1.2), nb_smooth_iter(0){}

    eMeshMode     mesh_mode;
    E_Float       hmin;
    E_Float       hmax;

    E_Bool        do_not_shuffle;
    E_Bool        remove_holes;
    E_Bool        silent_errors;
    E_Bool        ignore_coincident_nodes;
    
    eInterpolType metric_interpol_type;
    E_Float       growth_ratio;
    E_Int         nb_smooth_iter;
    

  };

  struct SurfaceMesherMode : public MesherMode
  {
    enum GMmode
    {
      ISO_CST, ///< A constant size is specified to mesh the surface.
      ISO_RHO, ///< local minimum curvature radius is used to compute the metric.
      ANISO    ///< both local principal curvature radii are used to compute the metric.
    };

    SurfaceMesherMode():MesherMode(), chordal_error(0.01), metric_mode(ISO_RHO){}

    E_Float     chordal_error;
    GMmode      metric_mode;
  };
}

#endif

