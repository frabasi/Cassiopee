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
#include <cstddef>
#include <cassert>

#include "queue.h"
#include "primitives.h"
#include "event.h"

Queue::Queue()
: root(NULL), nelem(0)
{}

// Insert an intersection
Event *Queue::insert(Float x, Float y)
{
    return insert_(root, x, y);
}

Event *Queue::insert_(Event *&root, Float x, Float y)
{
    if (root == NULL) {
        root = new Event(x, y);
        return root;
    }

    Vertex *key = root->key;

    Int cmp = cmp_points(key->x, key->y, x, y);

    if (cmp == 0) {
        return root;
    } else if (cmp < 0) {
        return insert_(root->right, x, y);
    } else {
        return insert_(root->left, x, y);
    }
}

// Insert an input vertex
Event *Queue::insert(Float x, Float y, Int oid, Int color)
{
    return insert_(root, x, y, oid, color);
}

Event *Queue::insert_(Event *&root, Float x, Float y, Int oid, Int color)
{
    if (root == NULL) {
        root = new Event(x, y, oid, color);
        return root;
    }

    Vertex *key = root->key;

    Int cmp = cmp_points(key->x, key->y, x, y);

    if (cmp == 0) {
        assert(key->oid[color] == -1);
        assert(key->oid[(color+1)%2] != -1);
        key->oid[color] = oid;
        return root;
    } else if (cmp < 0) {
        return insert_(root->right, x, y, oid, color);
    } else {
        return insert_(root->left, x, y, oid, color);
    }
}

Event *Queue::lookup(Float x, Float y)
{
    return lookup_(root, x, y);
}

Event *Queue::lookup(Vertex *key)
{
    return lookup_(root, key->x, key->y);
}

Event *Queue::lookup_(Event *root, Float x, Float y)
{
    if (root == NULL) return NULL;

    Vertex *key = root->key;
    Int cmp = cmp_points(key->x, key->y, x, y);

    if (cmp == 0) return root;
    else if (cmp < 0) return lookup_(root->right, x, y);
    else return lookup_(root->left, x, y);
}

Event *Queue::min()
{
    if (root == NULL) return NULL;

    Event *curr = root;

    while (curr->left != NULL) curr = curr->left;

    return curr;
}

void Queue::erase(Event *event)
{
    root = erase_(root, event->key);
}

void Queue::erase(Vertex *p)
{
    root = erase_(root, p);
}

Event *Queue::erase_(Event *root, Vertex *p)
{
    if (root == NULL) return NULL;

    Int cmp = compare(*root->key, *p);

    if (cmp < 0) {
        root->right = erase_(root->right, p);
        return root;
    } else if (cmp > 0) {
        root->left = erase_(root->left, p);
        return root;
    }

    assert(root->key == p);

    if (root->left == NULL) {
        Event *tmp = root->right;
        delete root;
        return tmp;
    } else if (root->right == NULL) {
        Event *tmp = root->left;
        delete root;
        return tmp;
    } else {
        Event *succ_parent = root;

        Event *succ = root->right;
        while (succ->left) {
            succ_parent = succ;
            succ = succ->left;
        }

        if (succ_parent != root) succ_parent->left = succ->right;
        else succ_parent->right = succ->left;

        root->key = succ->key;
        root->inf = succ->inf;

        delete succ;
        return root;
    }
}

void Queue::inorder(std::vector<Vertex *> &V) const
{
    if (root == NULL) return;
    root->inorder(V);
}
