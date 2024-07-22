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

#include "vertex.h"

struct Hedge;

struct Segment {
    Vertex *p;
    Vertex *q;
    Float dx;
    Float dy;
    Hedge *rep;
    Int id;
    Int color;

    inline Float X1() const { return p->x; }
    inline Float Y1() const { return p->y; }
    inline Float X2() const { return q->x; }
    inline Float Y2() const { return q->y; }
    
    Segment(Vertex *p, Vertex *q, Int Id);
    Segment(Hedge *h, Int Id);
    Segment(Vertex *P);

    bool overlaps(const Segment &seg);

    static void sort(std::vector<Segment *> &S, Int start, Int end,
        Int (*cmp)(const Segment &, const Segment &));
};
