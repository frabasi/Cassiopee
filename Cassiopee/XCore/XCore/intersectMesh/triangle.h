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
#pragma once

#include <vector>

#include "point.h"
#include "smesh.h"

struct Triangle {
    E_Int a, b, c;

    static E_Int ispointInside(E_Float x, E_Float y, E_Float ax, E_Float ay,
        E_Float bx, E_Float by, E_Float cx, E_Float cy);

    static E_Int ray_intersect(E_Float px, E_Float py, E_Float pz, E_Float dx,
        E_Float dy, E_Float dz, E_Float ax, E_Float ay, E_Float az, E_Float bx,
        E_Float by, E_Float bz, E_Float cx, E_Float cy, E_Float cz, E_Float &u,
        E_Float &v, E_Float &w, E_Float &t, E_Float &x, E_Float &y, E_Float &z);
};

struct TriIMesh {
    std::vector<point> P;
    std::vector<Triangle> T;

    TriIMesh(const Smesh &M);

    pointFace locate(point p);
};
