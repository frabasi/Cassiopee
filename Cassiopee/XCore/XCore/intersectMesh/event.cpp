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
#include "event.h"
#include "vertex.h"

Event::Event(Float x, Float y)
{
    key = new Vertex(x, y);
    inf = NULL;
    left = right = NULL;
}

Event::Event(Float x, Float y, Int oid, Int color)
{
    key = new Vertex(x, y, oid, color);
    inf = NULL;
    left = right = NULL;
}

void Event::inorder(std::vector<Vertex *> &V) const
{
    if (left) left->inorder(V);
    V.push_back(key);
    if (right) right->inorder(V);
}
