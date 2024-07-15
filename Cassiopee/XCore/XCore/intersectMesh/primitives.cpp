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
#include <cstdio>
#include <cassert>
#include <cmath>

#include "primitives.h"
#include "event.h"

E_Float TOL = 1e-10;

E_Int Sign(E_Float x)
{
    if (x > TOL) return 1;
    if (x < -TOL) return -1;
    //assert(x == 0.0);
    return 0;
}

E_Int cmp_points(E_Float x1, E_Float y1, E_Float x2, E_Float y2)
{
    E_Float t = x1 - x2;
    E_Int s = Sign(t);
    if (s) return s;
    t = y1 - y2;
    return Sign(t);
}

E_Int cmp_segments
(
    E_Float px0, E_Float py0,
    E_Float px1, E_Float py1,
    E_Float qx1, E_Float qy1,
    E_Float rx, E_Float ry,
    E_Float dx0, E_Float dy0,
    E_Float dx1, E_Float dy1
)
{
    E_Float T1 = dy0 * dx1 - dy1 * dx0;

    E_Int sign1 = Sign(T1);

    if (sign1 == 0) {

        E_Float mdx = qx1 - px0;
        E_Float mdy = qy1 - py0;

        E_Int sign2 = Sign(dy0 * mdx - mdy * dx0);
        
        if (sign2 == 0) {
            E_Int sign3 = Sign(dy1 * mdx - mdy * dx1);

            assert(sign3 == 0);

            if (sign3 == 0) {
                return 0;
            }
        }
    }

    if (Sign(dx0) == 0) {
        E_Float T2 = (py1 * dx1 - px1 * dy1) + (dy1 * rx - ry * dx1);
        E_Int sign2 = Sign(T2);
        return (sign2 <= 0) ? 1 : -1;
    }

    if (Sign(dx1) == 0) {
        E_Float T2 = (py0 * dx0 - px0 * dy0) + (dy0 * rx - ry * dx0);
        E_Int sign2 = Sign(T2);
        return (sign2 <= 0) ? -1 : 1;
    }

    E_Float T2 = dx1 * (py0 * dx0 + dy0 * (rx - px0)) - dx0
        * (py1 * dx1 + dy1 * (rx - px1));

    E_Int sign2 = Sign(T2);
    if (sign2 != 0) return sign2;

    E_Float T3 = (py0 * dx0 - px0 * dy0) + (dy0 * rx - ry * dx0);
    E_Int sign3 = Sign(T3);
    return (sign3 <= 0) ? sign1 : -sign1;
}

// We define the compare functions for Vertexs and Segments by first calling
// cmp_points and cmp_segments on the floating point filter coordinates of the
// corresponding points and segments. In the case that these calls do not
// return a reliable result (i.e. return NO_IDEA) we call them again with the
// exact routines.
E_Int compare(const Vertex &a, const Vertex &b)
{
    return cmp_points(a.x, a.y, b.x, b.y);
}

E_Int compare(const Segment &s1, const Segment &s2, E_Float rx, E_Float ry)
{
    return cmp_segments(s1.p->x, s1.p->y,
                        s2.p->x, s2.p->y,
                        s2.q->x, s2.q->y,
                        rx, ry,
                        s1.dx, s1.dy, s2.dx, s2.dy);
}

E_Int cmp_mySeg(const Segment &s1, const Segment &s2)
{
    E_Int cmp = cmp_points(s1.p->x, s1.p->y, s2.p->x, s2.p->y);
    if (cmp) return cmp;

    cmp = Sign(s1.color - s2.color);
    if (cmp) return cmp;

    cmp = Sign(s1.id - s2.id);

    assert(cmp);

    return cmp;
}

void compute_intersection(Queue &Q, Snode *sit0, Snode *sit1,
    std::vector<Vertex *> &I)
{
    Segment s0 = *sit0->key;
    Segment s1 = *sit1->key;

    E_Float w = s0.dy * s1.dx - s1.dy * s0.dx;
    E_Int i = Sign(w);
    if (i == -1 || i == 0) return;

    E_Float c1 = s0.X2() * s0.Y1() - s0.X1() * s0.Y2();
    E_Float c2 = s1.X2() * s1.Y1() - s1.X1() * s1.Y2();

    E_Float x = c2 * s0.dx - c1 * s1.dx;
    E_Float d0 = x - s0.X2() * w;
    if (Sign(d0) > 0) return;
    if (Sign(x - s1.X2() * w) > 0) return;

    E_Float y = c2 * s0.dy - c1 * s1.dy;
    if (Sign(d0) == 0 &&
        Sign(y - s0.Y2() * w) > 0) return;


    x /= w;
    y /= w;

    Event *xit = Q.lookup(x, y);

    if (xit == NULL) {
        xit = Q.insert(x, y);
        xit->key->id = I.size();
        I.push_back(xit->key);
    }
    
    xit->inf = sit0->key;
    sit0->inf = xit->key;
}

E_Float DifferenceOfProducts(E_Float a, E_Float b, E_Float c, E_Float d)
{
    E_Float cd = c * d;
    E_Float differenceOfProducts = std::fma(a, b, -cd);
    E_Float err = std::fma(-c, d, cd);
    return differenceOfProducts + err;
}

E_Float dRand(E_Float dMin, E_Float dMax)
{
    E_Float d = (E_Float) rand() / RAND_MAX;
    return dMin + d * (dMax - dMin);
}